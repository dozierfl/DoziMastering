#include "MasteringComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class App final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String&) override { window=std::make_unique<Window>(); }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

    class Window final : public juce::DocumentWindow {
    public:
        Window() : DocumentWindow("Dozi Mastering",juce::Colour(0xff101216),allButtons) {
            setUsingNativeTitleBar(true);
            content=new MasteringComponent;
            setContentOwned(content,true);
            setWantsKeyboardFocus(true);
            setResizable(true,true);
            centreWithSize(1280,960);
            setVisible(true);
            content->grabKeyboardFocus();
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
        bool keyPressed(const juce::KeyPress&key) override { return content!=nullptr&&content->handleTransportShortcut(key); }
        void activeWindowStatusChanged() override { juce::DocumentWindow::activeWindowStatusChanged();if(isActiveWindow()&&content!=nullptr)content->grabKeyboardFocus(); }
    private:
        MasteringComponent* content=nullptr;
    };

private:
    std::unique_ptr<Window> window;
};

START_JUCE_APPLICATION(App)
