/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_opengl/juce_opengl.h>
#include <Gem/src/Base/GemJUCEContext.h>

void triggerMotionEvent(int x, int y);
void triggerButtonEvent(int which, int state, int x, int y);
void triggerWheelEvent(int axis, int value);
void triggerKeyboardEvent(const char *string, int value, int state);
void triggerResizeEvent(int xSize, int ySize);

void initGemWindow();

class GemJUCEWindow final : public Component
{
public:
    //==============================================================================
    GemJUCEWindow()
    {
        setSize (800, 600);
        
        setOpaque (true);
        openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);
        
        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
        
        openGLContext.attachTo (*this);
        
        instance = libpd_this_instance();
    }

    ~GemJUCEWindow() override
    {
    }

    void resized() override
    {
        triggerResizeEvent(getWidth(), getHeight());
    }
    
    void paint (Graphics&) override
    {
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        triggerButtonEvent(e.mods.isRightButtonDown(), 1, e.x, e.y);
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        triggerButtonEvent(e.mods.isRightButtonDown(), 0, e.x, e.y);
    }
    
    void mouseMove(const MouseEvent& e) override
    {
        triggerMotionEvent(e.x, e.y);
    }
    void mouseDrag(const MouseEvent& e) override
    {
        triggerMotionEvent(e.x, e.y);
    }
    
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
    {
        triggerWheelEvent(wheel.deltaX, wheel.deltaY);
    }
    
    bool keyPressed(KeyPress const& key) override
    {
        //triggerKeyboardEvent(key.getTextDescription().toRawUTF8(), key.getKeyCode(), 1);
        return false;
    }
    
    OpenGLContext openGLContext;
    t_pdinstance* instance;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GemJUCEWindow)
};

void GemCallOnMessageThread(std::function<void()> callback)
{
    //MessageManager::callAsync(callback);
    MessageManager::getInstance()->callFunctionOnMessageThread([](void* callback) -> void* {
        auto& fn = *reinterpret_cast<std::function<void()>*>(callback);
        fn();
        
        return nullptr;
    }, (void*)&callback);
}

// window/context creation&destruction
bool initGemWin(void)
{
  return true;
}

std::map<t_pdinstance*, std::unique_ptr<GemJUCEWindow>> gemJUCEWindow;

int createGemWindow(WindowInfo& info, WindowHints& hints)
{
    // Problem: this call can either come from the message thread, or the pd/audio thread
    // As a result, on Linux, we need to make sure that only one thread has the GL context set as active, otherwise things go very wrong
    
    auto* window = new GemJUCEWindow();
    window->addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowIsResizable |      ComponentPeer::windowHasDropShadow);
    window->setVisible(true);
      
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
        initGemWindow();
        
        context->makeActive();
        context->swapBuffers();
    }
}
void gemWinMakeCurrent(WindowInfo& info) {
    if (auto* context = info.getContext()) {
        context->makeActive();
    }
}

void gemWinResize(WindowInfo& info, int width, int height) {
    MessageManager::callAsync([window = info.getWindow(), width, height](){
        if(window) window->setSize(width, height);
    });
}

bool gemWinMakeCurrent() {
    if(!gemJUCEWindow.contains(libpd_this_instance())) return false;
        
    if(auto& window = gemJUCEWindow.at(libpd_this_instance())) {
        window->openGLContext.makeActive();
        return true;
    }
    
    return false;
}

// We handle this manually with JUCE
void dispatchGemWindowMessages(WindowInfo& info) {
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
