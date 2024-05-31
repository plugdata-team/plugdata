/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;


#include "Utility/Config.h"
#include "Utility/SettingsFile.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#undef NANOVG_GL_IMPLEMENTATION
#include <nanovg_gl_utils.h>
#define NANOVG_GL_IMPLEMENTATION 1
#endif


class FrameTimer;
class PluginEditor;
class NVGSurface :
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
public UIViewComponent
#else
public Component, public Timer
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface();
    
    void initialise();
    void updateBufferSize();
    
    void render();
    
    void triggerRepaint();
    bool makeContextActive();
    
    void detachContext();

#ifdef NANOVG_GL_IMPLEMENTATION
    void timerCallback() override;
#endif

    Rectangle<int> getInvalidArea() { return invalidArea; }
    
    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);
    
    class InvalidationListener : public CachedComponentImage
    {
    public:
        InvalidationListener(NVGSurface& s, Component* origin, bool passRepaintEvents = false) : surface(s), originComponent(origin),  passEvents(passRepaintEvents)
        {
            
        }
        
        void paint(Graphics& g) override {};
        
        bool invalidate (const Rectangle<int>& rect) override
        {
            if(originComponent->isVisible()) {
                // Translate from canvas coords to viewport coords as float to prevent rounding errors
                auto invalidatedBounds = surface.getLocalArea(originComponent, rect.toFloat()).getSmallestIntegerContainer();
                surface.invalidateArea(invalidatedBounds);
            }
            return passEvents;
        }
        
        bool invalidateAll() override
        {   
            if(originComponent->isVisible()) {
                surface.invalidateArea(originComponent->getLocalBounds());
            }
            return passEvents;
        }
        
        void releaseResources() override {};
        
        
        NVGSurface& surface;
        Component* originComponent;
        bool passEvents;
    };
        
    void invalidateArea(Rectangle<int> area);
    void invalidateAll();
    
    NVGcontext* getRawContext() { return nvg; }

private:

    void resized() override;
    
    void renderPerfMeter(NVGcontext* nvg);
    
    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;
    
    Rectangle<int> invalidArea;
    NVGframebuffer* mainFBO = nullptr;
    NVGframebuffer* invalidFBO = nullptr;
    int fbWidth = 0, fbHeight = 0;
    
    bool hresize = false;
    bool resizing = false;
    Rectangle<int> newBounds;
    
#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif
    
    std::unique_ptr<FrameTimer> frameTimer;
};


class NVGImage
{
public:
    
    NVGImage(NVGcontext* ctx, int width, int height, int image)
    : nvg(ctx)
    , imageId(image)
    , imageWidth(width)
    , imageHeight(height)
    {
        allImages.insert(this);
    }
    
    NVGImage(NVGcontext* nvg, int width, int height, std::function<void(Graphics&)> renderCall)
    {
        Image image = Image(Image::ARGB, width, height, true);
        Graphics g(image); // Render resize handles with JUCE, since rounded rect exclusion is hard with nanovg
        renderCall(g);
        loadJUCEImage(nvg, image);
        allImages.insert(this);
    }
    
    NVGImage(NVGcontext* nvg, Image image)
    {
        loadJUCEImage(nvg, image);
        allImages.insert(this);
    }
    
    NVGImage()
    {
        allImages.insert(this);
    }
    
    NVGImage(NVGImage& other)
    {
        // Check for self-assignment
        if (this != &other) {
            nvg = other.nvg;
            imageId = other.imageId;
            imageWidth = other.imageWidth;
            imageHeight = other.imageHeight;
            
            other.imageId = 0;
            allImages.insert(this);
        }
    }
    
    NVGImage& operator=(NVGImage&& other) noexcept
    {
        // Check for self-assignment
        if (this != &other) {
            nvg = other.nvg;
            imageId = other.imageId;
            imageWidth = other.imageWidth;
            imageHeight = other.imageHeight;
            
            other.imageId = 0; // Important, makes sure the old buffer can't delete this buffer
            allImages.insert(this);
        }
        
        return *this;
    }
    
    ~NVGImage()
    {
        if(imageId && nvg) nvgDeleteImage(nvg, imageId);
        allImages.erase(this);
    }

