#pragma once

#include "AppPrefs.h"
#include "GalleryStore.h"
#include "StarterProfiles.h"
#include "SuggestionTracker.h"
#include "audio/AudioEngine.h"
#include "profiles/ProfileStore.h"
#include <juce_events/juce_events.h>
#include <optional>

// Glue between UI, audio engine and per-device profiles. Message thread only.
class Controller : private juce::ChangeBroadcaster
{
public:
    // Folders and files the controller keeps its state in. The defaults are the user's real
    // ~/Library/Application Support/Oscilla ones; tests pass temp locations.
    struct Locations
    {
        juce::File profilesDirectory = ProfileStore::defaultDirectory();
        juce::File prefsFile = AppPrefs::defaultFile();
        juce::File galleryDirectory = GalleryStore::defaultUserDirectory();
        juce::File bundledGalleryDirectory = GalleryStore::defaultBundledDirectory();
    };

    Controller() : Controller (Locations {}) {}
    explicit Controller (const Locations&);
    ~Controller() override;

    // Takes over the system output once microphone access is granted (asks first if undecided).
    // False -> not running yet; lastError() explains (driver missing, microphone access needed...).
    // Starts by itself later when access is granted.
    bool startAudio();
    juce::String lastError() const;

    bool needsMicrophoneAccess() const;  // denied: show openMicrophoneSettings() next to lastError()
    void openMicrophoneSettings();       // System Settings > Privacy & Security > Microphone

    const DspSettings& settings() const { return current; }
    // Edits the active device's profile, applies + saves (debounced). Counts as "the user took
    // control": a pending starter suggestion for this device ends (and is remembered as handled).
    // Includes the safety fields: settings().nightMode, settings().limiterCeilingDb (-24..0, 0 = off).
    void update (const std::function<void (DspSettings&)>& edit);

    juce::StringArray presetNames() const;   // bundled .fac presets
    // Effects + EQ from the preset. Keeps power, master gain, balance, display state, nightMode and
    // limiterCeilingDb. Ends a pending starter suggestion like update().
    void loadPreset (const juce::String& name);

    // ---- per-device starter suggestions ------------------------------------------------------
    // Set when the active output device has no handled profile yet (first time it is used). The new
    // device starts with a copy of the last-used settings and keeps them until the user chooses:
    // nothing is ever applied automatically. Fires a change message whenever the pending state changes.
    // The suggestion lists five options (Flat, Warm, Bass-heavy, Voice, Wide) tuned for the device kind.
    std::optional<ProfileSuggestion> pendingSuggestion() const;
    // Applies the starter (id from ProfileSuggestion::options: "flat", "warm", "bass-heavy", "voice",
    // "wide") on top of the current settings, saves, and marks the device handled. Unknown id or no
    // suggestion pending: no-op.
    void applySuggestion (const juce::String& starterId);
    // Closes the suggestion without changing settings. dontAskAgainForThisDevice: never suggest for this
    // device again; otherwise it can come back the next time the device becomes active after a restart.
    void dismissSuggestion (bool dontAskAgainForThisDevice);

    // ---- gallery (pictures for the phosphor display) -----------------------------------------
    // Everything shown: user folder (~/Library/Application Support/Oscilla/Gallery) plus the bundled
    // pictures, sorted by file name. Re-lists the folders on every call (cheap).
    juce::Array<juce::File> galleryImages() const;
    // Copies jpg/png/webp/heic/gif/tiff files into the user folder (content-checked, max 50 MB each,
    // safe unique names). Returns how many were imported; `error` holds one line per rejected file
    // (empty when all went in). Fires a change message when anything was imported.
    int importGalleryImages (const juce::Array<juce::File>&, juce::String& error);
    // Deletes a picture from the user folder. Bundled pictures and files elsewhere: false, untouched.
    // Fires a change message on success.
    bool removeGalleryImage (const juce::File&);

    // ---- app preferences (prefs.json, not per device) ----------------------------------------
    bool gallerySlideshow() const;               // gallery cycles through pictures by itself; default off
    int gallerySlideshowSeconds() const;         // seconds per picture, 3..600, default 20
    void setGallerySlideshow (bool);             // each setter notifies listeners
    void setGallerySlideshowSeconds (int);
    bool firstRunPending() const;                // true until markFirstRunDone(); survives restarts
    void markFirstRunDone();

    juce::Array<OutputDevice> outputDevices() const;
    OutputDevice activeDevice() const;
    void selectDevice (const juce::String& uid);

    bool launchAtLogin() const;              // system login-item state (SMAppService or LaunchAgent)
    void setLaunchAtLogin (bool shouldStart); // notifies listeners; re-read launchAtLogin() for the outcome

    AudioEngine& engine() { return audio; }

    using juce::ChangeBroadcaster::addChangeListener;    // fired after any settings/device change
    using juce::ChangeBroadcaster::removeChangeListener;

private:
    bool startEngine();
    void requestMicrophoneAccess();
    void waitForMicrophoneAccess();
    void onMicrophonePoll();
    void applyAndSave();
    void saveNow();
    bool switchProfile (const OutputDevice&);

    void refreshSuggestion (const OutputDevice&); // after the active device changed; notifies if it changed
    void endSuggestionByUser();

    AudioEngine audio;
    ProfileStore profiles;
    AppPrefs prefs;
    GalleryStore gallery;
    SuggestionTracker suggestions { profiles };
    DspSettings current;
    OutputDevice device;
    juce::String permissionError; // set while waiting for microphone access; overrides engine errors
    juce::TimedCallback saveTimer { [this] { saveNow(); } }; // debounced profile save
    juce::TimedCallback microphonePoll { [this] { onMicrophonePoll(); } }; // after a denial

    JUCE_DECLARE_WEAK_REFERENCEABLE (Controller)
};
