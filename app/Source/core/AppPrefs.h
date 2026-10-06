#pragma once

#include <juce_core/juce_core.h>

// Small per-user preferences that belong to the app, not to an output device:
// ~/Library/Application Support/Oscilla/prefs.json ($OSCILLA_DATA_DIR/prefs.json when that is set). Every change is written at once (atomic replace).
// A missing or corrupt file means defaults; a corrupt one is kept as prefs.json.bak. Message thread only.
class AppPrefs
{
public:
    static constexpr int slideshowMinSeconds = 3, slideshowMaxSeconds = 600, slideshowDefaultSeconds = 20;

    explicit AppPrefs (juce::File file = defaultFile());

    bool gallerySlideshow() const { return slideshow; }              // cycle the gallery pictures automatically
    int gallerySlideshowSeconds() const { return slideshowSeconds; } // seconds per picture, 3..600, default 20
    bool firstRunDone() const { return firstRun; }                   // the welcome flow has been completed

    void setGallerySlideshow (bool);
    void setGallerySlideshowSeconds (int);   // clamped
    void setFirstRunDone (bool);

    static juce::File defaultFile();

private:
    void load();
    void save() const;

    juce::File file;
    bool slideshow = false;
    int slideshowSeconds = slideshowDefaultSeconds;
    bool firstRun = false;
};
