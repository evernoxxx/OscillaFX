#include "ui/PhosphorScreen.h"
#include "ui/crt/CrtPalette.h"

namespace
{
    juce::Colour toColour (Crt::Rgb c, float alpha = 1.0f)
    {
        return juce::Colour::fromFloatRGBA (c.r, c.g, c.b, alpha);
    }

    const auto body = toColour (Crt::palette.body);
    const auto core = toColour (Crt::palette.core);
    const auto halo = toColour (Crt::palette.halo);

    // A tight, faint halo under a thin teal line with a near-white centre.
    constexpr float haloWidth = 4.5f, haloAlpha = 0.16f;
    constexpr float bodyWidth = 2.0f, coreWidth = 1.0f;
}

const juce::Colour PhosphorScreen::unlit = toColour (Crt::palette.face);

void PhosphorScreen::prepare (juce::Rectangle<float> logicalBounds, float pixelScale)
{
    const auto w = juce::roundToInt (logicalBounds.getWidth() * pixelScale);
    const auto h = juce::roundToInt (logicalBounds.getHeight() * pixelScale);
    if (image.isValid() && image.getWidth() == w && image.getHeight() == h && bounds == logicalBounds)
        return;

    bounds = logicalBounds;
    scale = pixelScale;
    if (w <= 0 || h <= 0)
    {
        image = {};
        return;
    }

    juce::Image resized (juce::Image::ARGB, w, h, false, juce::SoftwareImageType());
    resized.clear (resized.getBounds(), unlit);
    if (image.isValid())
    {
        // keep the glow alive across scale changes (window resize, snapshots)
        juce::Graphics g (resized);
        g.drawImage (image, resized.getBounds().toFloat());
    }
    image = resized;
}

void PhosphorScreen::decay (float dtSeconds, float persistenceSeconds)
{
    if (! image.isValid())
        return;

    // Integer exponential decay towards the unlit colour; strictly converges (no 8-bit ghosting).
    // Two channels at a time in 16-bit lanes; channels the blender left a hair below the unlit
    // colour are clamped to it so they cannot borrow from their neighbours.
    const auto keep = std::exp (-dtSeconds / juce::jmax (0.001f, persistenceSeconds));
    const auto keep256 = (juce::uint32) juce::jlimit (0, 255, (int) (keep * 256.0f));
    const auto bg = juce::PixelARGB (255, unlit.getRed(), unlit.getGreen(), unlit.getBlue()).getNativeARGB();
    const auto bgEven = bg & 0x00ff00ffu, bgOdd = (bg >> 8) & 0x00ff00ffu;

    const auto aboveUnlit = [] (juce::uint32 lanes, juce::uint32 floor)
    {
        const auto biased = lanes + 0x01000100u - floor;              // 0x100 == equal to floor
        const auto positive = ((biased >> 8) & 0x00010001u) * 0xffu;  // lane mask where >= floor
        return biased & 0x00ff00ffu & positive;
    };

    const juce::Image::BitmapData px (image, juce::Image::BitmapData::readWrite);
    jassert (px.pixelStride == 4);
    for (int y = 0; y < px.height; ++y)
    {
        auto* row = reinterpret_cast<juce::uint32*> (px.getLinePointer (y));
        for (int x = 0; x < px.width; ++x)
        {
            const auto even = aboveUnlit (row[x] & 0x00ff00ffu, bgEven);
            const auto odd = aboveUnlit ((row[x] >> 8) & 0x00ff00ffu, bgOdd);
            row[x] = bg + ((((even * keep256) >> 8) & 0x00ff00ffu) | ((odd * keep256) & 0xff00ff00u));
        }
    }

}

void PhosphorScreen::stroke (const juce::Path& path, float intensity, float thickness)
{
    if (intensity <= 0.0f)
        return;

    draw ([&] (juce::Graphics& g)
    {
        const auto joint = juce::PathStrokeType::beveled; // round joins cost far more on long traces
        const auto cap = juce::PathStrokeType::butt;
        g.setColour (halo.withAlpha (juce::jmin (1.0f, haloAlpha * intensity)));
        g.strokePath (path, juce::PathStrokeType (haloWidth * thickness, joint, cap));
        g.setColour (body.withAlpha (juce::jmin (1.0f, 0.9f * intensity)));
        g.strokePath (path, juce::PathStrokeType (bodyWidth * thickness, joint, cap));
        g.setColour (core.withAlpha (juce::jmin (1.0f, 0.9f * intensity)));
        g.strokePath (path, juce::PathStrokeType (coreWidth * thickness, joint, cap));
    });
}

void PhosphorScreen::spot (juce::Point<float> centre, float intensity, float radius)
{
    draw ([&] (juce::Graphics& g)
    {
        const auto reach = radius * 2.5f;
        juce::ColourGradient grad (halo.withAlpha (juce::jmin (1.0f, 0.5f * intensity)), centre,
                                   halo.withAlpha (0.0f), centre.translated (reach, 0.0f), true);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (reach * 2.0f, reach * 2.0f).withCentre (centre));
        g.setColour (core.withAlpha (juce::jmin (1.0f, intensity)));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    });
}

