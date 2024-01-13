/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_opengl/juce_opengl.h>
#include <Gem/src/Base/GemJUCEContext.h>

extern void performGemRender();
extern void initGemWindow();

void dequeueEvents(void);

void triggerMotionEvent(int x, int y);
void triggerButtonEvent(int which, int state, int x, int y);
void triggerWheelEvent(int axis, int value);
void triggerKeyboardEvent(const char *string, int value, int state);
void triggerResizeEvent(int xSize, int ySize);

class GemJUCEWindow final : public Component
{
public:
    //==============================================================================
    GemJUCEWindow()
    {
        setSize (800, 600);
        
        setOpaque (true);
        //openGLContext.setRenderer (this);
        openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);
        
        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
        
        openGLContext.attachTo (*this);
        //openGLContext.setContinuousRepainting (true);
    
        instance = libpd_this_instance();
    }

    ~GemJUCEWindow() override
    {
    }

    //dequeueEvents(); -> restore changes

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

// window/context creation&destruction
bool initGemWin(void)
{
  return true;
}

std::map<t_pdinstance*, std::unique_ptr<GemJUCEWindow>> gemJUCEWindow;

int createGemWindow(WindowInfo& info, WindowHints& hints)
{
    if(auto* window = info.getWindow())
    {
        window->removeFromDesktop();
        gemJUCEWindow[window->instance].reset(nullptr);
    }
    
    auto* window = new GemJUCEWindow();
    gemJUCEWindow[window->instance].reset(window);
    info.window[window->instance] = window;
    
    window->addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowIsResizable | ComponentPeer::windowHasDropShadow);
    window->setVisible(true);
    
    window->openGLContext.makeActive();
    info.context[window->instance] = &window->openGLContext;
      
    hints.real_w = window->getWidth();
    hints.real_h = window->getHeight();

    
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
        context->makeActive();
        context->swapBuffers();
    }
}
void gemWinMakeCurrent(WindowInfo& info) {
    if (auto* context = info.getContext()) {
        context->makeActive();
    }
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
