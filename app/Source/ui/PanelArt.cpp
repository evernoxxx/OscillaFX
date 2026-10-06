#include "ui/PanelArt.h"
#include "core/DspSettings.h"
#include "ui/Brand.h"
#include "ui/Engraving.h"
#include "ui/Fader.h"
#include "ui/Hardware.h"
#include "ui/Textures.h"
#include "ui/Theme.h"

namespace PanelArt
{
namespace
{
    using juce::Point;
    using juce::Rectangle;
    namespace D = Theme::Design;

    constexpr float labelSize = 15.0f;
    constexpr float headerSize = 16.0f;
    constexpr float scaleTextSize = 12.0f;
    constexpr float boxCorner = 12.0f;
    constexpr float ruleThickness = 1.7f;
    constexpr float textureScale = 0.5f;   // tiles are authored at 2x
    constexpr float knobLabelGap = 36.0f;  // from the knob's rim to its legend

    juce::Path rounded (Rectangle<float> r, float corner)
    {
        juce::Path p;
        p.addRoundedRectangle (r, corner);
        return p;
    }

    void fillTexture (juce::Graphics& g, const juce::Path& area, const juce::Image& tile)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (area);
        g.setFillType (juce::FillType (tile, juce::AffineTransform::scale (textureScale)));
        g.fillPath (area);
    }

    Point<float> onDial (Point<float> c, float radius, float angle)
    {
        return c + Point<float> (std::sin (angle), -std::cos (angle)) * radius;
    }

    //==========================================================================
    // Knob scale
    struct ScaleMark
    {
        float position;    // 0..1 across the knob's sweep
        juce::String text;
    };

    void paintKnobScale (juce::Graphics& g, Point<float> c, float knobRadius, int tickCount,
                         std::initializer_list<ScaleMark> marks, bool withArc)
    {
        const auto inner = knobRadius + 9.0f, outer = inner + 6.0f, major = inner + 10.0f;
        if (withArc)
        {
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, inner, inner, 0.0f, -D::knobSweep, D::knobSweep, true);
            Engraving::path (g, arc, 1.3f);
        }

        for (int i = 0; i < tickCount; ++i)
        {
            const auto t = (float) i / (float) (tickCount - 1);
            const auto a = juce::jmap (t, -D::knobSweep, D::knobSweep);
            const auto isMajor = std::any_of (marks.begin(), marks.end(), [t] (const ScaleMark& m) { return std::abs (m.position - t) < 0.001f; });
            Engraving::line (g, onDial (c, inner, a), onDial (c, isMajor ? major : outer, a), isMajor ? 1.7f : 1.1f);
        }