void PhosphorScreen::mask (const juce::Image& alpha, juce::Rectangle<float> area, float intensity)
{
    if (! alpha.isValid() || intensity <= 0.0f)
        return;
    draw ([&] (juce::Graphics& g)
    {
        g.setColour (body.withAlpha (juce::jmin (1.0f, intensity)));
        g.drawImage (alpha, area, juce::RectanglePlacement::stretchToFit, true);
    });
}

void PhosphorScreen::paint (juce::Graphics& g) const
{
    if (! image.isValid())
        return;
    g.drawImageTransformed (image, juce::AffineTransform::scale (1.0f / scale).translated (bounds.getPosition()));
}

//==============================================================================
namespace CrtGlass
{
namespace
{
    constexpr int ticksPerDivision = 5;

    void paintTint (juce::Graphics& g, juce::Rectangle<float> glass)
    {
        const auto c = glass.getCentre();
        const auto centre = toColour (Crt::palette.faceCentre);
        juce::ColourGradient lift (centre.withAlpha (0.7f), c, centre.withAlpha (0.0f),
                                   c.translated (glass.getWidth() * 0.62f, 0.0f), true);
        g.setGradientFill (lift);
        g.fillRect (glass);
    }

    void paintGraticule (juce::Graphics& g, juce::Rectangle<float> grat, float px)
    {
        constexpr int columns = 10, rows = 8;
        const auto colour = toColour (Crt::palette.grid);
        const auto axis = toColour (Crt::palette.gridAxis);
        const auto cellW = grat.getWidth() / columns, cellH = grat.getHeight() / rows;

        g.setColour (colour);
        for (int i = 1; i < columns; ++i)
            g.fillRect (juce::Rectangle<float> (grat.getX() + cellW * (float) i - px * 0.5f, grat.getY(), px, grat.getHeight()));
        for (int i = 1; i < rows; ++i)
            g.fillRect (juce::Rectangle<float> (grat.getX(), grat.getY() + cellH * (float) i - px * 0.5f, grat.getWidth(), px));

        g.setColour (axis);
        g.drawRect (grat, px * 1.2f);
        g.fillRect (juce::Rectangle<float> (grat.getCentreX() - px * 0.5f, grat.getY(), px, grat.getHeight()));
        g.fillRect (juce::Rectangle<float> (grat.getX(), grat.getCentreY() - px * 0.5f, grat.getWidth(), px));

        const auto c = grat.getCentre();
        const auto tick = cellW * 0.12f;
        for (int i = 0; i <= columns * ticksPerDivision; ++i)
        {
            const auto x = grat.getX() + cellW * (float) i / ticksPerDivision;
            g.fillRect (juce::Rectangle<float> (x - px * 0.5f, c.y - tick, px, tick * 2.0f));
        }
        for (int i = 0; i <= rows * ticksPerDivision; ++i)
        {
            const auto y = grat.getY() + cellH * (float) i / ticksPerDivision;
            g.fillRect (juce::Rectangle<float> (c.x - tick, y - px * 0.5f, tick * 2.0f, px));
        }
    }

    void paintScanlines (juce::Image& img)
    {
        constexpr int period = 3;
        juce::Graphics g (img);
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        for (int y = 0; y < img.getHeight(); y += period)
            g.fillRect (0, y, img.getWidth(), 1);
    }

    void paintVignette (juce::Graphics& g, juce::Rectangle<float> glass, float corner)
    {
        const auto c = glass.getCentre();
        juce::ColourGradient vignette (juce::Colours::transparentBlack, c, juce::Colours::black.withAlpha (0.55f),
                                       c.translated (glass.getWidth() * 0.62f, glass.getHeight() * 0.1f), true);
        vignette.addColour (0.6, juce::Colours::transparentBlack);
        vignette.addColour (0.85, juce::Colours::black.withAlpha (0.25f));
        g.setGradientFill (vignette);
        g.fillRect (glass);

        for (int ring = 0; ring < 6; ++ring)
        {
            g.setColour (juce::Colours::black.withAlpha (0.22f - (float) ring * 0.035f));
            g.drawRoundedRectangle (glass.reduced ((float) ring * 1.5f), corner - (float) ring * 1.5f, 2.0f);
        }
    }

    // Opaque black outside the rounded face, so the CRT can paint without a path clip.
    void paintTubeMask (juce::Graphics& g, juce::Rectangle<float> glass, float corner)
    {
        juce::Path mask;
        mask.setUsingNonZeroWinding (false);
        mask.addRectangle (glass.expanded (2.0f));
        mask.addRoundedRectangle (glass, corner);
        g.setColour (juce::Colours::black);
        g.fillPath (mask);
    }

