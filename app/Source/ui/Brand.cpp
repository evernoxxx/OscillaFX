#include "ui/Brand.h"
#include "ui/BrandAssets.h"

namespace Brand
{
namespace
{
    juce::Image decode (const unsigned char* data, int size)
    {
        return juce::ImageFileFormat::loadFrom (data, (size_t) size);
    }
}

const juce::Image& mark()
{
    static const auto image = decode (BrandAssets::markPng, BrandAssets::markPngSize);
    return image;
}

const juce::Image& trayGlyph()
{
    static const auto image = decode (BrandAssets::trayGlyphPng, BrandAssets::trayGlyphPngSize);
    return image;
}
}