        for (const auto& mark : marks)
        {
            const auto a = juce::jmap (mark.position, -D::knobSweep, D::knobSweep);
            Engraving::text (g, mark.text, onDial (c, major + 10.0f, a), scaleTextSize, true);
        }
    }

    //==========================================================================
    // Fader scales
    struct FaderMark
    {
        float position;    // 0..1 along the travel
        juce::String text;
    };

    // Ticks both sides of a vertical fader; labels (if any) to the left of the left ticks.
    void paintVerticalScale (juce::Graphics& g, Rectangle<float> bounds, int divisions,
                             std::initializer_list<FaderMark> marks, bool withLabels)
    {
        constexpr float gap = 20.0f;
        const auto cx = bounds.getCentreX();
        for (int i = 0; i <= divisions; ++i)
        {
            const auto t = (float) i / (float) divisions;
            const auto y = Fader::capCentre (bounds, Fader::Orientation::vertical, t);
            const auto isMajor = std::any_of (marks.begin(), marks.end(), [t] (const FaderMark& m) { return std::abs (m.position - t) < 0.001f; });
            const auto length = isMajor ? 7.0f : 4.0f;
            Engraving::line (g, { cx - gap, y }, { cx - gap - length, y }, isMajor ? 1.5f : 1.0f);
            Engraving::line (g, { cx + gap, y }, { cx + gap + length, y }, isMajor ? 1.5f : 1.0f);
        }

        if (! withLabels)
            return;

        for (const auto& mark : marks)
        {
            const auto y = Fader::capCentre (bounds, Fader::Orientation::vertical, mark.position);
            Engraving::text (g, mark.text, { cx - gap - 12.0f, y }, scaleTextSize, true, juce::Justification::right);
        }
    }

    void paintHorizontalScale (juce::Graphics& g, Rectangle<float> bounds, int divisions)
    {
        const auto cy = bounds.getCentreY();
        for (int i = 0; i <= divisions; ++i)
        {
            const auto t = (float) i / (float) divisions;
            const auto x = Fader::capCentre (bounds, Fader::Orientation::horizontal, t);
            const auto isMajor = i == 0 || i == divisions || i * 2 == divisions;
            Engraving::line (g, { x, cy - 17.0f }, { x, cy - 17.0f - (isMajor ? 7.0f : 4.0f) }, isMajor ? 1.5f : 1.0f);
        }
    }

    void boxWithHeader (juce::Graphics& g, Rectangle<float> box, const juce::String& header)
    {
        Engraving::path (g, rounded (box, boxCorner), ruleThickness);
        Engraving::text (g, header, { box.getX() + 16.0f, box.getY() + 20.0f }, headerSize, true, juce::Justification::left);
    }

    void knobLegend (juce::Graphics& g, const juce::String& text, Point<float> knob, float radius)
    {
        Engraving::text (g, text, { knob.x, knob.y + radius + knobLabelGap }, labelSize, true);
    }

    //==========================================================================
    // Chassis surface
    void paintBlotches (juce::Graphics& g)
    {
        // Yellowing is uneven: a coarse random field scaled up smoothly.
        constexpr int cols = 9, rows = 12;
        juce::Image field (juce::Image::ARGB, cols, rows, true);
        juce::Random rng (917);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x)
                field.setPixelAt (x, y, juce::Colour (0xffb08a46).withAlpha (rng.nextFloat() * 0.16f));

        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (field, 0, 0, (int) D::width, (int) D::height, 0, 0, cols, rows);
    }

    // Paint rubbed off along an edge: pale rubbed glints and a few chips showing the darker primer.
    void paintEdgeWear (juce::Graphics& g, Rectangle<float> area, int seed, float density)
    {
        juce::Random rng (seed);
        const auto perimeter = 2.0f * (area.getWidth() + area.getHeight());
        const auto count = (int) (perimeter * density);
        for (int i = 0; i < count; ++i)
        {
            auto d = rng.nextFloat() * perimeter;
            Point<float> p;
            if (d < area.getWidth())                              p = { area.getX() + d, area.getY() };
            else if ((d -= area.getWidth()) < area.getHeight())   p = { area.getRight(), area.getY() + d };
            else if ((d -= area.getHeight()) < area.getWidth())   p = { area.getRight() - d, area.getBottom() };
            else                                                  p = { area.getX(), area.getBottom() - (d - area.getWidth()) };

            const auto inward = std::pow (rng.nextFloat(), 2.5f) * 7.0f;
            const auto toCentre = area.getCentre() - p;
            const auto centre = p + toCentre * (inward / juce::jmax (1.0f, toCentre.getDistanceFromOrigin()));
            const auto size = 0.8f + rng.nextFloat() * 2.2f;
            const auto isChip = rng.nextFloat() < 0.35f;
            g.setColour (isChip ? Theme::shade.darker (0.5f).withAlpha (0.28f) : Theme::creamLight.withAlpha (0.35f));
            g.fillEllipse (Rectangle<float> (size * (isChip ? 1.0f : 1.8f), size).withCentre (centre));
        }
    }

    void paintChassis (juce::Graphics& g)
    {
        const auto all = Rectangle<float> (D::width, D::height);
        g.setGradientFill (juce::ColourGradient (Theme::creamLight.interpolatedWith (Theme::cream, 0.5f), 0.0f, 0.0f,
                                                 Theme::cream.interpolatedWith (juce::Colour (0xffc6a96f), 0.5f), D::width, D::height, false));
        g.fillRect (all);
        paintBlotches (g);

        // Light from the upper left, and darker corners where the case is shadowed.
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), 0.0f, 0.0f, juce::Colours::transparentWhite, D::width * 0.7f, D::height * 0.5f, false));
        g.fillRect (all);
        juce::ColourGradient vignette (juce::Colours::transparentBlack, all.getCentre(), Theme::shade.darker (0.6f).withAlpha (0.28f), all.getTopLeft(), true);
        vignette.addColour (0.65, juce::Colours::transparentBlack);
        g.setGradientFill (vignette);
        g.fillRect (all);

        g.setFillType (juce::FillType (Textures::orangePeel(), juce::AffineTransform::scale (textureScale)));
        g.fillRect (all);

        paintEdgeWear (g, all.reduced (1.0f), 31, 0.18f);
    }

    void paintPlate (juce::Graphics& g, Rectangle<float> plate, int seed)
    {
        const auto shape = rounded (plate, 0.0f);
        // Cast shadow of the cream case lip onto the module, then the painted metal.
        g.setGradientFill (juce::ColourGradient::vertical (Theme::metalGrayLight, plate.getY(), Theme::metalGrayDark.interpolatedWith (Theme::metalGray, 0.45f), plate.getBottom()));
        g.fillPath (shape);
        fillTexture (g, shape, Textures::paintedMetal());

        g.setGradientFill (juce::ColourGradient::vertical (Theme::shade.darker (0.7f).withAlpha (0.55f), plate.getY(), juce::Colours::transparentBlack, plate.getY() + 14.0f));
        g.fillRect (plate.withHeight (14.0f));

        g.setColour (juce::Colour (0xff17130d));
        g.fillRect (plate.getX(), plate.getY(), plate.getWidth(), 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillRect (plate.getX(), plate.getY() + 2.5f, plate.getWidth(), 1.0f);

        paintEdgeWear (g, plate.reduced (1.0f, 3.0f), seed, 0.06f);
    }

    //==========================================================================
    void paintBadge (juce::Graphics& g)
    {
        const auto plate = D::badge;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (plate.expanded (3.0f).translated (1.0f, 2.0f), 5.0f);
        g.setGradientFill (Hardware::chromeGradient (plate.expanded (3.0f)));
        g.fillRoundedRectangle (plate.expanded (3.0f), 5.0f);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff1e1e1e), plate.getY(), juce::Colour (0xff050505), plate.getBottom()));
        g.fillRoundedRectangle (plate, 3.0f);

        // recessed glass window with the lit butterfly, then the engraved wordmark beside it
        const auto window = juce::Rectangle<float> (plate.getHeight() - 6.0f, plate.getHeight() - 6.0f)
                                .withCentre ({ plate.getX() + plate.getHeight() * 0.5f, plate.getCentreY() });
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff010505), window.getY(), juce::Colour (0xff0b1618), window.getBottom()));
        g.fillRoundedRectangle (window, 4.0f);
        {
            juce::Graphics::ScopedSaveState state (g);
            juce::Path clip;
            clip.addRoundedRectangle (window, 4.0f);
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (Brand::mark(), window.expanded (9.0f), juce::RectanglePlacement::centred);
        }
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (window.translated (0.0f, 0.5f), 4.0f, 0.8f);

        constexpr int logoLetters = 7;   // "OSCILLA" is chrome, "FX" is lit phosphor
        const juce::String word ("OSCILLAFX");
        juce::GlyphArrangement glyphs;
        const juce::Font font (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 40.0f, juce::Font::bold)
                                   .withHorizontalScale (1.4f)
                                   .withKerningFactor (0.16f));
        glyphs.addLineOfText (font, word, 0.0f, 0.0f);
        juce::Path chrome, lit, all;
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
            glyphs.getGlyph (i).createPath (i < logoLetters ? chrome : lit);
        all.addPath (chrome);
        all.addPath (lit);
        const auto textArea = juce::Rectangle<float> (window.getRight() + 8.0f, plate.getY(), plate.getRight() - window.getRight() - 16.0f, plate.getHeight()).reduced (0.0f, 10.0f);
        const auto fit = all.getTransformToScaleToFit (textArea, true);
        chrome.applyTransform (fit);
        lit.applyTransform (fit);

        g.setColour (juce::Colours::black);
        g.fillPath (chrome, juce::AffineTransform::translation (1.2f, 1.6f));
        g.setGradientFill (Hardware::chromeGradient (chrome.getBounds()));
        g.fillPath (chrome);

        for (const auto [radius, alpha] : { std::pair { 3.0f, 0.18f }, std::pair { 1.5f, 0.30f } })
            juce::DropShadow (Theme::glassTeal.withAlpha (alpha), (int) radius, {}).drawForPath (g, lit);
        g.setColour (Theme::glassTeal);
        g.fillPath (lit);
    }

    void paintModelTitle (juce::Graphics& g)
    {
        Engraving::text (g, "MODEL OS-1", D::modelTitle, 22.0f, true);
        Engraving::text (g, "SYSTEM AUDIO ENHANCER", D::modelTitle.translated (0.0f, 22.0f), 12.0f);
        Engraving::text (g, "OUTPUT DEVICE", { D::devicePlate.getX() - 12.0f, D::devicePlate.getCentreY() }, 12.0f, true, juce::Justification::right);
    }

    //==========================================================================
    // The tube: a deep, rounded pocket in a putty frame with a black rubber gasket around the glass.
    void paintTubeFrame (juce::Graphics& g)
    {
        const auto frame = rounded (D::bezel, 30.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 14, { 3, 7 }).drawForPath (g, frame);

        g.setGradientFill (juce::ColourGradient (Theme::putty, D::bezel.getTopLeft(), Theme::puttyBody.darker (0.06f), D::bezel.getBottomRight(), false));
        g.fillPath (frame);
        fillTexture (g, frame, Textures::orangePeel());
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.strokePath (rounded (D::bezel.reduced (1.2f), 29.0f), juce::PathStrokeType (1.4f));
        g.setColour (Theme::shade.darker (0.3f).withAlpha (0.45f));
        g.strokePath (rounded (D::bezel.reduced (0.6f), 29.5f), juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.8f, 1.2f));

        // The pocket: dark, with the shadow of its upper-left rim falling inside.
        const auto pocketArea = D::glass.expanded (32.0f);
        const auto pocket = rounded (pocketArea, 40.0f);
        g.setColour (juce::Colour (0xff12100c));
        g.fillPath (pocket);
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (pocket);
            juce::Path outside;
            outside.addRectangle (D::bezel.expanded (80.0f));
            outside.addPath (pocket);
            outside.setUsingNonZeroWinding (false);
            juce::DropShadow (juce::Colours::black.withAlpha (0.9f), 16, { 5, 7 }).drawForPath (g, outside);
        }
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.strokePath (rounded (pocketArea.expanded (1.0f), 41.0f), juce::PathStrokeType (1.2f), juce::AffineTransform::translation (0.0f, 1.0f));

        // Black rubber gasket, lit from the upper left.
        const auto gasketOuter = D::glass.expanded (17.0f), gasketInner = D::glass.expanded (2.0f);
        juce::Path gasket = rounded (gasketOuter, D::glassCorner + 17.0f);
        gasket.addRoundedRectangle (gasketInner, D::glassCorner + 2.0f);
        gasket.setUsingNonZeroWinding (false);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3a38), gasketOuter.getTopLeft(), juce::Colour (0xff060606), gasketOuter.getBottomRight(), false));
        g.fillPath (gasket);
        fillTexture (g, gasket, Textures::rubberGrain());
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.strokePath (rounded (gasketOuter.reduced (0.8f), D::glassCorner + 16.0f), juce::PathStrokeType (1.1f));

        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (D::glass.expanded (2.0f), D::glassCorner); // tube mask; the CRT paints over this

        const auto screws = D::bezel.reduced (21.0f);
        for (auto p : { screws.getTopLeft(), screws.getTopRight(), screws.getBottomLeft(), screws.getBottomRight() })
            Hardware::bezelScrew (g, p, 8.0f);

        paintEdgeWear (g, D::bezel.reduced (2.0f), 77, 0.16f);
    }

    //==========================================================================
    void paintMaster (juce::Graphics& g)
    {
        boxWithHeader (g, D::masterBox, "MASTER");
        Engraving::text (g, "POWER", { D::powerLed.x, D::powerLed.y + 25.0f }, labelSize - 1.0f, true);

        paintKnobScale (g, D::volumeKnob, D::volumeRadius, 9,
                        { { 0.0f, "-12" }, { 0.25f, "-6" }, { 0.5f, "0" }, { 0.75f, "+6" }, { 1.0f, "+12" } }, true);
        Engraving::text (g, "dB", D::volumeKnob.translated (0.0f, D::volumeRadius + 18.0f), scaleTextSize);
        knobLegend (g, "VOLUME", D::volumeKnob, D::volumeRadius + 10.0f);
        Engraving::text (g, "VOLUME LIMIT", D::limitLegend, labelSize - 1.0f, true);
        const auto limitLeft = Fader::capCentre (D::limitFader, Fader::Orientation::horizontal, 0.0f);
        const auto limitRight = Fader::capCentre (D::limitFader, Fader::Orientation::horizontal, 1.0f);
        Engraving::text (g, "-24", { limitLeft, D::limitFader.getBottom() + 8.0f }, 11.0f, true);
        Engraving::text (g, "OFF", { limitRight, D::limitFader.getBottom() + 8.0f }, 11.0f, true);
    }

    void paintScreenSwitchHeader (juce::Graphics& g)
    {
        Engraving::text (g, "SCREEN SHOWS", { D::screenSwitch.getX(), D::screenSwitch.getY() - 11.0f }, 12.5f, true, juce::Justification::left);
    }

    void paintLevel (juce::Graphics& g)
    {
        boxWithHeader (g, D::levelBox, "OUTPUT LEVEL");
        Engraving::text (g, "how loud the sound is going out", { D::levelBox.getRight() - 16.0f, D::levelBox.getY() + 20.0f }, 12.5f, false, juce::Justification::right);
    }

    void paintEffects (juce::Graphics& g)
    {
        boxWithHeader (g, D::effectsBox, "EFFECTS");
        const auto toneX = D::effectsBox.getX() + 16.0f + D::effectColumnWidth * 4.0f;
        Engraving::line (g, { toneX, D::effectsBox.getY() + 14.0f }, { toneX, D::effectsBox.getBottom() - 14.0f }, 1.2f);
        Engraving::text (g, "TONE", { toneX + 16.0f, D::effectsBox.getY() + 20.0f }, headerSize, true, juce::Justification::left);

        struct Label { const char* name; const char* plain; };
        const Label labels[] = { { "FIDELITY", "clearer detail" }, { "AMBIENCE", "roomy space" }, { "DYNAMIC BOOST", "lifts quiet parts" },
                                 { "WIDER SOUND", "(2 speakers)" }, { "BASS", "heavier thump" } };
        for (int i = 0; i < 5; ++i)
        {
            const auto bounds = D::effectFader (i);
            paintVerticalScale (g, bounds, 10, { { 0.0f, "0" }, { 0.5f, "5" }, { 1.0f, "10" } }, true);
            Engraving::text (g, labels[i].name, { bounds.getCentreX(), D::effectLabelY }, labelSize - 1.0f, true);
            Engraving::text (g, labels[i].plain, { bounds.getCentreX(), D::effectLabelY + 16.0f }, 12.0f);
        }

        paintHorizontalScale (g, D::balanceFader, 10);
        const auto left = Fader::capCentre (D::balanceFader, Fader::Orientation::horizontal, 0.0f);
        const auto right = Fader::capCentre (D::balanceFader, Fader::Orientation::horizontal, 1.0f);
        Engraving::text (g, "L", { left, D::balanceFader.getCentreY() + 22.0f }, labelSize, true);
        Engraving::text (g, "R", { right, D::balanceFader.getCentreY() + 22.0f }, labelSize, true);
        Engraving::text (g, "BALANCE", { D::balanceFader.getCentreX(), D::effectLabelY }, labelSize - 1.0f, true);
        Engraving::text (g, "left / right", { D::balanceFader.getCentreX(), D::effectLabelY + 16.0f }, 12.0f);
    }

    void paintEqualizer (juce::Graphics& g)
    {
        boxWithHeader (g, D::eqBox, "EQUALIZER");
        Engraving::text (g, "each slider boosts or cuts one slice of the sound, deep bass on the left to treble on the right",
                         { D::eqBox.getRight() - 16.0f, D::eqBox.getY() + 20.0f }, 12.5f, false, juce::Justification::right);

        for (int band = 0; band < DspSettings::numEqBands; ++band)
            paintVerticalScale (g, D::eqFader (band), 8, { { 0.0f, "-12" }, { 0.5f, "0" }, { 1.0f, "+12" } }, band == 0);

        Engraving::text (g, "dB", { 66.0f, D::eqFaderTop + D::eqFaderHeight * 0.5f + 26.0f }, scaleTextSize);
        Engraving::text (g, "HZ", { 66.0f, D::eqLabelY }, scaleTextSize + 0.5f, true);
    }
}

void paint (juce::Graphics& g)
{
    paintChassis (g);
    paintBadge (g);
    paintModelTitle (g);

    boxWithHeader (g, D::displayBox, "DISPLAY");
    boxWithHeader (g, D::presetBox, "PRESET");
    paintScreenSwitchHeader (g);
    paintMaster (g);
    paintLevel (g);
    paintTubeFrame (g);

    paintPlate (g, D::effectsPlate, 41);
    paintPlate (g, D::eqPlate, 43);
    paintEffects (g);
    paintEqualizer (g);
}
}
