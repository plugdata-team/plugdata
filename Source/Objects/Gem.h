/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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
    struct GemWindowResizeListener final : public ComponentBoundsConstrainer {
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
    GemJUCEWindow(Rectangle<int> bounds, bool const border)
    {
        instance = libpd_this_instance();

        resizeListener.beginResize = [this] {
            setThis();
            sys_lock();
            gemBeginExternalResize();
            sys_unlock();
        };

        resizeListener.endResize = [this] {
            setThis();
            sys_lock();
            gemEndExternalResize();
            initGemWindow();
            sys_unlock();
        };

        setOpaque(true);
        openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);

        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
        openGLContext.attachTo(*this);

        startTimerHz(30);

        if (border) {
#    if JUCE_WINDOWS
            addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasDropShadow | ComponentPeer::windowIsResizable);

#    else
            addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasDropShadow | ComponentPeer::windowIsResizable | ComponentPeer::windowHasMinimiseButton | ComponentPeer::windowHasMaximiseButton);
#    endif
        } else {
            addToDesktop(0);
        }

#    if JUCE_WINDOWS
        bounds = bounds.translated(10, 30);
#    endif
        setBounds(bounds);

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

        if (auto const* peer = getPeer()) {
            auto const scale = peer->getPlatformScaleFactor();
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

                heldKeys.remove_at(i);
            }
        }
    }

    void setThis()
    {
        libpd_set_instance(instance);
    }

    void checkThread()
    {
        auto const currentThread = Thread::getCurrentThreadId();
        if (activeThread != currentThread) {
            openGLContext.initialiseOnThread();
            activeThread = currentThread;
        }
    }

    OpenGLContext openGLContext;
    Thread::ThreadID activeThread;
    t_pdinstance* instance;
    SmallArray<KeyPress> heldKeys;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GemJUCEWindow)
};

void GemCallOnMessageThread(std::function<void()> callback)
{
    MessageManager::getInstance()->callFunctionOnMessageThread([](void* callback) -> void* {
        auto const& fn = *static_cast<std::function<void()>*>(callback);
        fn();

        return nullptr;
    },
        &callback);
}

UnorderedMap<t_pdinstance*, std::unique_ptr<GemJUCEWindow>> gemJUCEWindow;

bool gemWinSetCurrent()
{
    if (!gemJUCEWindow.contains(libpd_this_instance()))
        return false;

    if (auto const& window = gemJUCEWindow.at(libpd_this_instance())) {
        window->checkThread();
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

    window->checkThread();
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
        GemCallOnMessageThread([window, &info] {
            window->openGLContext.detach();
            window->removeFromDesktop();
            info.window.erase(window->instance);
            info.context.erase(window->instance);
            gemJUCEWindow[window->instance].reset(nullptr);
        });
    }
}

void initWin_sharedContext(WindowInfo& info, WindowHints& hints)
{
    if (auto const* window = info.getWindow()) {
        window->openGLContext.makeActive();
    }
}

// Rendering
void gemWinSwapBuffers(WindowInfo& info)
{
    if (auto* context = info.getContext()) {
        context->makeActive();
        context->swapBuffers();
        initGemWindow(); // This isn't as bad as it seems, it just resets the openGL state
    }
}
void gemWinMakeCurrent(WindowInfo& info)
{
    if (auto const* context = info.getContext()) {
        if (auto* window = info.getWindow()) {
            window->checkThread();
        }
        context->makeActive();
    }
}

void gemWinResize(WindowInfo& info, int width, int height)
{
    if (auto* windowPtr = info.getWindow()) {
        MessageManager::callAsync([window = Component::SafePointer(windowPtr), width, height] {
            if (auto* w = window.getComponent()) {
                w->setSize(width, height);
            }
        });
    }
}

// Window behaviour
int cursorGemWindow(WindowInfo& info, int const state)
{
    if (auto* window = info.getWindow()) {
        window->setMouseCursor(state ? MouseCursor::NormalCursor : MouseCursor::NoCursor);
    }

    return state;
}

int topmostGemWindow(WindowInfo& info, int const state)
{
    if (info.getWindow() && state)
        info.getWindow()->toFront(true);
    return state;
}

void titleGemWindow(WindowInfo& info, const char* title)
{
    if (auto* window = info.getWindow()) {
        MessageManager::callAsync([window = Component::SafePointer(window), title = String::fromUTF8(title)] {
            if(window) {
                if (auto* peer = window->getPeer()) {
                    peer->setTitle(title);
                }
            }
        });
    }
}

#endif
