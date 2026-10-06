#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// One frame's worth of everything the CRT renderers need. All geometry is in CrtDisplay's local
// (logical) coordinates, i.e. the undistorted phosphor plane; renderers scale to device pixels.
namespace Crt
{
    // A beam sweep from a to b: a gaussian line whose steady-state peak energy is `density` (a == b
    // makes a spot): what a line drawn every frame settles at, whatever the persistence or frame rate.
    // `sigma` is the radius in logical pixels.
    struct BeamSegment
    {
        juce::Point<float> a, b;
        float density = 1.0f;
        float sigma = 1.0f;
    };

    // Phosphor "picture" for gallery mode: auto-levelled luminance, row-major, 0..255.
    struct LuminanceImage
    {
        int width = 0, height = 0;
        std::vector<juce::uint8> pixels;
        int id = 0;   // unique per loaded image, lets renderers cache their texture

        bool isValid() const { return width > 0 && height > 0; }
    };

    struct GalleryState
    {
        const LuminanceImage* current = nullptr;
        juce::Rectangle<float> area;    // where the picture sits (contain-fit inside the graticule)
        float gain = 0.0f;              // brightness the picture is refreshed to (breathing + bass)
        juce::Point<float> drift;       // slow parallax offset, logical px
        float tear = 0.0f;              // 0..1 tracking-glitch envelope
        float tearY = 0.5f, tearHeight = 0.1f; // band of the tear, relative to screen height
        float tearSeed = 0.0f;
    };

    // Static text/labels "written" into the phosphor (re-uploaded only when `version` changes).
    struct Overlay
    {
        const juce::Image* image = nullptr;   // SingleChannel, device pixels covering the bounds
        int version = 0;
        float level = 0.0f;                   // phosphor energy the text settles at
    };

    struct Scene
    {
        juce::Rectangle<float> bounds;        // component local
        juce::Rectangle<float> graticule;
        float cornerRadius = 0.0f;

        std::vector<BeamSegment> beams;
        Overlay overlay;                      // labels written into the phosphor (they glow and persist)
        Overlay hint;                         // crisp help text over the glass, bypasses the phosphor/bloom
        GalleryState gallery;

        double time = 0.0;                    // seconds, monotonic
        float dt = 0.0f;
        float persistence = 0.07f;            // phosphor decay time constant, seconds
        float saturation = 1.3f;              // the phosphor cannot store more energy than this
        float pictureResponse = 0.08f;        // how fast overlay/gallery settle, seconds

        float brightness = 1.0f;              // beam current (warm-up)
        float bloom = 1.0f;
        float collapseX = 1.0f, collapseY = 1.0f; // power-off squash, 1 = full picture
        float afterglow = 0.0f;               // the dot left behind by power-off
        float degauss = 0.0f;                 // 0..1 wobble at power-on
        float scaleIllumination = 1.0f;       // graticule lamp
        float faceGlow = 1.0f;                // 0..1 glow of the unlit glass (fades with the beam)
        bool reducedMotion = false;
    };
}
