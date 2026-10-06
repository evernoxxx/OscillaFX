#include "ProfileStore.h"
#include "core/AppPaths.h"
#include <optional>

namespace
{
const char* const LAST_USED_FILE = "last_used_uid.txt";
const char* const HANDLED_KEY = "suggestionHandled";

void logIssue (const juce::String& message)
{
    juce::Logger::writeToLog ("OscillaFX profiles: " + message);
}

bool isCorrupt (const juce::File& file)
{
    return file.existsAsFile() && ! juce::JSON::parse (file).isObject();
}

std::optional<DspSettings> readProfile (const juce::File& file)
{
    if (! file.existsAsFile())
        return std::nullopt;
    auto parsed = juce::JSON::parse (file);
    if (! parsed.isObject())
    {
        logIssue ("ignoring unreadable " + file.getFullPathName());
        return std::nullopt;
    }
    return DspSettings::fromVar (parsed);
}

// Keeps a corrupt profile as <name>.bak for inspection instead of silently overwriting it.
void backUpIfCorrupt (const juce::File& file)
{
    if (! isCorrupt (file))
        return;
    auto backup = file.getSiblingFile (file.getFileName() + ".bak");
    if (! file.moveFileTo (backup))
        logIssue ("could not back up corrupt " + file.getFullPathName());
}

void writeText (const juce::File& file, const juce::String& text)
{
    if (! file.replaceWithText (text))
        logIssue ("could not write " + file.getFullPathName());
}
} // namespace

ProfileStore::ProfileStore (juce::File directory)
    : dir (std::move (directory))
{
    lastUsedUid = dir.getChildFile (LAST_USED_FILE).loadFileAsString().trim();
}

juce::File ProfileStore::defaultDirectory()
{
    return AppPaths::dataDirectory().getChildFile ("profiles");
}

// Readable prefix plus a hash so distinct UIDs never collide after sanitising.
juce::File ProfileStore::fileFor (const juce::String& deviceUid) const
{
    auto legal = juce::File::createLegalFileName (deviceUid).replaceCharacters (" /:", "___");
    auto hash = juce::String::toHexString ((juce::int64) deviceUid.hashCode64());
    return dir.getChildFile (legal.substring (0, 64) + "_" + hash + ".json");
}

DspSettings ProfileStore::load (const juce::String& deviceUid) const
{
    if (auto own = readProfile (fileFor (deviceUid)))
        return *own;
    if (lastUsedUid.isNotEmpty())
        if (auto last = readProfile (fileFor (lastUsedUid)))
            return *last;
    return {};
}

ProfileStore::Handled ProfileStore::handledState (const juce::File& file)
{
    if (! file.existsAsFile())
        return Handled::no;
    const auto parsed = juce::JSON::parse (file);
    if (! parsed.isObject() || ! parsed.hasProperty (HANDLED_KEY))
        return parsed.isObject() ? Handled::absent : Handled::no;
    return (bool) parsed[HANDLED_KEY] ? Handled::yes : Handled::no;
}

bool ProfileStore::suggestionHandled (const juce::String& deviceUid) const
{
    return handledState (fileFor (deviceUid)) != Handled::no;
}

void ProfileStore::markSuggestionHandled (const juce::String& deviceUid, const DspSettings& current)
{
    write (deviceUid, current, Handled::yes);
}

void ProfileStore::save (const juce::String& deviceUid, const DspSettings& settings)
{
    if (deviceUid.isNotEmpty())
        write (deviceUid, settings, handledState (fileFor (deviceUid)));
}

void ProfileStore::write (const juce::String& deviceUid, const DspSettings& settings, Handled handled)
{
    if (deviceUid.isEmpty())
        return;
    if (auto created = dir.createDirectory(); created.failed())
    {
        logIssue (created.getErrorMessage());
        return;
    }
    auto file = fileFor (deviceUid);
    backUpIfCorrupt (file);
    auto data = settings.toVar();
    if (handled != Handled::absent)
        data.getDynamicObject()->setProperty (HANDLED_KEY, handled == Handled::yes);
    writeText (file, juce::JSON::toString (data));
    if (lastUsedUid != deviceUid)
    {
        lastUsedUid = deviceUid;
        writeText (dir.getChildFile (LAST_USED_FILE), deviceUid);
    }
}
