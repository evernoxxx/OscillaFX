#pragma once

#include "ui/MainPanel.h"

// Fixed-aspect, freely scalable window with no title-bar strip: the panel fills it edge to edge and
// the window buttons float over its top-left corner (Theme::Design::windowButtons stays free).
// Closing it only hides it; the app lives on in the menu bar.
class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow (Controller&);
    ~MainWindow() override;

    void addToDesktop (int windowStyleFlags, void* nativeWindowToAttachTo = nullptr) override;

    void visibilityChanged() override;
    void closeButtonPressed() override;
    void present();   // show, un-minimise and focus
    void showTour();  // the first-run walk-through again

private:
    class ScaledHost : public juce::Component
    {
    public:
        explicit ScaledHost (Controller& c) : panel (c) { addAndMakeVisible (panel); }
        void showTour() { panel.showTour(); }
        void resized() override;
        void paint (juce::Graphics&) override;

    private:
        MainPanel panel;
    };

    // macOS hands the constrainer the whole frame, title bar included: keep the *content* at the
    // panel's aspect ratio at every size, so the panel is never letterboxed.
    class PanelConstrainer : public juce::ComponentBoundsConstrainer
    {
    public:
        explicit PanelConstrainer (juce::Component& w) : window (w) {}
        void checkBounds (juce::Rectangle<int>& bounds, const juce::Rectangle<int>& previous, const juce::Rectangle<int>& limits,
                          bool isStretchingTop, bool isStretchingLeft, bool isStretchingBottom, bool isStretchingRight) override;

    private:
        int titleBarHeight() const;
        juce::Component& window;
    };

    ScaledHost host;
    PanelConstrainer constrainer { *this };
};
