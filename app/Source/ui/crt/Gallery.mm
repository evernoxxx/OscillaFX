#include "ui/crt/Gallery.h"

#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

namespace Crt
{
namespace
{
    constexpr int maxPictureSide = 512;   // the phosphor dot grid is ~100 cells across; plenty
    constexpr juce::uint32 rescanIntervalMs = 2000;
    const char* const extensions = "*.png;*.jpg;*.jpeg;*.webp;*.heic;*.gif;*.tif;*.tiff;*.bmp";

    juce::File bundleDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::currentApplicationFile).getChildFile ("Contents/Resources/Gallery");
    }

    // <repo>/app/Source/ui/crt/Gallery.mm -> <repo>/assets/gallery
    juce::File developmentDirectory()
    {
        auto dir = juce::File (__FILE__);
        for (int i = 0; i < 5; ++i)
            dir = dir.getParentDirectory();
        return dir.getChildFile ("assets/gallery");
    }

    void addPictures (const juce::File& dir, juce::Array<juce::File>& into)
    {
        if (dir.isDirectory())
            into.addArray (dir.findChildFiles (juce::File::findFiles, false, extensions));
    }

    // Stretch the 1st..99.5th percentile to full range so dim and washed-out pictures both glow.
    void autoLevel (LuminanceImage& image)
    {
        std::array<int, 256> histogram {};
        for (auto p : image.pixels)
            ++histogram[p];

        const auto total = (int) image.pixels.size();
        auto percentile = [&] (float fraction)
        {
            auto count = 0;
            for (int v = 0; v < 256; ++v)
                if ((count += histogram[(size_t) v]) >= (int) ((float) total * fraction))
                    return v;
            return 255;
        };

        const auto lo = percentile (0.01f), hi = juce::jmax (lo + 16, percentile (0.995f));
        for (auto& p : image.pixels)
            p = (juce::uint8) juce::jlimit (0, 255, (int) (std::pow (juce::jlimit (0.0f, 1.0f, (float) (p - lo) / (float) (hi - lo)), 1.1f) * 255.0f));
    }
}

juce::File Gallery::userDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support/Oscilla/Gallery");
}

Gallery::Gallery()
{
    if (juce::SystemStats::getEnvironmentVariable ("OSCILLA_GALLERY_DIR", {}).isEmpty())
        userDirectory().createDirectory();   // so the folder named by the "no images" hint exists
}

Gallery::~Gallery()
{
    decoder.removeAllJobs (true, 2000);
}

void Gallery::rescan (bool force)
{
    const auto now = juce::Time::getMillisecondCounter();
    if (! force && hasScanned && now - lastScanMs < rescanIntervalMs)
        return;
    hasScanned = true;
    lastScanMs = now;

    juce::Array<juce::File> found;
    const auto overrideDir = juce::SystemStats::getEnvironmentVariable ("OSCILLA_GALLERY_DIR", {});
    if (overrideDir.isNotEmpty())
    {
        addPictures (juce::File (overrideDir), found);
    }
    else
    {
        addPictures (userDirectory(), found);
        addPictures (bundleDirectory(), found);
        if (found.isEmpty())
            addPictures (developmentDirectory(), found);
    }

    std::sort (found.begin(), found.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareNatural (b.getFileName()) < 0;
    });
    files = found;
}

const LuminanceImage* Gallery::get (int index)
{
    collectDecoded();
    if (! juce::isPositiveAndBelow (index, files.size()))
        return nullptr;

    const auto path = files.getReference (index).getFullPathName();
    if (const auto it = cache.find (path); it != cache.end())
        return it->second.isValid() ? &it->second : nullptr;

    if (pending.insert (path).second)
    {
        decoder.addJob ([path, box = inbox]
        {
            auto image = decodeLuminance (juce::File (path), maxPictureSide);
            if (image.isValid())
                autoLevel (image);
            const std::scoped_lock guard (box->lock);
            box->decoded.emplace_back (path, std::move (image));
        });
    }
    return nullptr;
}

void Gallery::collectDecoded()
{
    std::vector<std::pair<juce::String, LuminanceImage>> arrived;
    {
        const std::scoped_lock guard (inbox->lock);
        arrived.swap (inbox->decoded);
    }
    for (auto& [path, image] : arrived)
    {
        pending.erase (path);
        if (image.isValid())
            image.id = nextId++;
        else
            juce::Logger::writeToLog ("Gallery: cannot decode " + path);
        cache.insert_or_assign (path, std::move (image));
    }
}

// ImageIO thumbnailing: never decodes the full-size image.
LuminanceImage decodeLuminance (const juce::File& file, int maxSide)
{
    LuminanceImage result;
    @autoreleasepool
    {
        const auto path = file.getFullPathName().toStdString();
        CFURLRef url = CFURLCreateFromFileSystemRepresentation (nullptr, (const UInt8*) path.data(), (CFIndex) path.size(), false);
        if (url == nullptr)
            return result;
        CGImageSourceRef source = CGImageSourceCreateWithURL (url, nullptr);
        CFRelease (url);
        if (source == nullptr)
            return result;

        NSDictionary* options = @{ (id) kCGImageSourceCreateThumbnailFromImageAlways: @YES,
                                   (id) kCGImageSourceCreateThumbnailWithTransform: @YES,
                                   (id) kCGImageSourceShouldCacheImmediately: @YES,
                                   (id) kCGImageSourceThumbnailMaxPixelSize: @(maxSide) };
        CGImageRef thumbnail = CGImageSourceCreateThumbnailAtIndex (source, 0, (CFDictionaryRef) options);
        CFRelease (source);
        if (thumbnail == nullptr)
            return result;

        result.width = (int) CGImageGetWidth (thumbnail);
        result.height = (int) CGImageGetHeight (thumbnail);
        result.pixels.assign ((size_t) (result.width * result.height), 0);

        CGColorSpaceRef grey = CGColorSpaceCreateDeviceGray();
        CGContextRef context = CGBitmapContextCreate (result.pixels.data(), (size_t) result.width, (size_t) result.height,
                                                      8, (size_t) result.width, grey, (CGBitmapInfo) kCGImageAlphaNone);
        CGColorSpaceRelease (grey);
        if (context != nullptr)
        {
            CGContextDrawImage (context, CGRectMake (0, 0, result.width, result.height), thumbnail);
            CGContextRelease (context);
        }
        CGImageRelease (thumbnail);
        if (context == nullptr)
            return {};
    }
    return result;
}
}
