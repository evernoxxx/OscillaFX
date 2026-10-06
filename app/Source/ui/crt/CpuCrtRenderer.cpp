#include "ui/crt/CpuCrtRenderer.h"

void CpuCrtRenderer::render (const Crt::Scene& scene)
{
    const auto fade = juce::jmin (scene.collapseX, scene.collapseY);
    screen.decay (scene.dt, scene.persistence);
    drawGallery (scene.gallery);
    drawBeams (scene.beams, scene.brightness * fade);
    if (scene.overlay.image != nullptr)
        screen.mask (*scene.overlay.image, scene.bounds, scene.overlay.level * 0.6f);
    if (scene.hint.image != nullptr && scene.hint.level > 0.0f)
        screen.mask (*scene.hint.image, scene.bounds, scene.hint.level * 0.7f);
    if (scene.afterglow > 0.0f)
        screen.spot (scene.graticule.getCentre(), scene.afterglow, 2.5f);
}

void CpuCrtRenderer::drawBeams (const std::vector<Crt::BeamSegment>& beams, float brightness)
{
    // Runs of equal beam width become one stroked path (sub-paths where they are not connected).
    juce::Path path;
    auto peak = 0.0f, sigma = 1.0f;
    auto flush = [&]
    {
        if (! path.isEmpty())
            screen.stroke (path, juce::jmin (1.0f, peak * brightness), sigma);
        path.clear();
        peak = 0.0f;
    };

    for (size_t i = 0; i < beams.size(); ++i)
    {
        const auto& b = beams[i];
        if (b.a == b.b)
        {
            flush();
            screen.spot (b.a, juce::jmin (1.0f, b.density * brightness), b.sigma * 1.2f);
            continue;
        }
        if (! juce::approximatelyEqual (sigma, b.sigma))
            flush();
        if (path.isEmpty() || beams[i - 1].b != b.a)
            path.startNewSubPath (b.a);
        path.lineTo (b.b);
        peak = juce::jmax (peak, b.density);
        sigma = b.sigma;
    }
    flush();
}

void CpuCrtRenderer::drawGallery (const Crt::GalleryState& gallery)
{
    const auto* picture = gallery.current;
    if (picture == nullptr || gallery.gain <= 0.0f)
        return;

    if (picture->id != galleryId)
    {
        galleryMask = juce::Image (juce::Image::SingleChannel, picture->width, picture->height, false, juce::SoftwareImageType());
        const juce::Image::BitmapData px (galleryMask, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < picture->height; ++y)
            std::memcpy (px.getLinePointer (y), picture->pixels.data() + (size_t) (y * picture->width), (size_t) picture->width);
        galleryId = picture->id;
    }
    screen.mask (galleryMask, gallery.area + gallery.drift, juce::jmin (1.0f, gallery.gain * 0.35f));
}

void CpuCrtRenderer::paint (juce::Graphics& g, const Crt::Scene& scene)
{
    const auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (! glass.glass.isValid() || ! juce::approximatelyEqual (scale, pixelScale) || laidOut != scene.bounds)
    {
        pixelScale = scale;
        laidOut = scene.bounds;
        screen.prepare (scene.bounds, scale);
        glass = CrtGlass::render (scene.bounds, scene.graticule, scene.cornerRadius, scale);
    }

    // Device-pixel aligned: an unscaled blit is much cheaper than a resampled one.
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    CrtGlass::compose (glass, screen.getImage(), frame);
    g.drawImageTransformed (frame, juce::AffineTransform::scale (1.0f / pixelScale));
}

juce::Image CpuCrtRenderer::snapshot()
{
    if (glass.glass.isValid())
        CrtGlass::compose (glass, screen.getImage(), frame);
    return frame.createCopy();   // frame is reused every paint
}
