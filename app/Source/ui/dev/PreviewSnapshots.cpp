#include "ui/dev/PreviewSnapshots.h"
#include "ui/CrtDisplay.h"
#include "ui/MainPanel.h"
#include "ui/Theme.h"
#include <algorithm>

#ifndef OSCILLA_UI_STUB
 #define OSCILLA_UI_STUB 0
#endif

namespace PreviewSnapshots
{
#if OSCILLA_UI_STUB
namespace
{
    constexpr int settleMs = 1500;   // CRT warm-up and phosphor persistence
    constexpr int rebuildMs = 300;   // MainPanel rebuilds its cached art this long after a scale change
    const char* const silentSignal = "OSCILLA_STUB_SILENT";   // read by ControllerStub on every frame
    const char* const micDenied = "OSCILLA_STUB_MIC_DENIED";  // read by MicrophoneAccessStub on every call

    struct Step
    {
        juce::String name;
        std::function<void()> setup;
        std::function<void()> beforeCapture = [] {};   // for transient state such as the value read-out
        int settle = settleMs;                          // shorter for transient animations (the power-on splash)
    };

    juce::Component* findByTitle (juce::Component& root, const juce::String& title)
    {
        if (root.getTitle() == title)
            return &root;
        for (auto* child : root.getChildren())
            if (auto* found = findByTitle (*child, title))
                return found;
        return nullptr;
    }

    template <typename Type>
    Type* findFirst (juce::Component& root)
    {
        if (auto* match = dynamic_cast<Type*> (&root))
            return match;
        for (auto* child : root.getChildren())
            if (auto* found = findFirst<Type> (*child))
                return found;
        return nullptr;
    }

    // The CRT may render into a Metal layer that component snapshots cannot see: paint its frame in.
    juce::Image snapshot (juce::Component& content, float scale)
    {
        auto image = content.createComponentSnapshot (content.getLocalBounds(), true, scale);
        if (auto* crt = findFirst<CrtDisplay> (content))
        {
            const auto screen = crt->renderedScreen();
            if (screen.isValid())
            {
                juce::Graphics g (image);
                const auto area = content.getLocalArea (crt, crt->getLocalBounds()).toFloat() * scale;
                g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
                g.drawImage (screen, area, juce::RectanglePlacement::stretchToFit);
            }
        }
        return image;
    }

    void write (const juce::Image& image, const juce::File& file)
    {
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
    }

    // The CRT alone, as rendered (GPU read-back), for the steps that show something new on it.
    void saveTubeFace (juce::Component& content, const juce::File& dir, const juce::String& stepName)
    {
        static const juce::StringArray crtSteps { "spectrum", "wave", "xy", "gallery", "gallery-rat", "eq", "no-signal", "power-off" };
        const auto name = stepName.fromFirstOccurrenceOf ("panel-", false, false);
        auto* crt = findFirst<CrtDisplay> (content);
        if (crt == nullptr || ! crtSteps.contains (name))
            return;
        if (const auto screen = crt->renderedScreen(); screen.isValid())
            write (screen, dir.getChildFile ("crt-" + name + ".png"));
    }

    // Each scale is captured twice: the first capture makes the panel rebuild its art at that scale.
    void save (juce::Component* content, juce::File dir, const Step& step, float scale, std::function<void()> done)
    {
        step.beforeCapture();
        snapshot (*content, scale);
        juce::Timer::callAfterDelay (rebuildMs, [=]
        {
            write (snapshot (*content, scale), dir.getChildFile (step.name + (scale > 1.0f ? "@2x.png" : ".png")));
            if (scale > 1.0f)
                saveTubeFace (*content, dir, step.name);
            done();
        });
    }

    void run (std::shared_ptr<std::vector<Step>> steps, size_t index, juce::Component* content, juce::File dir)
    {
        if (index >= steps->size())
        {
            juce::JUCEApplicationBase::quit();
            return;
        }
        const auto& step = (*steps)[index];
        unsetenv (silentSignal); // only the no-signal step silences the stub
        unsetenv (micDenied);    // only the mic-denied step denies microphone access
        step.setup();
        juce::Timer::callAfterDelay (step.settle, [=]
        {
            save (content, dir, (*steps)[index], 1.0f, [=]
            {
                save (content, dir, (*steps)[index], 2.0f, [=] { run (steps, index + 1, content, dir); });
            });
        });
    }

