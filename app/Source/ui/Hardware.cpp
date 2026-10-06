#include "ui/Hardware.h"
#include "ui/Textures.h"
#include "ui/Theme.h"

namespace Hardware
{
namespace
{
    using juce::Colour;
    using juce::Point;
    using juce::Rectangle;

    constexpr float lightAngle = -2.36f; // upper left, radians in screen space (atan2)
    constexpr int knurlCount = 56;

    Rectangle<float> circle (Point<float> c, float r) { return { c.x - r, c.y - r, r * 2.0f, r * 2.0f }; }

    void softShadow (juce::Graphics& g, Point<float> c, float r, Point<float> offset, float alpha)
    {
        const auto centre = c + offset;
        juce::ColourGradient grad (juce::Colours::black.withAlpha (alpha), centre,
                                   juce::Colours::transparentBlack, centre.translated (r * 1.3f, 0.0f), true);
        grad.addColour (0.68, juce::Colours::black.withAlpha (alpha * 0.6f));
        g.setGradientFill (grad);
        g.fillEllipse (circle (centre, r * 1.3f));
    }

    juce::ColourGradient litDisc (Point<float> c, float r, Colour light, Colour dark)
    {
        return { light, c.translated (-r * 0.55f, -r * 0.6f), dark, c.translated (r * 0.7f, r * 0.8f), false };
    }

    float facing (float angle) { return 0.5f + 0.5f * std::cos (angle - lightAngle); }

    Point<float> along (float angleFromNoon) { return { std::sin (angleFromNoon), -std::cos (angleFromNoon) }; }

    void knurledSkirt (juce::Graphics& g, Point<float> c, float r)
    {
        g.setGradientFill (litDisc (c, r, juce::Colour (0xff3a3a3a), juce::Colour (0xff030303)));
        g.fillEllipse (circle (c, r));

        const auto inner = r * 0.8f;
        for (int i = 0; i < knurlCount; ++i)
        {
            const auto a = juce::MathConstants<float>::twoPi * (float) i / (float) knurlCount;
            const auto dir = Point<float> (std::cos (a), std::sin (a));
            const auto side = Point<float> (-dir.y, dir.x) * (r * 0.022f);
            const auto lit = facing (a);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawLine ({ c + dir * inner - side, c + dir * r * 0.985f - side }, r * 0.03f);
            g.setColour (juce::Colours::white.withAlpha (0.04f + 0.22f * lit * lit));
            g.drawLine ({ c + dir * inner + side, c + dir * r * 0.985f + side }, r * 0.02f);
        }

        g.setColour (juce::Colours::black);
        g.drawEllipse (circle (c, r), r * 0.03f);

        // smooth chamfer between the knurl and the cap
        g.setGradientFill (litDisc (c, inner, juce::Colour (0xff4a4a4a), juce::Colour (0xff050505)));
        g.fillEllipse (circle (c, inner));
        g.setGradientFill (litDisc (c, inner * 0.92f, juce::Colour (0xff1d1d1d), juce::Colour (0xff0b0b0b)));
        g.fillEllipse (circle (c, inner * 0.92f));
    }