    void paintReflections (juce::Graphics& g, juce::Rectangle<float> glass)
    {
        const auto hot = glass.getRelativePoint (0.24f, 0.16f);
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.045f), hot, juce::Colours::white.withAlpha (0.0f),
                                    hot.translated (glass.getWidth() * 0.38f, 0.0f), true);
        g.setGradientFill (sheen);
        g.fillRect (glass);

        juce::Path catchLight;
        const auto top = glass.reduced (glass.getWidth() * 0.06f, 4.0f);
        catchLight.startNewSubPath (top.getX(), top.getY() + 6.0f);
        catchLight.quadraticTo (top.getCentreX(), top.getY() - 2.0f, top.getRight(), top.getY() + 6.0f);
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.0f), top.getX(), 0.0f, juce::Colours::white.withAlpha (0.0f), top.getRight(), 0.0f, false);
        edge.addColour (0.3, juce::Colours::white.withAlpha (0.08f));
        edge.addColour (0.6, juce::Colours::white.withAlpha (0.03f));
        g.setGradientFill (edge);
        g.strokePath (catchLight, juce::PathStrokeType (2.0f));
    }

// Premultiplied "over": glass + illumination * (1 - Ga) + phosphor * (1 - Ia) * (1 - Ga).
void foldLayers (Layers& layers)
{
    const auto w = layers.glass.getWidth(), h = layers.glass.getHeight();
    layers.phosphorKeep.resize ((size_t) (w * h));
    layers.glassAdd.resize ((size_t) (w * h));
    const juce::Image::BitmapData illum (layers.illumination, juce::Image::BitmapData::readOnly);
    const juce::Image::BitmapData glassPx (layers.glass, juce::Image::BitmapData::readOnly);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto i = illum.getPixelColour (x, y), g = glassPx.getPixelColour (x, y);
            const auto ia = i.getFloatAlpha(), ga = g.getFloatAlpha();
            const auto channel = [&] (juce::uint8 ic, juce::uint8 gc)
            {
                return (juce::uint8) juce::jmin (255.0f, (float) gc * ga + (float) ic * ia * (1.0f - ga));
            };
            const auto keep = (juce::uint16) juce::jlimit (0, 256, (int) ((1.0f - ia) * (1.0f - ga) * 256.0f));
            const auto alpha = (juce::uint8) (255 - ((255 * keep) >> 8));
            const auto index = (size_t) (y * w + x);
            layers.phosphorKeep[index] = keep;
            layers.glassAdd[index] = juce::PixelARGB (alpha, channel (i.getRed(), g.getRed()),
                                                      channel (i.getGreen(), g.getGreen()),
                                                      channel (i.getBlue(), g.getBlue())).getNativeARGB();
        }
    }
}
}

Layers render (juce::Rectangle<float> glass, juce::Rectangle<float> graticule, float corner, float pixelScale)
{
    const auto w = juce::roundToInt (glass.getWidth() * pixelScale), h = juce::roundToInt (glass.getHeight() * pixelScale);
    if (w <= 0 || h <= 0)
        return {};

    const auto toPixels = juce::AffineTransform::translation (-glass.getX(), -glass.getY()).scaled (pixelScale);
    Layers layers { juce::Image (juce::Image::ARGB, w, h, true), juce::Image (juce::Image::ARGB, w, h, true), {}, {} };
    {
        juce::Graphics g (layers.illumination);
        g.addTransform (toPixels);
        paintTint (g, glass);
        paintGraticule (g, graticule, juce::jmax (1.0f / pixelScale, 1.1f));
    }
    paintScanlines (layers.glass);
    {
        juce::Graphics g (layers.glass);
        g.addTransform (toPixels);
        paintVignette (g, glass, corner);
        paintReflections (g, glass);
        paintTubeMask (g, glass, corner);
    }

    foldLayers (layers);
    return layers;
}

void compose (const Layers& layers, const juce::Image& phosphor, juce::Image& frame)
{
    const auto w = phosphor.getWidth(), h = phosphor.getHeight();
    if (layers.glassAdd.size() != (size_t) (w * h))
        return;
    if (frame.getWidth() != w || frame.getHeight() != h)
        frame = juce::Image (juce::Image::ARGB, w, h, false, juce::SoftwareImageType());

    const juce::Image::BitmapData src (phosphor, juce::Image::BitmapData::readOnly);
    const juce::Image::BitmapData dst (frame, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h; ++y)
    {
        const auto* in = reinterpret_cast<const juce::uint32*> (src.getLinePointer (y));
        auto* out = reinterpret_cast<juce::uint32*> (dst.getLinePointer (y));
        const auto* keep = layers.phosphorKeep.data() + (size_t) (y * w);
        const auto* add = layers.glassAdd.data() + (size_t) (y * w);
        for (int x = 0; x < w; ++x)
        {
            const auto k = (juce::uint32) keep[x];
            const auto even = (((in[x] & 0x00ff00ffu) * k) >> 8) & 0x00ff00ffu;
            const auto odd = ((((in[x] >> 8) & 0x00ff00ffu) * k)) & 0xff00ff00u;
            out[x] = add[x] + (even | odd);
        }
    }
}
}
