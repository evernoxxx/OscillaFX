#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Palette and design-space geometry for the instrument panel (aged cream plastic and painted
// metal, after a Tektronix 7613). Colours are in docs/DESIGN_SWATCHES.md. All layout is expressed in a
// fixed 1000 x 1280 design space and scaled as a whole by the window. Light comes from the upper left.
namespace Theme
{
    inline const juce::Colour desk           { 0xff1c1a18 };   // window background before the first paint
    inline const juce::Colour cream          { 0xffdac38f };
    inline const juce::Colour creamLight     { 0xffe9dcbb };
    inline const juce::Colour putty          { 0xffe6d3a5 };
    inline const juce::Colour puttyBody      { 0xffcdb88c };
    inline const juce::Colour shade          { 0xff9b8559 };
    inline const juce::Colour metalGray      { 0xffc9c7ba };
    inline const juce::Colour metalGrayLight { 0xffe3ddcc };
    inline const juce::Colour metalGrayDark  { 0xffaaa392 };
    inline const juce::Colour ink            { 0xff17120b };
    inline const juce::Colour legendInk      { 0xff1c1710 };
    inline const juce::Colour inkLip         { 0xfff1e6c8 };   // faint light catch beside printed strokes
    inline const juce::Colour bakelite       { 0xff121212 };
    inline const juce::Colour bakeliteLight  { 0xff5a5a5a };
    inline const juce::Colour rubber         { 0xff141414 };
    inline const juce::Colour chromeLight    { 0xfff2f2f0 };
    inline const juce::Colour chromeDark     { 0xff6c6d6f };
    inline const juce::Colour lampGreen      { 0xff4cf29a };
    inline const juce::Colour lampGreenDeep  { 0xff12803f };
    inline const juce::Colour ledRed         { 0xffff2b1c };
    inline const juce::Colour alertAmber     { 0xffffa21f };
    inline const juce::Colour glassTeal      { 0xff4be3d6 };   // panel read-outs and tooltips on dark glass
    inline const juce::Colour glassTealHot   { 0xffc9fffa };
    inline const juce::Colour darkGlass      { 0xf0040a0a };

    // Used by the CRT drawing code.
    inline const juce::Colour phosphor       { 0xff39ff88 };
    inline const juce::Colour phosphorHot    { 0xffd2ffe2 };
    inline const juce::Colour screenCentre   { 0xff0b2418 };

    namespace Design
    {
        constexpr float width = 1000.0f;
        constexpr float height = 1280.0f;

        // Top-left corner kept free for the macOS window buttons (they float over the panel).
        // At the smallest window scale (0.58) these are 72 x 36 points.
        inline const juce::Rectangle<float> windowButtons { 0.0f, 0.0f, 124.0f, 62.0f };
        inline const juce::Rectangle<float> badge         { 136.0f, 10.0f, 206.0f, 42.0f };
        inline const juce::Point<float>     modelTitle    { 470.0f, 26.0f };
        inline const juce::Rectangle<float> devicePlate   { 716.0f, 14.0f, 270.0f, 34.0f };

        // Row 1: master and level meters (on the cream faceplate)
        inline const juce::Rectangle<float> masterBox { 14.0f, 66.0f, 340.0f, 244.0f };
        inline const juce::Rectangle<float> levelBox  { 366.0f, 66.0f, 620.0f, 244.0f };

        // Row 2: tube and its side column
        inline const juce::Rectangle<float> bezel      { 14.0f, 322.0f, 590.0f, 440.0f };
        inline const juce::Rectangle<float> glass      { 81.0f, 371.0f, 456.0f, 342.0f };
        constexpr float glassCorner = 26.0f;
        inline const juce::Rectangle<float> micAlert   { 205.0f, 733.0f, 260.0f, 26.0f };
        inline const juce::Rectangle<float> galleryAdd       { 632.0f, 526.0f, 104.0f, 24.0f };   // gallery display only, same band
        inline const juce::Rectangle<float> galleryRemove    { 742.0f, 526.0f, 82.0f, 24.0f };
        inline const juce::Rectangle<float> gallerySlideshow { 830.0f, 526.0f, 140.0f, 24.0f };
        inline const juce::Rectangle<float> suggestionCard   { 536.0f, 52.0f, 450.0f, 139.0f };
        inline const juce::Rectangle<float> displayBox { 616.0f, 322.0f, 370.0f, 232.0f };
        inline const juce::Rectangle<float> presetBox  { 616.0f, 566.0f, 370.0f, 196.0f };

