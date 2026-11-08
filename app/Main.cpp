// Midi4u App: a tray app that mixes Ableton's music (from Midi4u Tap) with the mic and
// sends the mix to the virtual cable that Discord listens to.

#include "AppController.h"
#include "MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
juce::Image drawTrayIcon(juce::Colour colour)
{
    juce::Image image(juce::Image::ARGB, 64, 64, true);
    juce::Graphics g(image);
    g.setColour(colour);
    g.fillEllipse(6.0f, 6.0f, 52.0f, 52.0f);
    g.setColour(colour.contrasting(1.0f));
    g.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    g.drawText("M", image.getBounds(), juce::Justification::centred);
    return image;
}

class TrayIcon final : public juce::SystemTrayIconComponent
{
public:
    TrayIcon(std::function<void()> show, std::function<void()> quit) : show_(std::move(show)), quit_(std::move(quit))
    {
        setIconImage(drawTrayIcon(juce::Colour(0xff3ccf6e)), drawTrayIcon(juce::Colours::black));
        setIconTooltip("Midi4u");
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
#if JUCE_MAC
        juce::ignoreUnused(event);
        showMenu(); // menu bar items open a menu on any click
#else
        if (event.mods.isPopupMenu())
            showMenu();
        else
            show_();
#endif
    }

private:
    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem("Show Midi4u", show_);
        menu.addSeparator();
        menu.addItem("Quit Midi4u", quit_);
#if JUCE_MAC
        showDropdownMenu(menu);
#else
        menu.showMenuAsync({});
#endif
    }

    std::function<void()> show_, quit_;
};

class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow(AppController& controller)
        : DocumentWindow("Midi4u", juce::Colour(0xff26282e), DocumentWindow::closeButton | DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(controller), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
    }

    // Closing the window keeps Midi4u running in the tray.
    void closeButtonPressed() override { setVisible(false); }
};

class Midi4uApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& commandLine) override
    {
        controller_ = std::make_unique<AppController>();
        window_ = std::make_unique<MainWindow>(*controller_);
        tray_ = std::make_unique<TrayIcon>([this] { showWindow(); }, [this] { systemRequestedQuit(); });

        if (! commandLine.contains("--tray")) // started with Windows: stay in the tray
            showWindow();
    }

    void shutdown() override
    {
        tray_.reset();
        window_.reset();
        controller_.reset();
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String&) override { showWindow(); }

private:
    void showWindow()
    {
        window_->setVisible(true);
        window_->setMinimised(false);
        window_->toFront(true);
    }

    std::unique_ptr<AppController> controller_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<TrayIcon> tray_;
};
} // namespace

START_JUCE_APPLICATION(Midi4uApplication)
