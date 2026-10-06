#include "ui/Textures.h"

namespace Textures
{
namespace
{
    // Tileable value noise over a wrapped random lattice with independent x / y cell counts, so it
    // can be stretched along one axis (brush strokes). Returns -1..1.
    class ValueNoise
    {
    public:
        ValueNoise (int cellsXIn, int cellsYIn, juce::uint32 seed)
            : cellsX (juce::jmax (1, cellsXIn)), cellsY (juce::jmax (1, cellsYIn))
        {
            juce::Random rng ((juce::int64) seed);
            lattice.resize ((size_t) (cellsX * cellsY));
            for (auto& v : lattice)
                v = rng.nextFloat() * 2.0f - 1.0f;
        }

        float at (float u, float v) const // u, v in 0..1
        {
            const auto x = u * (float) cellsX, y = v * (float) cellsY;
            const auto x0 = (int) x, y0 = (int) y;
            const auto fx = smooth (x - (float) x0), fy = smooth (y - (float) y0);
            const auto a = cell (x0, y0), b = cell (x0 + 1, y0);
            const auto c = cell (x0, y0 + 1), d = cell (x0 + 1, y0 + 1);
            return juce::jmap (fy, juce::jmap (fx, a, b), juce::jmap (fx, c, d));
        }

    private:
        static float smooth (float t) { return t * t * (3.0f - 2.0f * t); }
        float cell (int x, int y) const { return lattice[(size_t) ((y % cellsY) * cellsX + (x % cellsX))]; }

        int cellsX, cellsY;
        std::vector<float> lattice;
    };

    // Writes a signed noise field as a white (positive) / black (negative) translucent overlay.
    template <typename Field>
    juce::Image overlay (int width, int height, float strength, Field&& field)
    {
        juce::Image img (juce::Image::ARGB, width, height, true);
        const juce::Image::BitmapData px (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const auto n = juce::jlimit (-1.0f, 1.0f, field (x, y));
                const auto tone = n > 0.0f ? juce::Colours::white : juce::Colours::black;
                px.setPixelColour (x, y, tone.withAlpha (std::abs (n) * strength));
            }
        }
        return img;
    }

    // Returned by value (images are reference counted), so the result stays valid even if this
    // call held the last reference to the cache.
    template <typename Make>
    juce::Image cached (juce::Image Cache::* slot, Make&& make)
    {
        juce::SharedResourcePointer<Cache> cache;
        auto& image = (*cache).*slot;
        if (image.isNull())
            image = make();
        return image;
    }

    // Matte-paint "orange peel": a soft bumpy height field (three octaves plus a little pitting),
    // lit from the upper left, written as a light / dark overlay. Tileable.
    struct PeelRecipe
    {
        int size;
        int cellsFine, cellsMid, cellsCoarse;
        float weightFine, weightMid, weightCoarse, pitting;
        float relief;     // shading gain
        float strength;   // overlay opacity at full relief
        juce::uint32 seed;
    };

    juce::Image makePeel (const PeelRecipe& r)
    {
        const ValueNoise fine (r.cellsFine, r.cellsFine, r.seed), mid (r.cellsMid, r.cellsMid, r.seed + 1),
                         coarse (r.cellsCoarse, r.cellsCoarse, r.seed + 2);
        juce::Random rng ((juce::int64) r.seed + 3);
        const auto size = r.size;

        std::vector<float> height ((size_t) (size * size));
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const auto u = (float) x / (float) size, v = (float) y / (float) size;
                height[(size_t) (y * size + x)] = r.weightFine * fine.at (u, v) + r.weightMid * mid.at (u, v)
                                                + r.weightCoarse * coarse.at (u, v) + r.pitting * (rng.nextFloat() * 2.0f - 1.0f);
            }

        const auto h = [&] (int x, int y) { return height[(size_t) (((y + size) % size) * size + ((x + size) % size))]; };
        return overlay (size, size, r.strength, [&] (int x, int y)
        {
            return r.relief * (h (x - 1, y - 1) - h (x + 1, y + 1));
        });
    }

    juce::Image makeSpunAluminium()
    {
        constexpr int size = 256;
        constexpr float lightAngle = -2.36f;
        juce::Image img (juce::Image::ARGB, size, size, false);
        const juce::Image::BitmapData px (img, juce::Image::BitmapData::writeOnly);
        juce::Random rng (113);

        std::vector<float> rings (size);
        for (auto& r : rings)
            r = rng.nextFloat() * 2.0f - 1.0f;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto dx = (float) x - size * 0.5f, dy = (float) y - size * 0.5f;
                const auto radius = std::sqrt (dx * dx + dy * dy);
                const auto angle = std::atan2 (dy, dx);
                // Radial spin marks reflect light in a bow-tie perpendicular to the light direction.
                const auto sheen = std::pow (std::abs (std::cos (angle - lightAngle - juce::MathConstants<float>::halfPi)), 6.0f);
                const auto ring = rings[(size_t) juce::jlimit (0, size - 1, (int) (radius * 1.4f))];
                const auto lum = 0.60f + 0.30f * sheen + 0.035f * ring + 0.015f * (rng.nextFloat() * 2.0f - 1.0f)
                               - 0.10f * (radius / (size * 0.5f));
                px.setPixelColour (x, y, juce::Colour::fromFloatRGBA (lum, lum, lum * 1.01f, 1.0f));
            }
        }
        return img;
    }
}

juce::Image orangePeel()
{
    return cached (&Cache::orangePeel, []
    {
        return makePeel ({ 768, 384, 160, 64, 0.8f, 0.45f, 0.15f, 0.5f, 1.0f, 0.2f, 211 });
    });
}

juce::Image paintedMetal()
{
    return cached (&Cache::paintedMetal, []
    {
        return makePeel ({ 768, 384, 160, 32, 0.6f, 0.3f, 0.15f, 0.4f, 1.0f, 0.12f, 223 });
    });
}

juce::Image rubberGrain()
{
    return cached (&Cache::rubberGrain, []
    {
        const ValueNoise fine (160, 160, 47);
        juce::Random rng (53);
        return overlay (512, 512, 0.07f, [&] (int x, int y)
        {
            return 0.4f * fine.at ((float) x / 512.0f, (float) y / 512.0f) + 0.7f * (rng.nextFloat() * 2.0f - 1.0f);
        });
    });
}

juce::Image spunAluminium()
{
    return cached (&Cache::spunAluminium, makeSpunAluminium);
}
}
