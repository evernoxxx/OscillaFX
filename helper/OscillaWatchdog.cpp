// Restores the system default output if Oscilla dies while routing through its virtual device.
//
//   OscillaWatchdog <app-pid> <restore-file>     (spawned by the app: <app-pid> must be the parent)
//
// While routing, Oscilla keeps <restore-file> = "<app pid>\n<real output device UID>" and deletes
// it on a clean stop. If the file still names the watched pid when that pid exits, the app crashed
// or was killed: if the virtual device is still the default output, switch back to the saved one.
// OSCILLA_WATCHDOG_DRYRUN=1 logs the decision without touching the default device; in dry-run
// OSCILLA_WATCHDOG_DRYRUN_DEFAULT_UID pretends that UID is the current default (tests only).

#include <CoreAudio/CoreAudio.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <sys/event.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace
{
// Oscilla's driver (main + mirror device) and stock BlackHole, which the app also accepts.
const char* const VIRTUAL_UIDS[] = { "Oscilla_UID", "Oscilla_2_UID", "BlackHole2ch_UID" };

FILE* logFile = stderr;

void openLog()
{
    const char* home = std::getenv ("HOME");
    if (home == nullptr)
        return;
    std::string dir = std::string (home) + "/Library/Logs/Oscilla";
    mkdir (dir.c_str(), 0755);
    if (FILE* file = std::fopen ((dir + "/watchdog.log").c_str(), "a"))
        logFile = file;
}

void logLine (const char* format, ...)
{
    char stamp[32];
    std::time_t now = std::time (nullptr);
    std::strftime (stamp, sizeof (stamp), "%Y-%m-%d %H:%M:%S", std::localtime (&now));
    std::fprintf (logFile, "%s [%d] ", stamp, getpid());
    va_list args;
    va_start (args, format);
    std::vfprintf (logFile, format, args);
    va_end (args);
    std::fputc ('\n', logFile);
    std::fflush (logFile);
}

bool isDryRun() { return std::getenv ("OSCILLA_WATCHDOG_DRYRUN") != nullptr; }

bool isVirtualUid (const std::string& uid)
{
    for (auto* candidate : VIRTUAL_UIDS)
        if (uid == candidate)
            return true;
    return false;
}

// ---------------------------------------------------------------- watching the app

enum class Watch { failed, watching, exited };

// Registers for the parent's exit. Fails (and logs) when `appPid` is not our parent.
Watch watchParent (int queue, pid_t appPid)
{
    struct kevent change, event;
    EV_SET (&change, appPid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, nullptr);
    if (kevent (queue, &change, 1, nullptr, 0, nullptr) < 0)
    {
        if (errno == ESRCH && getppid() != appPid)
            return Watch::exited; // died before we got here; the restore file decides
        logLine ("cannot watch pid %d (errno %d)", appPid, errno);
        return Watch::failed;
    }
    if (getppid() == appPid)
        return Watch::watching;
    const timespec noWait {};
    if (kevent (queue, nullptr, 0, &event, 1, &noWait) == 1)
        return Watch::exited; // died right after registration (we were reparented to launchd)
    logLine ("pid %d is not my parent (parent %d); exiting", appPid, getppid());
    return Watch::failed;
}

void waitForExit (int queue)
{
    struct kevent event;
    while (kevent (queue, nullptr, 0, &event, 1, nullptr) < 0 && errno == EINTR)
    {
    }
}

// UID left behind by an unclean exit of `appPid`; empty if the app stopped cleanly or the file
// belongs to another process (including the legacy single-line format).
std::string pendingRestoreUid (const char* path, pid_t appPid)
{
    std::ifstream file (path);
    std::string pidLine, uid;
    if (! std::getline (file, pidLine))
        return {};
    if (! std::getline (file, uid) || uid.empty())
    {
        logLine ("ignoring restore file in an old or unknown format");
        return {};
    }
    if (std::atoi (pidLine.c_str()) != appPid)
    {
        logLine ("ignoring restore file of pid %s", pidLine.c_str());
        return {};
    }
    return uid;
}

// ---------------------------------------------------------------- CoreAudio

AudioObjectPropertyAddress globalAddress (AudioObjectPropertySelector selector, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal)
{
    return { selector, scope, kAudioObjectPropertyElementMain };
}

AudioObjectID defaultOutput()
{
    AudioObjectID device = kAudioObjectUnknown;
    UInt32 size = sizeof (device);
    auto address = globalAddress (kAudioHardwarePropertyDefaultOutputDevice);
    AudioObjectGetPropertyData (kAudioObjectSystemObject, &address, 0, nullptr, &size, &device);
    return device;
}

std::string deviceUid (AudioObjectID device)
{
    CFStringRef uid = nullptr;
    UInt32 size = sizeof (uid);
    auto address = globalAddress (kAudioDevicePropertyDeviceUID);
    if (AudioObjectGetPropertyData (device, &address, 0, nullptr, &size, &uid) != noErr || uid == nullptr)
        return {};
    char buffer[256] = {};
    CFStringGetCString (uid, buffer, sizeof (buffer), kCFStringEncodingUTF8);
    CFRelease (uid);
    return buffer;
}

AudioObjectID deviceForUid (const std::string& uid)
{
    CFStringRef cfUid = CFStringCreateWithCString (nullptr, uid.c_str(), kCFStringEncodingUTF8);
    AudioObjectID device = kAudioObjectUnknown;
    UInt32 size = sizeof (device);
    auto address = globalAddress (kAudioHardwarePropertyTranslateUIDToDevice);
    AudioObjectGetPropertyData (kAudioObjectSystemObject, &address, sizeof (cfUid), &cfUid, &size, &device);
    CFRelease (cfUid);
    return device;
}

std::vector<AudioObjectID> allDevices()
{
    UInt32 size = 0;
    auto address = globalAddress (kAudioHardwarePropertyDevices);
    if (AudioObjectGetPropertyDataSize (kAudioObjectSystemObject, &address, 0, nullptr, &size) != noErr)
        return {};
    std::vector<AudioObjectID> devices (size / sizeof (AudioObjectID));
    AudioObjectGetPropertyData (kAudioObjectSystemObject, &address, 0, nullptr, &size, devices.data());
    return devices;
}

bool isRealOutput (AudioObjectID device)
{
    UInt32 size = 0;
    auto address = globalAddress (kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput);
    return AudioObjectGetPropertyDataSize (device, &address, 0, nullptr, &size) == noErr && size > 0
        && ! isVirtualUid (deviceUid (device));
}

bool isBuiltIn (AudioObjectID device)
{
    UInt32 transport = 0, size = sizeof (transport);
    auto address = globalAddress (kAudioDevicePropertyTransportType);
    AudioObjectGetPropertyData (device, &address, 0, nullptr, &size, &transport);
    return transport == kAudioDeviceTransportTypeBuiltIn;
}

// Saved device, else (unplugged during the crash) built-in output, else any other real output.
AudioObjectID restoreTarget (const std::string& savedUid)
{
    if (auto saved = deviceForUid (savedUid); saved != kAudioObjectUnknown && isRealOutput (saved))
        return saved;
    AudioObjectID anyReal = kAudioObjectUnknown;
    for (auto device : allDevices())
    {
        if (! isRealOutput (device))
            continue;
        if (isBuiltIn (device))
            return device;
        if (anyReal == kAudioObjectUnknown)
            anyReal = device;
    }
    return anyReal;
}

std::string currentDefaultUid()
{
    const char* pretend = std::getenv ("OSCILLA_WATCHDOG_DRYRUN_DEFAULT_UID");
    return isDryRun() && pretend != nullptr ? pretend : deviceUid (defaultOutput());
}

void restoreOutput (const std::string& savedUid)
{
    const std::string current = currentDefaultUid();
    if (! isVirtualUid (current))
        return logLine ("default output is %s, not the virtual device; nothing to do", current.c_str());

    AudioObjectID target = restoreTarget (savedUid);
    if (target == kAudioObjectUnknown)
        return logLine ("no real output device found (saved uid %s); nothing to restore", savedUid.c_str());

    std::string targetUid = deviceUid (target);
    if (isDryRun())
        return logLine ("DRY RUN: would set default output to %s (saved uid %s)", targetUid.c_str(), savedUid.c_str());

    auto address = globalAddress (kAudioHardwarePropertyDefaultOutputDevice);
    OSStatus status = AudioObjectSetPropertyData (kAudioObjectSystemObject, &address, 0, nullptr, sizeof (target), &target);
    logLine ("set default output to %s (saved uid %s): status %d", targetUid.c_str(), savedUid.c_str(), (int) status);
}
} // namespace

int main (int argc, char** argv)
{
    if (argc != 3)
    {
        std::fprintf (stderr, "usage: %s <app-pid> <restore-file>\n", argv[0]);
        return 2;
    }
    openLog();
    const pid_t appPid = (pid_t) std::atoi (argv[1]);
    const char* restorePath = argv[2];

    int queue = kqueue();
    Watch watch = appPid > 1 ? watchParent (queue, appPid) : Watch::failed;
    if (watch == Watch::failed)
        return 1;
    logLine ("watching pid %d%s", appPid, isDryRun() ? " (dry run)" : "");
    if (watch == Watch::watching)
        waitForExit (queue);
    close (queue);

    std::string uid = pendingRestoreUid (restorePath, appPid);
    if (uid.empty())
    {
        logLine ("pid %d exited cleanly", appPid);
        return 0;
    }
    logLine ("pid %d exited without restoring output", appPid);
    restoreOutput (uid);
    return 0;
}