    void barPointer (juce::Graphics& g, Point<float> c, float r, float angle)
    {
        const auto width = r * 0.46f;
        const Rectangle<float> bar (-width * 0.5f, -r * 1.24f, width, r * 1.9f);
        const auto toPanel = juce::AffineTransform::rotation (angle).translated (c);

        juce::Path body;
        body.addRoundedRectangle (bar, width * 0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (body, toPanel.translated (r * 0.07f, r * 0.12f));

        juce::ColourGradient grad (juce::Colour (0xff3c3c3c), bar.getX(), 0.0f, juce::Colour (0xff050505), bar.getRight(), 0.0f, false);
        grad.addColour (0.3, juce::Colour (0xff222222));
        grad.point1 = grad.point1.transformedBy (toPanel);
        grad.point2 = grad.point2.transformedBy (toPanel);
        g.setGradientFill (grad);
        g.fillPath (body, toPanel);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.strokePath (body, juce::PathStrokeType (r * 0.02f), toPanel);
    }

    void indexLine (juce::Graphics& g, Point<float> c, float from, float to, float angle, float thickness)
    {
        const auto dir = along (angle);
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawLine ({ c + dir * from, c + dir * to }, thickness);
    }

    void spunCap (juce::Graphics& g, Point<float> c, float r)
    {
        const auto texture = Textures::spunAluminium();
        {
            juce::Graphics::ScopedSaveState state (g);
            juce::Path clip;
            clip.addEllipse (circle (c, r));
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.setOpacity (1.0f); // images take the alpha of the last colour set
            g.drawImage (texture, circle (c, r), juce::RectanglePlacement::stretchToFit);
        }

        juce::ColourGradient rim (juce::Colours::transparentBlack, c, juce::Colours::black.withAlpha (0.45f), c.translated (r, 0.0f), true);
        rim.addColour (0.82, juce::Colours::transparentBlack);
        g.setGradientFill (rim);
        g.fillEllipse (circle (c, r));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (circle (c.translated (-r * 0.02f, -r * 0.03f), r * 0.97f), r * 0.03f);
    }
}

juce::ColourGradient chromeGradient (Rectangle<float> a)
{
    juce::ColourGradient grad (Theme::chromeLight, a.getTopLeft(), Theme::chromeDark, a.getBottomRight(), false);
    grad.addColour (0.45, juce::Colour (0xffb9babb));
    grad.addColour (0.55, juce::Colour (0xffe7e7e5));
    return grad;
}

void knob (juce::Graphics& g, Point<float> c, const KnobStyle& style, float angle)
{
    const auto r = style.radius;
    softShadow (g, c, r, { r * 0.1f, r * 0.18f }, 0.55f);
    knurledSkirt (g, c, r);

    if (style.isSelector)
    {
        barPointer (g, c, r, angle);
        spunCap (g, c, r * 0.5f);
        indexLine (g, c, r * 0.6f, r * 1.14f, angle, r * 0.07f);
        return;
    }

    spunCap (g, c, r * 0.62f);
    indexLine (g, c, r * 0.66f, r * 0.95f, angle, r * 0.075f);
}

void led (juce::Graphics& g, Point<float> c, float r, float glow, Colour colour)
{
    if (glow > 0.01f)
    {
        juce::ColourGradient halo (colour.withAlpha (0.6f * glow), c, colour.withAlpha (0.0f), c.translated (r * 3.4f, 0.0f), true);
        halo.addColour (0.3, colour.withAlpha (0.28f * glow));
        g.setGradientFill (halo);
        g.fillEllipse (circle (c, r * 3.4f));
    }

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (circle (c, r * 1.22f));

    const auto lit = colour.darker (0.85f).interpolatedWith (colour, glow);
    juce::ColourGradient dome (lit.brighter (0.25f + 0.6f * glow), c.translated (-r * 0.3f, -r * 0.35f),
                               lit.darker (0.7f - 0.4f * glow), c.translated (r * 0.8f, r * 0.9f), true);
    g.setGradientFill (dome);
    g.fillEllipse (circle (c, r));

    if (glow > 0.01f)
    {
        juce::ColourGradient core (colour.brighter (0.8f).withAlpha (0.85f * glow), c, juce::Colours::transparentWhite, c.translated (r * 0.6f, 0.0f), true);
        g.setGradientFill (core);
        g.fillEllipse (circle (c, r * 0.6f));
    }

    g.setColour (juce::Colours::white.withAlpha (0.4f + 0.3f * glow));
    g.fillEllipse (circle (c.translated (-r * 0.35f, -r * 0.42f), r * 0.24f));
}

void bezelScrew (juce::Graphics& g, Point<float> c, float r)
{
    softShadow (g, c, r, { r * 0.08f, r * 0.15f }, 0.7f);
    g.setGradientFill (litDisc (c, r, juce::Colour (0xff4b4b4b), juce::Colour (0xff050505)));
    g.fillEllipse (circle (c, r));
    g.setColour (juce::Colours::black);
    g.drawEllipse (circle (c, r), r * 0.08f);

    const auto rot = juce::AffineTransform::rotation (0.5f).translated (c);
    juce::Path slot;
    slot.addRectangle (-r * 0.66f, -r * 0.12f, r * 1.32f, r * 0.24f);
    slot.addRectangle (-r * 0.12f, -r * 0.66f, r * 0.24f, r * 1.32f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillPath (slot, rot.translated (r * 0.05f, r * 0.08f));
    g.setColour (juce::Colours::black);
    g.fillPath (slot, rot);
}

void chromeScrew (juce::Graphics& g, Point<float> c, float r)
{
    softShadow (g, c, r, { r * 0.1f, r * 0.15f }, 0.5f);
    g.setGradientFill (chromeGradient (circle (c, r)));
    g.fillEllipse (circle (c, r));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (circle (c, r), r * 0.08f);

    const auto rot = juce::AffineTransform::rotation (0.3f).translated (c);
    juce::Path cross;
    cross.addRectangle (-r * 0.6f, -r * 0.11f, r * 1.2f, r * 0.22f);
    cross.addRectangle (-r * 0.11f, -r * 0.6f, r * 0.22f, r * 1.2f);
    g.setColour (juce::Colour (0xff3a3a3a));
    g.fillPath (cross, rot);
}

void slideSwitch (juce::Graphics& g, Rectangle<float> slot, float position, bool isHighlighted)
{
    const auto isHorizontal = slot.getWidth() > slot.getHeight();
    const auto corner = juce::jmin (slot.getWidth(), slot.getHeight()) * 0.18f;

    g.setColour (Theme::inkLip.withAlpha (0.6f));
    g.fillRoundedRectangle (slot.translated (0.0f, 1.2f), corner);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff020202), slot.getY(), juce::Colour (0xff2a2a2a), slot.getBottom()));
    g.fillRoundedRectangle (slot, corner);

