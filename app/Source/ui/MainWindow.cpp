#include "ui/MainWindow.h"
#include "ui/DockReopen.h"
#include "ui/Theme.h"
#include "ui/WindowChrome.h"

namespace
{
    namespace D = Theme::Design;

    constexpr float preferredScale = 0.76f;
    constexpr float minimumScale = 0.58f;   // below this the engraved legends become unreadable
    constexpr float maximumScale = 1.5f;
    constexpr int screenMargin = 60;

    int heightForWidth (int width) { return juce::roundToInt ((float) width * D::height / D::width); }
    int widthForHeight (int height) { return juce::roundToInt ((float) height * D::width / D::height); }
}

void MainWindow::ScaledHost::resized()
{
    // Letterbox as a fallback: programmatic sizes do not pass through the constrainer.
    const auto scale = juce::jmin ((float) getWidth() / D::width, (float) getHeight() / D::height);
    const auto x = ((float) getWidth() - D::width * scale) * 0.5f;
    const auto y = ((float) getHeight() - D::height * scale) * 0.5f;
    panel.setTransform (juce::AffineTransform::scale (scale).translated (x, y));
}

void MainWindow::ScaledHost::paint (juce::Graphics& g)
{
    g.fillAll (Theme::desk);
}

//==============================================================================
int MainWindow::PanelConstrainer::titleBarHeight() const
{
    if (auto* peer = window.getPeer())
        if (const auto frame = peer->getFrameSizeIfPresent())
            return frame->getTop();
    return 0;
}

void MainWindow::PanelConstrainer::checkBounds (juce::Rectangle<int>& bounds, const juce::Rectangle<int>&, const juce::Rectangle<int>& limits,
                                                bool isStretchingTop, bool isStretchingLeft, bool isStretchingBottom, bool isStretchingRight)
{
    const auto titleBar = titleBarHeight();
    const auto isOnlyVertical = (isStretchingTop || isStretchingBottom) && ! (isStretchingLeft || isStretchingRight);
    const auto proposedWidth = isOnlyVertical ? widthForHeight (bounds.getHeight() - titleBar) : bounds.getWidth();
    const auto widestOnScreen = limits.isEmpty() ? getMaximumWidth() : widthForHeight (limits.getHeight() - titleBar);
    const auto maxWidth = juce::jmax (getMinimumWidth(), juce::jmin (getMaximumWidth(), widestOnScreen));
    const auto width = juce::jlimit (getMinimumWidth(), maxWidth, proposedWidth);
    const auto height = heightForWidth (width) + titleBar;

    if (isStretchingLeft)
        bounds.setLeft (bounds.getRight() - width);
    else
        bounds.setWidth (width);

    if (isStretchingTop)
        bounds.setTop (bounds.getBottom() - height);
    else
        bounds.setHeight (height);
}

//==============================================================================
MainWindow::MainWindow (Controller& controller)
    : juce::DocumentWindow ("OscillaFX", Theme::desk, juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton),
      host (controller)
{
    setUsingNativeTitleBar (true);
    if (auto* peer = getPeer())
        WindowChrome::useFullSizeContent (*peer);
    setContentNonOwned (&host, false);

    auto scale = preferredScale;
    if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = juce::jlimit (minimumScale, preferredScale, (float) (display->userArea.getHeight() - screenMargin) / D::height);

    constrainer.setSizeLimits ((int) (D::width * minimumScale), heightForWidth ((int) (D::width * minimumScale)),
                               (int) (D::width * maximumScale), heightForWidth ((int) (D::width * maximumScale)));
    setConstrainer (&constrainer);
    setResizable (true, false);

    centreWithSize (juce::roundToInt (D::width * scale), heightForWidth (juce::roundToInt (D::width * scale)));
    DockReopen::setHandler ([this] { present(); });
}

void MainWindow::addToDesktop (int windowStyleFlags, void* nativeWindowToAttachTo)
{
    juce::DocumentWindow::addToDesktop (windowStyleFlags, nativeWindowToAttachTo);
    if (auto* peer = getPeer())
        WindowChrome::useFullSizeContent (*peer);   // every time JUCE recreates the native window
}

MainWindow::~MainWindow()
{
    DockReopen::setHandler (nullptr);
    setConstrainer (nullptr);
}

void MainWindow::visibilityChanged()
{
    juce::DocumentWindow::visibilityChanged();
    if (auto* peer = getPeer())
        WindowChrome::useFullSizeContent (*peer);
}

void MainWindow::closeButtonPressed()
{
    setVisible (false); // keeps running in the menu bar; the Dock icon or menu bar item brings it back
}

void MainWindow::present()
{
    if (isMinimised())
        setMinimised (false);
    setVisible (true);
    toFront (true);
    juce::Process::makeForegroundProcess();
}

void MainWindow::showTour()
{
    host.showTour();
}
