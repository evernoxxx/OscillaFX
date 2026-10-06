#include "AppPrefs.h"
#include "AppPaths.h"

AppPrefs::AppPrefs (juce::File prefsFile)
    : file (std::move (prefsFile))
{
    load();
}

juce::File AppPrefs::defaultFile()
{
    return AppPaths::dataDirectory().getChildFile ("prefs.json");
}

void AppPrefs::setGallerySlideshow (bool enabled)
{
    if (slideshow == enabled)
        return;
    slideshow = enabled;
    save();
}

void AppPrefs::setGallerySlideshowSeconds (int seconds)
{
    seconds = juce::jlimit (slideshowMinSeconds, slideshowMaxSeconds, seconds);
    if (slideshowSeconds == seconds)
        return;
    slideshowSeconds = seconds;
    save();
}

void AppPrefs::setFirstRunDone (bool done)
{
    if (firstRun == done)
        return;
    firstRun = done;
    save();
}

void AppPrefs::load()
{
    if (! file.existsAsFile())
        return;
    const auto parsed = juce::JSON::parse (file);
    if (! parsed.isObject())
    {
        juce::Logger::writeToLog ("OscillaFX prefs: unreadable " + file.getFullPathName() + ", using defaults");
        file.moveFileTo (file.getSiblingFile (file.getFileName() + ".bak"));
        return;
    }
    slideshow = (bool) parsed.getProperty ("gallerySlideshow", slideshow);
    slideshowSeconds = juce::jlimit (slideshowMinSeconds, slideshowMaxSeconds,
                                     (int) parsed.getProperty ("gallerySlideshowSeconds", slideshowSeconds));
    firstRun = (bool) parsed.getProperty ("firstRunDone", firstRun);
}

void AppPrefs::save() const
{
    auto* obj = new juce::DynamicObject();
    const juce::var data (obj);
    obj->setProperty ("gallerySlideshow", slideshow);
    obj->setProperty ("gallerySlideshowSeconds", slideshowSeconds);
    obj->setProperty ("firstRunDone", firstRun);
    if (auto created = file.getParentDirectory().createDirectory(); created.failed())
    {
        juce::Logger::writeToLog ("OscillaFX prefs: " + created.getErrorMessage());
        return;
    }
    if (! file.replaceWithText (juce::JSON::toString (data)))
        juce::Logger::writeToLog ("OscillaFX prefs: could not write " + file.getFullPathName());
}
