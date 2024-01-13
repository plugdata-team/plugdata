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

class GemJUCEWindow final : public OpenGLAppComponent
{
public:
    //==============================================================================
    GemJUCEWindow()
    {
        setSize (800, 600);
        //openGLContext.setSwapInterval(0);
        openGLContext.setMultisamplingEnabled(true);
        
        auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
        pixelFormat.multisamplingLevel = 2;
        openGLContext.setPixelFormat(pixelFormat);
    }

    ~GemJUCEWindow() override
    {
        shutdownOpenGL();
    }

    void initialise() override
    {
        openGLContext.makeActive();
        initGemWindow();
    }

    void shutdown() override
    {
    }

    void render() override
    {
        OpenGLHelpers::clear(Colours::black);
        
        sys_lock();
        dequeueEvents();
        performGemRender();
        sys_unlock();
    }
    
    void resized() override
    {
        const ScopedLock lock (mutex);
        bounds = getLocalBounds();
        
        triggerResizeEvent(bounds.getWidth(), bounds.getHeight());
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
    }
    
private:
   
    Rectangle<int> bounds;
    CriticalSection mutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GemJUCEWindow)
};

// window/context creation&destruction
bool initGemWin(void)
{
  return true;
}

std::unique_ptr<GemJUCEWindow> gemJUCEWindow;

int createGemWindow(WindowInfo& info, WindowHints& hints)
{
    if(info.window)
    {
        info.window->removeFromDesktop();
        delete info.window;
    }
    
    auto* window = new GemJUCEWindow();
    gemJUCEWindow.reset(window);
    info.window = window;
    
    info.window->addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowIsResizable | ComponentPeer::windowHasDropShadow);
    window->setVisible(true);
    window->openGLContext.makeActive();
    info.context = &window->openGLContext;
    //info.have_constContext = 1;
      
    hints.real_w = window->getWidth();
    hints.real_h = window->getHeight();
    
    return 1;
}
void destroyGemWindow(WindowInfo& info) {
    if(info.window)  {
        info.window->removeFromDesktop();
        delete info.window;
    }
}

void initWin_sharedContext(WindowInfo& info, WindowHints& hints) {
    //info.have_constContext = 1;
    if(info.window) info.window->openGLContext.makeActive();
}

// Rendering
void gemWinSwapBuffers(WindowInfo& info) {
    if (info.context) {
        info.context->makeActive();
        info.context->swapBuffers();
    }
}
void gemWinMakeCurrent(WindowInfo& info) {
    if (info.context) {
        info.context->makeActive();
    }
}

// We handle this manually with JUCE
void dispatchGemWindowMessages(WindowInfo& info) {
}

// Window behaviour
int cursorGemWindow(WindowInfo& info, int state)
{
    if(info.window) info.window->setMouseCursor(state ? MouseCursor::NormalCursor : MouseCursor::NoCursor);
  return state;
}

int topmostGemWindow(WindowInfo& info, int state)
{
  if(info.window && state) info.window->toFront(true);
  return state;
}
