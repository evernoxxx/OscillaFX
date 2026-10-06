#pragma once

#include <CoreAudio/CoreAudio.h>
#include <functional>

// Keeps the Mac audible if OscillaFX dies while the virtual device is the system default.
namespace CrashGuard
{
// SIGTERM / SIGINT / SIGHUP delivered on the main queue -> onQuit (normal shutdown restores output).
void installQuitSignals (std::function<void()> onQuit);

// SIGSEGV / SIGBUS / SIGABRT / SIGILL / SIGFPE and std::terminate: best-effort switch of the
// system default output back to the saved real device, then the original signal is re-raised.
void installCrashHandlers();

// Real device to restore on a crash; kAudioObjectUnknown while not routing. Any thread.
void setRestoreDevice (AudioObjectID);
} // namespace CrashGuard
