#pragma once

#include "core/DspSettings.h"
#include <juce_core/juce_core.h>

// One settings profile per output device, stored as JSON in
// ~/Library/Application Support/Oscilla/profiles/<sanitised uid>.json
class ProfileStore
{
public:
    explicit ProfileStore (juce::File directory = defaultDirectory());

    DspSettings load (const juce::String& deviceUid) const; // unknown device -> copy of last-used profile, else defaults
    void save (const juce::String& deviceUid, const DspSettings&); // keeps the device's suggestionHandled flag

    // Starter-profile suggestions. The flag lives in the device's profile file ("suggestionHandled").
    // No profile yet: not handled (the device is new). Profile without the key (saved before the suggestion feature): handled,
    // so existing devices are never nagged. Profiles created by this app start unhandled until the user decides.
    bool suggestionHandled (const juce::String& deviceUid) const;
    void markSuggestionHandled (const juce::String& deviceUid, const DspSettings& current); // writes the profile too

    static juce::File defaultDirectory();

private:
    enum class Handled { absent, no, yes };
    juce::File fileFor (const juce::String& deviceUid) const;
    static Handled handledState (const juce::File&);
    void write (const juce::String& deviceUid, const DspSettings&, Handled);
    juce::File dir;
    juce::String lastUsedUid;
};