    const auto inner = slot.reduced (2.5f);
    auto actuator = isHorizontal ? inner.withWidth (inner.getWidth() * 0.55f) : inner.withHeight (inner.getHeight() * 0.55f);
    if (isHorizontal)
        actuator.setX (inner.getX() + (inner.getWidth() - actuator.getWidth()) * position);
    else
        actuator.setY (inner.getY() + (inner.getHeight() - actuator.getHeight()) * position);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (actuator.translated (1.0f, 1.5f), corner * 0.8f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4a), actuator.getTopLeft(),
                                             juce::Colour (0xff0a0a0a), actuator.getBottomRight(), false));
    g.fillRoundedRectangle (actuator, corner * 0.8f);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    for (int ridge = 1; ridge <= 4; ++ridge)
    {
        const auto t = (float) ridge / 5.0f;
        if (isHorizontal)
        {
            const auto x = actuator.getX() + actuator.getWidth() * t;
            g.drawLine (x, actuator.getY() + 3.0f, x, actuator.getBottom() - 3.0f, 1.2f);
        }
        else
        {
            const auto y = actuator.getY() + actuator.getHeight() * t;
            g.drawLine (actuator.getX() + 3.0f, y, actuator.getRight() - 3.0f, y, 1.2f);
        }
    }

    g.setColour (juce::Colours::white.withAlpha (isHighlighted ? 0.28f : 0.16f));
    g.drawRoundedRectangle (actuator.reduced (0.6f), corner * 0.8f, 1.0f);
}

void creamKey (juce::Graphics& g, Rectangle<float> area, bool isDown, bool isHighlighted, bool isEnabled, float lampGlow)
{
    g.setColour (Theme::inkLip.withAlpha (0.4f));
    g.fillRoundedRectangle (area.translated (0.0f, 1.0f), 4.0f);
    g.setColour (Theme::legendInk);
    g.fillRoundedRectangle (area, 4.0f);

    const auto cap = area.reduced (2.5f).translated (0.0f, isDown ? 1.0f : 0.0f);
    auto top = isHighlighted && isEnabled ? Theme::creamLight : Theme::putty;
    auto bottom = Theme::puttyBody.darker (isDown ? 0.25f : 0.0f);
    if (! isEnabled)
    {
        top = top.interpolatedWith (Theme::cream, 0.6f);
        bottom = bottom.interpolatedWith (Theme::cream, 0.6f);
    }
    g.setGradientFill (juce::ColourGradient::vertical (isDown ? top.darker (0.15f) : top, cap.getY(), bottom, cap.getBottom()));
    g.fillRoundedRectangle (cap, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (isDown ? 0.2f : 0.55f));
    g.drawLine (cap.getX() + 2.5f, cap.getY() + 0.8f, cap.getRight() - 2.5f, cap.getY() + 0.8f, 1.0f);
    g.setColour (Theme::shade.withAlpha (0.55f));
    g.drawRoundedRectangle (cap.reduced (0.5f), 2.5f, 0.8f);

    if (lampGlow >= 0.0f)
        led (g, { cap.getRight() - 7.0f, cap.getY() + 7.0f }, 2.6f, isEnabled ? lampGlow : 0.0f, Theme::lampGreen);
}

void recessedPlate (juce::Graphics& g, Rectangle<float> area, bool isHighlighted)
{
    g.setColour (Theme::inkLip.withAlpha (0.6f));
    g.fillRoundedRectangle (area.translated (0.0f, 1.2f), 4.0f);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff000000), area.getY(),
                                                       isHighlighted ? juce::Colour (0xff2e2e2e) : juce::Colour (0xff1c1c1c),
                                                       area.getBottom()));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (area.reduced (0.5f), 4.0f, 1.0f);
}

