/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#if ENABLE_GEM

#    include <juce_opengl/juce_opengl.h>
#    include <Gem/src/Base/GemJUCEContext.h>

void triggerMotionEvent(int x, int y);
void triggerButtonEvent(int which, int state, int x, int y);
void triggerWheelEvent(int axis, int value);
void triggerKeyboardEvent(char const* string, int value, int state);
void triggerResizeEvent(int xSize, int ySize);

void initGemWindow();
void gemBeginExternalResize();
void gemEndExternalResize();

class GemJUCEWindow final : public Component
    , public Timer {
    // Use a constrainer as a resize listener!
    struct GemWindowResizeListener : public ComponentBoundsConstrainer {
        std::function<void()> beginResize, endResize;

        GemWindowResizeListener()
        {
            setSizeLimits(50, 50, 30000, 30000);
        }

        void resizeStart() override
        {
            beginResize();
        }

        void resizeEnd() override
        {
            endResize();
        }
    };

    GemWindowResizeListener resizeListener;

public:
    //==============================================================================
    GemJUCEWindow(Rectangle<int> bounds, bool border)
    {
        instance = libpd_this_instance();

        resizeListener.beginResize = [this]() {
            setThis();
            sys_lock();
            gemBeginExternalResize();
            sys_unlock();
        };

        resizeListener.endResize = [this]() {
            setThis();
            sys_lock();
            gemEndExternalResize();
            initGemWindow();
            sys_unlock();
        };

        setBounds(bounds);

        setOpaque(true);
        openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);

        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
        openGLContext.attachTo(*this);

        startTimerHz(30);

        if (border) {
            addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasDropShadow | ComponentPeer::windowIsResizable | ComponentPeer::windowHasMinimiseButton | ComponentPeer::windowHasMaximiseButton);
        } else {
            addToDesktop(0);
        }

        setVisible(true);

        if (auto* peer = getPeer()) {
            // Attach the constrainer to the peer
            // Constrainer doesn't actually contrain, it's just the simplest way to get a callback if the native window is being resized
            // The reason for this is that rendering while resizing causes issues, especially on macOS
            peer->setConstrainer(&resizeListener);
        }
    }

    ~GemJUCEWindow() override
    {
    }

    void resized() override
    {
        auto w = getWidth();
        auto h = getHeight();

        if (auto* peer = getPeer()) {
            auto scale = peer->getPlatformScaleFactor();
            w *= scale;
            h *= scale;
        }

        setThis();
        triggerResizeEvent(w, h);
    }

    void paint(Graphics&) override
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        setThis();
        triggerButtonEvent(e.mods.isRightButtonDown(), 1, e.x, e.y);
    }

    void mouseUp(MouseEvent const& e) override
    {
        setThis();
        triggerButtonEvent(e.mods.isRightButtonDown(), 0, e.x, e.y);
    }

    void mouseMove(MouseEvent const& e) override
    {
        setThis();
        triggerMotionEvent(e.x, e.y);
    }
    void mouseDrag(MouseEvent const& e) override
    {
        setThis();
        triggerMotionEvent(e.x, e.y);
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        setThis();
        triggerWheelEvent(wheel.deltaX, wheel.deltaY);
    }

    bool keyPressed(KeyPress const& key) override
    {
        triggerKeyboardEvent(key.getTextDescription().toRawUTF8(), key.getKeyCode(), 1);

        heldKeys.add(key);

        return false;
    }

    void timerCallback() override
    {
        for (int i = heldKeys.size() - 1; i >= 0; i--) {
            auto key = heldKeys[i];
            if (!KeyPress::isKeyCurrentlyDown(key.getKeyCode())) {
                sys_lock();
                triggerKeyboardEvent(key.getTextDescription().toRawUTF8(), key.getKeyCode(), 0);
                sys_unlock();

                heldKeys.remove(i);
            }
        }
    }

    void setThis()
    {
        libpd_set_instance(instance);
    }

    OpenGLContext openGLContext;
    t_pdinstance* instance;
    Array<KeyPress> heldKeys;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GemJUCEWindow)
};

void GemCallOnMessageThread(std::function<void()> callback)
{
    MessageManager::getInstance()->callFunctionOnMessageThread([](void* callback) -> void* {
        auto& fn = *reinterpret_cast<std::function<void()>*>(callback);
        fn();

        return nullptr;
    },
        (void*)&callback);
}

std::map<t_pdinstance*, std::unique_ptr<GemJUCEWindow>> gemJUCEWindow;

bool gemWinSetCurrent()
{
    if (!gemJUCEWindow.contains(libpd_this_instance()))
        return false;

    if (auto& window = gemJUCEWindow.at(libpd_this_instance())) {
        window->openGLContext.makeActive();
        return true;
    }

    return false;
}

void gemWinUnsetCurrent()
{
    OpenGLContext::deactivateCurrentContext();
}

// window/context creation&destruction
bool initGemWin()
{
    return true;
}

int createGemWindow(WindowInfo& info, WindowHints& hints)
{
    auto* window = new GemJUCEWindow({ hints.x_offset, hints.y_offset, hints.width, hints.height }, hints.border);
    gemJUCEWindow[window->instance].reset(window);
    info.window[window->instance] = window;


    window->openGLContext.makeActive();

    info.context[window->instance] = &window->openGLContext;

    hints.real_w = window->getWidth();
    hints.real_h = window->getHeight();

    if (auto* peer = window->getPeer()) {
        if (hints.title) {
            peer->setTitle(String::fromUTF8(hints.title));
        }
        if (hints.fullscreen) {
            peer->setFullScreen(hints.fullscreen);
        }

        auto const& displays = Desktop::getInstance().getDisplays().displays;
        if (hints.secondscreen && displays.size() >= 2) {
            // Move window to second screen
            window->setTopLeftPosition(displays[1].userArea.getPosition() + window->getPosition());
        }
    }

    return 1;
}
void destroyGemWindow(WindowInfo& info)
{
    if (auto* window = info.getWindow()) {
        GemCallOnMessageThread([window, &info]() {
            window->removeFromDesktop();
            window->openGLContext.detach();
            info.window.erase(window->instance);
            info.context.erase(window->instance);
            gemJUCEWindow[window->instance].reset(nullptr);
        });
    }
}

void initWin_sharedContext(WindowInfo& info, WindowHints& hints)
{
    if (auto* window = info.getWindow()) {
        window->openGLContext.makeActive();
    }
}

// Rendering
void gemWinSwapBuffers(WindowInfo& info)
{
    if (auto* context = info.getContext()) {
        context->makeActive();
        context->swapBuffers();
        initGemWindow(); // If we don't put this here, the background doens't get filled, but there must be a better way?
    }
}
void gemWinMakeCurrent(WindowInfo& info)
{
    if (auto* context = info.getContext()) {
        context->initialiseOnThread();
        context->makeActive();
    }
}

void gemWinResize(WindowInfo& info, int width, int height)
{
    if (auto* windowPtr = info.getWindow()) {
        MessageManager::callAsync([window = Component::SafePointer(windowPtr), width, height]() {
            if (auto* w = window.getComponent()) {
                w->setSize(width, height);
            }
        });
    }
}

// Window behaviour
int cursorGemWindow(WindowInfo& info, int state)
{
    if (auto* window = info.getWindow()) {
        window->setMouseCursor(state ? MouseCursor::NormalCursor : MouseCursor::NoCursor);
    }

    return state;
}

int topmostGemWindow(WindowInfo& info, int state)
{
    if (info.getWindow() && state)
        info.getWindow()->toFront(true);
    return state;
}
#endif
