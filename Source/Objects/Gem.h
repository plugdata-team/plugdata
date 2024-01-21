/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#if ENABLE_GEM

#include <juce_opengl/juce_opengl.h>
#include <Gem/src/Base/GemJUCEContext.h>

void triggerMotionEvent(int x, int y);
void triggerButtonEvent(int which, int state, int x, int y);
void triggerWheelEvent(int axis, int value);
void triggerKeyboardEvent(const char *string, int value, int state);
void triggerResizeEvent(int xSize, int ySize);

void initGemWindow();
void gemBeginExternalResize();
void gemEndExternalResize();

class GemJUCEWindow final : public Component, public Timer
{
    // Use a constrainer as a resize listener!
    struct GemWindowResizeListener : public ComponentBoundsConstrainer
    {
        std::function<void()> beginResize, endResize;

        GemWindowResizeListener() {
          setSizeLimits (50, 50, 30000, 30000);
        }
        
        void resizeStart()
        {
            beginResize();
        }

        void resizeEnd()
        {
            endResize();
        }
    };

    GemWindowResizeListener resizeListener;
    
public:
    //==============================================================================
    GemJUCEWindow(int width, int height) : gemWidth(width), gemHeight(height)
    {
        instance = libpd_this_instance();
        
        resizeListener.beginResize = [this](){
            setThis();
            sys_lock();
            gemBeginExternalResize();
            sys_unlock();
        };
        
        resizeListener.endResize = [this](){
            setThis();
            sys_lock();
            gemEndExternalResize();
            initGemWindow();
            sys_unlock();
        };
        
        setSize(width, height);
        
        setOpaque (true);
        openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);
        
        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
        
        openGLContext.attachTo (*this);
        
        startTimerHz(30);
        
        addToDesktop(ComponentPeer::windowHasTitleBar |
                     ComponentPeer::windowHasDropShadow |
                     ComponentPeer::windowIsResizable |
                     ComponentPeer::windowHasMinimiseButton |
                     ComponentPeer::windowHasMaximiseButton
                     );
        
        setVisible(true);
        setTopLeftPosition(100, 100);
        
        if(auto* peer = getPeer())
        {
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
        setThis();

        // TODO: this sometimes causes a deadlock
        sys_lock();
        triggerResizeEvent(getWidth(), getHeight());
        sys_unlock();
    }
    
    void paint (Graphics&) override
    {
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        setThis();
        sys_lock();
        triggerButtonEvent(e.mods.isRightButtonDown(), 1, e.x, e.y);
        sys_unlock();
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        setThis();
        sys_lock();
        triggerButtonEvent(e.mods.isRightButtonDown(), 0, e.x, e.y);
        sys_unlock();
    }
    
    void mouseMove(const MouseEvent& e) override
    {
        setThis();
        sys_lock();
        triggerMotionEvent(e.x, e.y);
        sys_unlock();
    }
    void mouseDrag(const MouseEvent& e) override
    {
        setThis();
        sys_lock();
        triggerMotionEvent(e.x, e.y);
        sys_unlock();
    }
    
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
    {
        setThis();
        sys_lock();
        triggerWheelEvent(wheel.deltaX, wheel.deltaY);
        sys_unlock();
    }
    
    bool keyPressed(KeyPress const& key) override
    {
        sys_lock();
        triggerKeyboardEvent(key.getTextDescription().toRawUTF8(), key.getKeyCode(), 1);
        sys_unlock();
        
        heldKeys.add(key);
        
        return false;
    }
    
    void timerCallback() override
    {
        for(int i = heldKeys.size() - 1; i >= 0; i--)
        {
            auto key = heldKeys[i];
            if(!KeyPress::isKeyCurrentlyDown(key.getKeyCode()))
            {
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
    
    
    int gemWidth, gemHeight;
    OpenGLContext openGLContext;
    t_pdinstance* instance;
    Array<KeyPress> heldKeys;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GemJUCEWindow)
};

void GemCallOnMessageThread(std::function<void()> callback)
{
    MessageManager::getInstance()->callFunctionOnMessageThread([](void* callback) -> void* {
        auto& fn = *reinterpret_cast<std::function<void()>*>(callback);
        fn();
        
        return nullptr;
    }, (void*)&callback);
}

std::map<t_pdinstance*, std::unique_ptr<GemJUCEWindow>> gemJUCEWindow;

bool gemWinSetCurrent() {
    if(!gemJUCEWindow.contains(libpd_this_instance())) return false;
        
    if(auto& window = gemJUCEWindow.at(libpd_this_instance())) {
        window->openGLContext.makeActive();
        return true;
    }
        
    return false;
}

void gemWinUnsetCurrent() {
    OpenGLContext::deactivateCurrentContext();
}

// window/context creation&destruction
bool initGemWin(void)
{
    return true;
}

int createGemWindow(WindowInfo& info, WindowHints& hints)
{
    auto* window = new GemJUCEWindow(hints.width, hints.height);
    gemJUCEWindow[window->instance].reset(window);
    info.window[window->instance] = window;
    
    #if JUCE_LINUX
    // Make sure only audio thread has the context set as active
    window->openGLContext.executeOnGLThread([](OpenGLContext& context){
        // We get unpredictable behaviour if the context is active on multiple threads
        OpenGLContext::deactivateCurrentContext();
        }, true);
    #endif

    window->openGLContext.makeActive();
    
    info.context[window->instance] = &window->openGLContext;
    
    hints.real_w = window->getWidth();
    hints.real_h = window->getHeight();
        
    auto* peer = window->getPeer();
    if(peer && hints.title)
    {
        window->setTitle(String::fromUTF8(hints.title));
    }
    
    // Make sure only audio thread has the context set as active
    // We call async here, because if this call comes from the message thread already,
    // we need to keep the context active until GLEW is initialised. Bit of a hack though
    MessageManager::callAsync([](){
      OpenGLContext::deactivateCurrentContext();
    });
    
    return 1;
}
void destroyGemWindow(WindowInfo& info) {
    if(auto* window = info.getWindow()) {
        window->removeFromDesktop();
        info.window.erase(window->instance);
        info.context.erase(window->instance);
        gemJUCEWindow[window->instance].reset(nullptr);
    }
}

void initWin_sharedContext(WindowInfo& info, WindowHints& hints) {
    if(auto* window = info.getWindow())  {
        window->openGLContext.makeActive();
    }
}

// Rendering
void gemWinSwapBuffers(WindowInfo& info) {
    if (auto* context = info.getContext()) {
        context->makeActive();
        context->swapBuffers();
        initGemWindow(); // If we don't put this here, the background doens't get filled, but there must be a better way?
    }
}
void gemWinMakeCurrent(WindowInfo& info) {
    if (auto* context = info.getContext()) {
        context->makeActive();
    }
}

void gemWinResize(WindowInfo& info, int width, int height) {
    if(auto* windowPtr = info.getWindow()) {
        MessageManager::callAsync([window = Component::SafePointer(info.getWindow()), width, height](){
            if(auto* w = window.getComponent())  {
                w->setSize(width, height);
                w->gemHeight = height;
                w->gemWidth = width;
            }
        });
    }
}


// Window behaviour
int cursorGemWindow(WindowInfo& info, int state)
{
    if(auto* window = info.getWindow()) {
        window->setMouseCursor(state ? MouseCursor::NormalCursor : MouseCursor::NoCursor);
    }
    
  return state;
}

int topmostGemWindow(WindowInfo& info, int state)
{
  if(info.getWindow() && state) info.getWindow()->toFront(true);
  return state;
}
#endif
