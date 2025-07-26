/*
 // Copyright (c) 2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <nanovg.h>

class NVGComponent {
public:
    NVGComponent(Component* comp);
    virtual ~NVGComponent();

    static NVGcolor convertColour(Colour const c);
    static Colour convertColour(NVGcolor const c);

    NVGcolor findNVGColour(int const colourId) const;

    static void setJUCEPath(NVGcontext* nvg, Path const& p);

    virtual void updateFramebuffers(NVGcontext*);

    virtual void render(NVGcontext*);

private:
    Component& component;

    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};

class NVGImage {
public:
    enum NVGImageFlags {
        RepeatImage = 1 << 0,
        DontClear = 1 << 1,
        AlphaImage = 1 << 2,
        MipMap = 1 << 3
    };

    NVGImage(NVGcontext* nvg, int width, int height, std::function<void(Graphics&)> renderCall, int const imageFlags = 0, Colour const clearColour = Colours::transparentBlack);
    NVGImage();
    NVGImage(NVGImage& other);
    NVGImage& operator=(NVGImage&& other) noexcept;
    ~NVGImage();

    static void clearAll(NVGcontext* nvg);

    bool isValid() const;

    void renderJUCEComponent(NVGcontext* nvg, Component& component, float const scale);

    void deleteImage();

    void loadJUCEImage(NVGcontext* context, Image& image, int const repeatImage = false, int const withMipmaps = false);

    void renderAlphaImage(NVGcontext* nvg, Rectangle<int> b, NVGcolor const col);

    void render(NVGcontext* nvg, Rectangle<int> b, bool quantize = false);

    bool needsUpdate(int const width, int const height) const;

    int getImageId();

    void setDirty();

    struct SubImage {
        int imageId = 0;
        Rectangle<int> bounds;
    };

    NVGcontext* nvg = nullptr;
    SmallArray<SubImage> subImages;
    int totalWidth = 0, totalHeight = 0;
    bool isDirty = false;

    std::function<void()> onImageInvalidate = nullptr;

    static inline UnorderedSet<NVGImage*> allImages = UnorderedSet<NVGImage*>();
};

class NVGFramebuffer {
public:
    NVGFramebuffer();
    ~NVGFramebuffer();
    
    static void clearAll(NVGcontext* nvg);
    
    bool needsUpdate(int const width, int const height) const;

    bool isValid() const;

    void setDirty();
    
    void bind(NVGcontext* ctx, int const width, int const height);

    static void unbind();
    
    void renderToFramebuffer(NVGcontext* nvg, int const width, int const height, std::function<void(NVGcontext*)> renderCallback);

    void render(NVGcontext* nvg, Rectangle<int> b);

    int getImage() const;

private:
    static inline UnorderedSet<NVGFramebuffer*> allFramebuffers;

    NVGcontext* nvg;
    NVGframebuffer* fb = nullptr;
    int fbWidth, fbHeight;
    bool fbDirty = false;
};

class NVGCachedPath {
public:
    NVGCachedPath();
    ~NVGCachedPath();

    static void clearAll(NVGcontext* nvg);
    static void resetAll();

    void clear();

    bool isValid() const;

    void save(NVGcontext* ctx);

    bool stroke();
    bool fill();

private:
    static inline UnorderedSet<NVGCachedPath*> allCachedPaths;
    NVGcontext* nvg = nullptr;
    int cacheId = -1;
};

struct NVGScopedState {
    NVGScopedState(NVGcontext* nvg);
    ~NVGScopedState();

    NVGcontext* nvg;
};
