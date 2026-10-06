#pragma once

#include <juce_core/juce_core.h>

// The picture folders behind the phosphor gallery. Message thread only.
//   user folder:    ~/Library/Application Support/Oscilla/Gallery   (imports go here, removable)
//   bundled folder: OscillaFX.app/Contents/Resources/Gallery          (read-only)
// $OSCILLA_GALLERY_DIR, when set, replaces both (dev/testing). When both hold nothing, the source
// tree's assets/gallery is listed (dev runs). Mirrors what ui/crt/Gallery.mm shows.
class GalleryStore
{
public:
    static constexpr juce::int64 maxImageBytes = 50 * 1024 * 1024;

    GalleryStore (juce::File userDirectory = defaultUserDirectory(), juce::File bundledDirectory = defaultBundledDirectory());

    // Every picture, user and bundled merged, sorted by file name (natural order).
    juce::Array<juce::File> images() const;

    // Copies each file in under a safe, unique name with the extension its content really has
    // (jpg png webp heic gif tiff; first frame of a GIF is what the gallery shows). A file is
    // rejected when it is not a readable image (checked on the stored copy, by content), is over
    // maxImageBytes, or cannot be read. Returns how many were imported. `error` is cleared, then
    // holds one line per rejected file (empty when everything went in).
    int importImages (const juce::Array<juce::File>&, juce::String& error);

    // Deletes a picture, only if it sits directly in the user folder (symlinks and ".." resolved).
    // Bundled pictures and anything outside return false and are left alone.
    bool removeImage (const juce::File&);

    const juce::File& userDirectory() const { return userDir; }

    static juce::File defaultUserDirectory();
    static juce::File defaultBundledDirectory();

private:
    juce::File userDir, bundledDir;
};
