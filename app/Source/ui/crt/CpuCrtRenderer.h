#pragma once

#include "ui/PhosphorScreen.h"
#include "ui/crt/CrtScene.h"

// Fallback CRT drawn with JUCE on the CPU, used when Metal is unavailable. Consumes the same
// Scene as the GPU path with a simpler look (no lens, bloom pyramid or tone mapping).
class CpuCrtRenderer
{
public:
    void render (const Crt::Scene&);                  // advance the phosphor one frame
    void paint (juce::Graphics&, const Crt::Scene&);  // composite into the component
    juce::Image snapshot();                           // the current picture, device pixels

private:
    void drawBeams (const std::vector<Crt::BeamSegment>&, float brightness);
    void drawGallery (const Crt::GalleryState&);

    PhosphorScreen screen;
    CrtGlass::Layers glass;
    juce::Image frame;
    juce::Rectangle<float> laidOut;
    float pixelScale = 0.0f;
    juce::Image galleryMask;
    int galleryId = -1;
};