    static void clearAll()
    {
        for(auto* image : allImages)
        {
            if(image->isValid() && image->nvg) nvgDeleteImage(image->nvg, image->imageId);
            image->imageId = 0;
        }
    }
    
    bool isValid()
    {
        return imageId != 0;
    }
    
    void renderJUCEComponent(NVGcontext* nvg, Component& component, float scale)
    {
        Image componentImage = component.createComponentSnapshot(Rectangle<int>(0, 0, component.getWidth() + 1, component.getHeight()), false, scale);
        if(componentImage.isNull()) return;
        
        loadJUCEImage(nvg, componentImage);

        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, component.getWidth() + 1, component.getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, component.getWidth() + 1, component.getHeight(), 0, imageId, 1.0f));
        nvgFill(nvg);
    }
    
    void loadJUCEImage(NVGcontext* context, Image& image)
    {
        Image::BitmapData imageData(image, Image::BitmapData::readOnly);

        int width = imageData.width;
        int height = imageData.height;
        uint8* pixelData = imageData.data;
        
        for (int y = 0; y < height; ++y)
        {
            auto* scanLine = (uint32*) imageData.getLinePointer(y);

            for (int x = 0; x < width; ++x)
            {
                uint32 argb = scanLine[x];

                uint8 a = argb >> 24;
                uint8 r = argb >> 16;
                uint8 g = argb >> 8;
                uint8 b = argb;

                // order bytes as abgr
                scanLine[x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
        
        if(imageId && imageWidth == width && imageHeight == height && nvg == context)
        {
            update(pixelData);
        }
        else {
            nvg = context;
            imageId =  nvgCreateImageRGBA(nvg, width, height, NVG_IMAGE_PREMULTIPLIED, pixelData);
            imageWidth = width;
            imageHeight = height;
        }
    }
    
    void update(uint8* pixelData)
    {
        nvgUpdateImage(nvg, imageId, pixelData);
    }
    
    bool needsUpdate(int width, int height)
    {
        return imageId == 0 || width != imageWidth || height != imageHeight;
    }
    
    int getImageId()
    {
        return imageId;
    }
    
    NVGcontext* nvg = nullptr;
    int imageId = 0;
    int imageWidth = 0, imageHeight = 0;
    
    static inline std::set<NVGImage*> allImages;
};

class NVGFramebuffer
{
public:
    NVGFramebuffer()
    {
        allFramebuffers.insert(this);
    }
    
    ~NVGFramebuffer()
    {
        
        allFramebuffers.erase(this);
    }
    
    static void clearAll()
    {
        for(auto* buffer : allFramebuffers)
        {
            if(buffer->fb) nvgDeleteFramebuffer(buffer->fb);
            buffer->fb = nullptr;
        }
    }
    
    bool needsUpdate(int width, int height)
    {
        return fb == nullptr || width != fbWidth || height != fbHeight || fbDirty;
    }
    
    bool isValid()
    {
        return fb != nullptr;
    }
    
    void setDirty()
    {
        fbDirty = true;
    }
    
    void bind(NVGcontext* nvg, int width, int height)
    {
        if(!fb || fbWidth != width || fbHeight != height) {
            if(fb) nvgDeleteFramebuffer(fb);
            fb = nvgCreateFramebuffer(nvg, width, height, NVG_IMAGE_PREMULTIPLIED);
            fbWidth = width;
            fbHeight = height;
        }
        
        nvgBindFramebuffer(fb);
    }
    
    void unbind()
    {
        nvgBindFramebuffer(nullptr);
    }
    
    void renderToFramebuffer(NVGcontext* nvg, int width, int height, std::function<void(NVGcontext*)> renderCallback)
    {
        bind(nvg, width, height);
        renderCallback(nvg);
        unbind();
        fbDirty = false;
    }
    
    void render(NVGcontext* nvg, Rectangle<int> b)
    {
        if(fb) {
            nvgBeginPath(nvg);
            nvgRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, b.getWidth(), b.getHeight(), 0, fb->image, 1));
            nvgFill(nvg);
        }
    }
    
    int getImage()
    {
        if(!fb) return -1;
        
        return fb->image;
    }
 
private:
    static inline std::set<NVGFramebuffer*> allFramebuffers;
    
    NVGframebuffer* fb = nullptr;
    int fbWidth, fbHeight;
    bool fbDirty = false;
};
