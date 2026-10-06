#pragma once

#include "core/Controller.h"
#include "ui/CrtDisplay.h"
#include "ui/Fader.h"
#include "ui/HoverHints.h"
#include "ui/Knob.h"
#include "ui/PanelWidgets.h"
#include "ui/RotarySelector.h"
#include "ui/SuggestionCard.h"
#include "ui/Textures.h"
#include "ui/Theme.h"
#include "ui/TourOverlay.h"
#include "ui/VuMeter.h"

// The whole instrument front panel, laid out in Theme::Design space (scaled by the window).
// Mirrors Controller state and writes every user change back through Controller::update().
// The panel fills the window edge to edge; dragging its background moves the window.
class MainPanel : public juce::Component,
                  public juce::FileDragAndDropTarget,
                  private juce::ChangeListener,
                  private juce::Timer
{
public:
    explicit MainPanel (Controller&);
    ~MainPanel() override;

    void reloadPresetList();
    void showTour();   // the first-run walk-through; also from the menu bar
    void hideTour();

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    static constexpr int meterWindowSamples = 2048;   // about 43 ms at 48 kHz

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void configureFaders();
    void configureEqualizer();
    void configureMaster();
    void configureDisplay();
    void configurePresets();
    void addControls();
    void setFocusOrder();
    void bindSlider (juce::Slider&, float DspSettings::* field);
    void describe (Knob&, const juce::String&);
    void describe (Fader&, const juce::String&);
    void describe (RotarySelector&, const juce::String&);
    void describe (juce::Button&, const juce::String&);

    void refreshFromController();
    void refreshSliders (const DspSettings&);
    void refreshEqualizer (const DspSettings&);
    void refreshDisplay (const DspSettings&);
    void refreshPresets (const DspSettings&);
    void refreshDevice();
    void refreshMicrophoneAlert();
    void refreshReadouts (const DspSettings&);
    void announcePreset (const juce::String& presetName);
    void refreshSuggestion();
    void refreshGalleryControls (const DspSettings&);

    void updateMeters();
    void markModified();
    void stepPreset (int delta);
    void showPresetMenu();
    void showDeviceMenu();
    void showNextImage();
    void addPictures();
    void importPictures (const juce::Array<juce::File>&);
    void removeCurrentPicture();
    void showSlideshowMenu();
    void flashMessage (const juce::String&);
    juce::File currentPicture() const;

    Controller& controller;

    // Declared first so they outlive every component that draws with them.
    juce::SharedResourcePointer<PanelLookAndFeel> lookAndFeel;
    juce::SharedResourcePointer<Textures::Cache> textures;

    CrtDisplay crt;
    HoverHints hints { [this] (const juce::String& text) { crt.setHoverHint (text); } };   // before the controls it watches
    ValueReadout readout;
    AlertButton microphoneButton { "Allow microphone access" };

    PowerLed powerLed;
    SlideSwitch powerSwitch { "Power", SlideSwitch::Axis::horizontal, "OFF", "ON" };
    Knob volume { "Volume", { Theme::Design::volumeRadius, false } };
    PushButton nightKey { "Night mode", "NIGHT MODE" };
    Fader limit { "Volume limit", Fader::Orientation::horizontal };
    PlateButton deviceButton { "Output device" };

    VuMeter vuLeft { "LEFT" }, vuRight { "RIGHT" };
    Vu::Ballistics leftBallistics, rightBallistics;
    float leftPeakHold = 0.0f, rightPeakHold = 0.0f;
    double lastMeterTime = 0.0;
    std::vector<float> scopeLeft, scopeRight;
    juce::TimedCallback meterTimer { [this] { updateMeters(); } };

    RotarySelector displayMode { "Display", { 32.0f, 48.0f, 4, juce::degreesToRadians (46.0f), 90.0f, 13.0f } };
    SlideSwitch screenSwitch { "Screen shows", SlideSwitch::Axis::vertical, "SOUND", "EQ CURVE" };
    PushButton nextImage { "Next image", "NEXT IMAGE" };
    PlateButton galleryAdd { "Add images", false }, galleryRemove { "Remove image", false };
    PlateButton gallerySlideshow { "Slideshow" };

    RotarySelector presets { "Preset", { 34.0f, 56.0f, 5, juce::degreesToRadians (36.0f), 100.0f, 12.0f } };
    StepKey previousPreset { "Previous preset", false };
    PlateButton presetButton { "Preset list" };
    StepKey nextPreset { "Next preset", true };
    PrintedLabel presetBlurb { 12.0f, false, juce::Justification::centredTop, 2 };

    Fader fidelity { "Fidelity", Fader::Orientation::vertical };
    Fader ambience { "Ambience", Fader::Orientation::vertical };
    Fader dynamicBoost { "Dynamic boost", Fader::Orientation::vertical };
    Fader widerSound { "Wider sound (2 speakers)", Fader::Orientation::vertical };
    Fader bass { "Bass", Fader::Orientation::vertical };
    Fader balance { "Balance", Fader::Orientation::horizontal };

    std::vector<std::unique_ptr<LcdReadout>> effectLcds, eqLcds;   // always-visible values under the sliders
    LcdReadout balanceLcd;
    std::vector<std::unique_ptr<Fader>> eqFaders;
    std::vector<std::unique_ptr<PrintedLabel>> eqLabels;

    SuggestionCard suggestionCard;
    TourOverlay tour;
    juce::String suggestedDeviceUid;
    bool slideshowShown = false;
    int slideshowSecondsShown = 0;
    std::unique_ptr<juce::FileChooser> pictureChooser;

    juce::StringArray presetNames;
    juce::String deviceUid, loadedPresetName;
    bool isModifiedSincePreset = false;
    bool isEqEditingShown = false;
    juce::Image background;
    float backgroundScale = 0.0f;
    juce::ComponentDragger windowDragger;
    juce::TooltipWindow tooltips { nullptr, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