    std::vector<Step> makeSteps (Controller& controller, juce::Component& root)
    {
        auto* c = &controller;
        auto* r = &root;
        const auto settings = [c] (std::function<void (DspSettings&)> edit) { return [c, edit] { c->update (edit); }; };
        const auto mode = [settings] (int m) { return settings ([m] (DspSettings& s) { s.power = true; s.eqEditMode = false; s.displayMode = m; s.nightMode = false; s.limiterCeilingDb = 0.0f; }); };
        const auto resizeTo = [r] (float scale)
        {
            return [r, scale]
            {
                if (auto* window = r->getTopLevelComponent())
                    window->setSize (juce::roundToInt (Theme::Design::width * scale), juce::roundToInt (Theme::Design::height * scale));
            };
        };

        return {
            { "panel-spectrum", mode (0) },
            { "panel-wave", mode (1) },
            { "panel-xy", mode (2) },
            { "panel-gallery", [mode, settings] { mode (3)(); settings ([] (DspSettings& s) { s.galleryIndex = 0; })(); } },
            { "panel-gallery-rat", settings ([] (DspSettings& s) { s.galleryIndex = 3; }) },
            { "panel-eq", settings ([] (DspSettings& s) { s.displayMode = 0; s.eqEditMode = true; }) },
            { "panel-night", settings ([] (DspSettings& s) { s.power = true; s.eqEditMode = false; s.displayMode = 0; s.nightMode = true; s.limiterCeilingDb = -12.0f; }) },
            { "panel-gallery-controls", [mode, settings]
              {
                  mode (3)();
                  settings ([] (DspSettings& s) { s.galleryIndex = 1; })();
              } },
            { "panel-suggestion", [c, mode]
              {
                  mode (0)();
                  c->selectDevice ("stub-airpods");   // a device without a profile: OscillaFX offers starter sounds
              } },
            { "panel-tour", [c, r]
              {
                  c->dismissSuggestion (true);
                  c->selectDevice ("stub-speakers");
                  if (auto* panel = findFirst<MainPanel> (*r))
                      panel->showTour();
              } },
            { "panel-readout", [r]
              {
                  if (auto* panel = findFirst<MainPanel> (*r))
                      panel->hideTour();
              }, [r]
              {
                  if (auto* volume = dynamic_cast<juce::Slider*> (findByTitle (*r, "Volume")))
                  {
                      volume->setValue (4.0, juce::dontSendNotification);
                      volume->setValue (4.5, juce::sendNotificationSync); // a user change: pops up the read-out
                  }
              } },
            { "panel-focus", [r]
              {
                  if (auto* power = findByTitle (*r, "Power"))
                  {
                      power->grabKeyboardFocus();
                      power->moveKeyboardFocusToSibling (true); // Tab: Volume shows its focus ring
                  }
              } },
            { "panel-no-signal", [mode]
              {
                  setenv (silentSignal, "1", 1);
                  mode (1)();
              } },
            { "panel-mic-denied", [settings]
              {
                  setenv (micDenied, "1", 1);
                  settings ([] (DspSettings& s) { s.power = true; s.displayMode = 0; })(); // change message: panel re-reads the state
              } },
            { "panel-power-off", settings ([] (DspSettings& s) { s.eqEditMode = false; s.power = false; }) },
            { "brand-splash", [settings]
              {
                  settings ([] (DspSettings& s) { s.power = false; s.eqEditMode = false; s.displayMode = 0; })();
                  juce::Timer::callAfterDelay (600, [settings] { settings ([] (DspSettings& s) { s.power = true; })(); });   // power-on: butterfly burns in during the warm-up
              }, [] {}, 1000 },
            { "panel-min", [settings, resizeTo]
              {
                  settings ([] (DspSettings& s) { s.power = true; s.displayMode = 1; s.nightMode = false; s.limiterCeilingDb = 0.0f; })();
                  resizeTo (0.58f)();
              } },
            { "panel-max", resizeTo (1.5f) },
        };
    }
}

void scheduleIfRequested (Controller& controller, juce::Component& content)
{
    if (juce::SystemStats::getEnvironmentVariable ("OSCILLA_DEV_CLOSE_WINDOW", {}).isNotEmpty())
        juce::Timer::callAfterDelay (settleMs, [&content]
        {
            if (auto* window = dynamic_cast<juce::DocumentWindow*> (content.getTopLevelComponent()))
                window->closeButtonPressed();
        });

    // Headless profiling: keep the CRT animating even if the window is occluded or the display sleeps.
    if (juce::SystemStats::getEnvironmentVariable ("OSCILLA_DEV_RENDER_OCCLUDED", {}).isNotEmpty())
        if (auto* crt = findFirst<CrtDisplay> (content))
            crt->setRendersWhenOccluded (true);

    const auto dirPath = juce::SystemStats::getEnvironmentVariable ("OSCILLA_SNAPSHOT_DIR", {});
    if (dirPath.isEmpty())
        return;

    const juce::File dir (dirPath);
    dir.createDirectory();
    if (auto* crt = findFirst<CrtDisplay> (content))
        crt->setRendersWhenOccluded (true); // the snapshot window may sit behind others

    if (auto* panel = findFirst<MainPanel> (content))
        panel->hideTour();
    controller.markFirstRunDone();           // no welcome tour or device card in the shots unless a step asks
    controller.dismissSuggestion (true);
    auto steps = std::make_shared<std::vector<Step>> (makeSteps (controller, content));

    // OSCILLA_SNAPSHOT_ONLY=wave,xy runs just the steps whose name contains one of the words (quick CRT iteration).
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("OSCILLA_SNAPSHOT_ONLY", {}), ",", ""); ! only.isEmpty())
        steps->erase (std::remove_if (steps->begin(), steps->end(), [&only] (const Step& step)
        {
            return std::none_of (only.begin(), only.end(), [&step] (const juce::String& word) { return step.name.contains (word); });
        }), steps->end());
    auto* root = &content;
    juce::Timer::callAfterDelay (settleMs, [=] { run (steps, 0, root, dir); });
}
#else
void scheduleIfRequested (Controller&, juce::Component&) {}
#endif
}