        // Rows 3 and 4: painted-metal modules, edge to edge
        inline const juce::Rectangle<float> effectsPlate { 0.0f, 768.0f, 1000.0f, 238.0f };
        inline const juce::Rectangle<float> eqPlate      { 0.0f, 1010.0f, 1000.0f, 270.0f };
        inline const juce::Rectangle<float> effectsBox   { 14.0f, 774.0f, 972.0f, 226.0f };
        inline const juce::Rectangle<float> eqBox        { 14.0f, 1016.0f, 972.0f, 258.0f };

        constexpr float knobSweep = 2.4f;   // radians either side of 12 o'clock, like a real pot

        // MASTER
        constexpr float volumeRadius = 50.0f;
        inline const juce::Point<float> volumeKnob   { 122.0f, 196.0f };
        inline const juce::Point<float> powerLed     { 284.0f, 102.0f };
        inline const juce::Point<float> powerSwitch  { 284.0f, 152.0f };
        inline const juce::Rectangle<float> nightKey { 224.0f, 178.0f, 120.0f, 62.0f };
        inline const juce::Point<float> limitLegend  { 284.0f, 250.0f };
        inline const juce::Rectangle<float> limitFader { 219.0f, 258.0f, 130.0f, 36.0f };

        // LEVEL
        inline const juce::Rectangle<float> vuLeft  { 380.0f, 98.0f, 286.0f, 196.0f };
        inline const juce::Rectangle<float> vuRight { 686.0f, 98.0f, 286.0f, 196.0f };

        // DISPLAY
        inline const juce::Point<float> displaySelector { 801.0f, 416.0f };
        inline const juce::Rectangle<float> screenSwitch { 632.0f, 472.0f, 170.0f, 50.0f };
        inline const juce::Rectangle<float> nextImage    { 850.0f, 448.0f, 120.0f, 72.0f };

        // PRESET
        inline const juce::Point<float> presetSelector { 801.0f, 650.0f };
        inline const juce::Rectangle<float> presetPrev  { 632.0f, 694.0f, 36.0f, 28.0f };
        inline const juce::Rectangle<float> presetPlate { 674.0f, 694.0f, 256.0f, 28.0f };
        inline const juce::Rectangle<float> presetNext  { 934.0f, 694.0f, 36.0f, 28.0f };
        inline const juce::Rectangle<float> presetBlurb { 632.0f, 726.0f, 338.0f, 32.0f };

        // EFFECTS: four effect faders, bass, then balance
        constexpr float effectFaderHeight = 124.0f;
        constexpr float effectFaderWidth = 64.0f;
        constexpr float effectColumnWidth = 150.0f;
        constexpr float effectFaderTop = 808.0f;
        constexpr float effectLabelY = 968.0f;
        inline const juce::Rectangle<float> balanceCell { 790.0f, 806.0f, 190.0f, 150.0f };
        inline juce::Rectangle<float> effectFader (int column)
        {
            const auto cx = effectsBox.getX() + effectColumnWidth * ((float) column + 0.5f) + 16.0f;
            return juce::Rectangle<float> (effectFaderWidth, effectFaderHeight).withCentre ({ cx, effectFaderTop + effectFaderHeight * 0.5f });
        }
        inline const juce::Rectangle<float> balanceFader { 806.0f, 852.0f, 160.0f, 52.0f };
        constexpr float effectLcdY = 936.0f;   // top of the value read-out plates under the faders
        constexpr float lcdWidth = 52.0f, lcdHeight = 17.0f;

        // EQ: ten faders in a row, scale in the first column
        constexpr float eqFaderHeight = 150.0f;
        constexpr float eqFaderWidth = 52.0f;
        constexpr float eqFirstX = 100.0f;
        constexpr float eqColumnWidth = 90.0f;
        constexpr float eqFaderTop = 1052.0f;
        constexpr float eqLabelY = 1244.0f;
        constexpr float eqLcdY = 1208.0f;
        inline juce::Rectangle<float> eqFader (int band)
        {
            const auto cx = eqFirstX + eqColumnWidth * (float) band + 22.0f;
            return juce::Rectangle<float> (eqFaderWidth, eqFaderHeight).withCentre ({ cx, eqFaderTop + eqFaderHeight * 0.5f });
        }
    }
}