void faderSlot (juce::Graphics& g, Rectangle<float> slot, bool isVertical, float litFrom, float litTo, float glow)
{
    const auto corner = juce::jmin (slot.getWidth(), slot.getHeight()) * 0.5f;
    g.setColour (Theme::inkLip.withAlpha (0.55f));
    g.fillRoundedRectangle (slot.translated (0.8f, 1.2f), corner);
    g.setGradientFill (isVertical ? juce::ColourGradient (juce::Colour (0xff000000), slot.getX(), 0.0f, juce::Colour (0xff2c2a26), slot.getRight(), 0.0f, false)
                                  : juce::ColourGradient::vertical (juce::Colour (0xff000000), slot.getY(), juce::Colour (0xff2c2a26), slot.getBottom()));
    g.fillRoundedRectangle (slot, corner);

    if (glow <= 0.0f || litTo <= litFrom)
        return;

    const auto lit = isVertical ? Rectangle<float> (slot.getCentreX() - 1.0f, slot.getY() + 2.0f + (slot.getHeight() - 4.0f) * (1.0f - litTo), 2.0f, (slot.getHeight() - 4.0f) * (litTo - litFrom))
                                : Rectangle<float> (slot.getX() + 2.0f + (slot.getWidth() - 4.0f) * litFrom, slot.getCentreY() - 1.0f, (slot.getWidth() - 4.0f) * (litTo - litFrom), 2.0f);
    g.setColour (Theme::lampGreen.withAlpha (0.18f * glow));
    g.fillRoundedRectangle (lit.expanded (2.0f), 3.0f);
    g.setColour (Theme::lampGreen.withAlpha (0.9f * glow));
    g.fillRoundedRectangle (lit, 1.0f);
}

void faderCap (juce::Graphics& g, Rectangle<float> cap, bool isVertical, bool isHighlighted, bool isDown)
{
    constexpr float corner = 2.5f;
    g.setColour (juce::Colours::black.withAlpha (isDown ? 0.28f : 0.38f));
    g.fillRoundedRectangle (cap.translated (isDown ? 1.5f : 3.0f, isDown ? 2.0f : 4.0f), corner + 1.0f);

    const auto lift = isHighlighted ? 0.08f : 0.0f;
    auto grad = isVertical ? juce::ColourGradient::vertical (juce::Colour (0xfff4f4f2).brighter (lift), cap.getY(), juce::Colour (0xff8f9092), cap.getBottom())
                           : juce::ColourGradient (juce::Colour (0xfff4f4f2).brighter (lift), cap.getX(), 0.0f, juce::Colour (0xff8f9092), cap.getRight(), 0.0f, false);
    grad.addColour (0.3, juce::Colour (0xffd9dadb).brighter (lift));
    grad.addColour (0.58, juce::Colour (0xffb4b5b7));
    grad.addColour (0.72, juce::Colour (0xffc9cacb));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (cap, corner);

    // Fine brushing along the travel.
    juce::Random rng (cap.getWidth() > cap.getHeight() ? 5 : 7);
    const auto brushes = isVertical ? (int) cap.getWidth() : (int) cap.getHeight();
    for (int i = 1; i < brushes; i += 2)
    {
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + 0.05f * rng.nextFloat()));
        if (isVertical)
            g.drawLine (cap.getX() + (float) i, cap.getY() + 2.0f, cap.getX() + (float) i, cap.getBottom() - 2.0f, 0.8f);
        else
            g.drawLine (cap.getX() + 2.0f, cap.getY() + (float) i, cap.getRight() - 2.0f, cap.getY() + (float) i, 0.8f);
    }

    // Grip grooves either side of a dark index line across the middle.
    const auto centre = cap.getCentre();
    const auto along = isVertical ? Point<float> (0.0f, 1.0f) : Point<float> (1.0f, 0.0f);
    const auto across = isVertical ? Point<float> (1.0f, 0.0f) : Point<float> (0.0f, 1.0f);
    const auto halfWidth = (isVertical ? cap.getWidth() : cap.getHeight()) * 0.5f - 3.5f;
    for (int i = -2; i <= 2; ++i)
    {
        const auto at = centre + along * ((float) i * 3.2f);
        const auto isIndex = i == 0;
        g.setColour (isIndex ? juce::Colours::black.withAlpha (0.85f) : juce::Colours::black.withAlpha (0.28f));
        g.drawLine ({ at - across * halfWidth, at + across * halfWidth }, isIndex ? 1.6f : 1.0f);
        if (! isIndex)
        {
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawLine ({ at - across * halfWidth + along, at + across * halfWidth + along }, 0.8f);
        }
    }

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (cap.reduced (0.5f), corner, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawLine (cap.getX() + 2.5f, cap.getY() + 1.2f, cap.getRight() - 2.5f, cap.getY() + 1.2f, 1.0f);
}

void focusRing (juce::Graphics& g, Rectangle<float> area, float cornerSize)
{
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawRoundedRectangle (area.expanded (1.5f), cornerSize + 1.5f, 4.0f);
    g.setColour (juce::Colour (0xff0a6d66));
    g.drawRoundedRectangle (area, cornerSize, 2.2f);
}
}
