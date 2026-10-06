#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// CPU fallback of the CRT (used when Metal is unavailable, see crt/CpuCrtRenderer).
// Offscreen phosphor layer at physical pixel resolution. Each frame the previous content decays
// towards the unlit screen colour (persistence) and new beam strokes are added with a soft bloom.
class PhosphorScreen
{
public:
    void prepare (juce::Rectangle<float> logicalBounds, float pixelScale);
    void decay (float dtSeconds, float persistenceSeconds);

    void stroke (const juce::Path&, float intensity, float thickness = 1.0f);
    void spot (juce::Point<float>, float intensity, float radius);
    void mask (const juce::Image& alpha, juce::Rectangle<float> area, float intensity); // lit where alpha is

    void paint (juce::Graphics&) const;
    const juce::Image& getImage() const { return image; }

    static const juce::Colour unlit;

private:
    template <typename Draw>
    void draw (Draw&& drawFn)
    {
        if (! image.isValid())
            return;
        juce::Graphics g (image);
        g.addTransform (juce::AffineTransform::scale (scale).translated (-bounds.getX() * scale, -bounds.getY() * scale));
        drawFn (g);
    }

    juce::Image image;
    juce::Rectangle<float> bounds;
    float scale = 1.0f;
};

// Static glass treatment drawn over the phosphor. The illuminated layer (screen tint, graticule)
// dims with the beam; the glass layer (scanlines, vignette, reflections) is always there.
namespace CrtGlass
{
    struct Layers
    {
        juce::Image illumination, glass;

        // Both layers at full beam folded into one per-pixel multiply-add over the phosphor, so a
        // frame is a single integer pass plus one opaque blit (the common, steady-state case).
        std::vector<juce::uint16> phosphorKeep;   // 0..256
        std::vector<juce::uint32> glassAdd;       // native ARGB, per channel
    };

    Layers render (juce::Rectangle<float> glass, juce::Rectangle<float> graticule,
                   float cornerRadius, float pixelScale);

    // frame = glass over illumination over phosphor, at full beam. Sizes must match the layers.
    void compose (const Layers&, const juce::Image& phosphor, juce::Image& frame);
}
