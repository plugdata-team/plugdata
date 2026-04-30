/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#if ENABLE_GEM

#    define GEM_NO_SETUP 1
#    include <Gem/src/Output/gemjucewindow.h>

// plugdata exclusive Gem object: renders the content of the currently active gem window to the plugdata canvas
class GemCanvasObject final : public ObjectBase
    , private Timer {
    Value sizeProperty = SynchronousValue();

    int nvgImage = -1;
    int texW = 0, texH = 0;
    std::vector<uint8_t> rgbaBuffer;
    GemCanvas* gemCanvas = nullptr;

    Array<KeyPress> heldKeys;
    bool shiftDown = false, ctrlDown = false, altDown = false, cmdDown = false;

public:
    GemCanvasObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        objectParameters.addParamSize(&sizeProperty);
        startTimerHz(60);
    }

    ~GemCanvasObject() override
    {
        stopTimer();
    }

    void updateCanvas()
    {
    }

    void update() override
    {
        if (auto obj = ptr.get<t_gemcanvas>()) {
            gemCanvas = &obj->canvas;
            sizeProperty = VarArray { var(obj->canvas.requestedWidth.load()), var(obj->canvas.requestedHeight.load()) };
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        setParameterExcludingListener(sizeProperty,
            VarArray { var(getWidth()), var(getHeight()) });
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();

        // Dark background placeholder
        nvgDrawRoundedRect(nvg,
            b.getX(), b.getY(), b.getWidth(), b.getHeight(),
            nvgRGBf(0.05f, 0.05f, 0.05f),
            nvgRGBf(0.15f, 0.15f, 0.15f),
            Corners::objectCornerRadius);

        if (!gemCanvas)
            return;

        // Pull new pixel data if available
        if (gemCanvas->frameDirty.load()) {
            int w = 0, h = 0;
            if (gemCanvas->pullPixels(rgbaBuffer, w, h) && !rgbaBuffer.empty()) {
                if (nvgImage < 0 || w != texW || h != texH) {
                    if (nvgImage >= 0)
                        nvgDeleteImage(nvg, nvgImage);
                    nvgImage = nvgCreateImageARGB(nvg, w, h, 0, rgbaBuffer.data());
                    texW = w;
                    texH = h;
                } else {
                    nvgUpdateImage(nvg, nvgImage, rgbaBuffer.data());
                }
            }
        }

        if (nvgImage < 0)
            return;

        NVGpaint paint = nvgImagePattern(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), 0.0f, nvgImage, 1.0f);

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
        nvgFillPaint(nvg, paint);
        nvgFill(nvg);
    }

    void timerCallback() override
    {
        // Repaint when a new frame is available
        if (gemCanvas && gemCanvas->frameDirty.load())
            repaint();

        if (gemCanvas && gemCanvas->framerate != (1000.0f / getTimerInterval()))
            startTimerHz(gemCanvas->framerate);

        if (!gemCanvas)
            return;

        auto mods = ModifierKeys::getCurrentModifiers();
        auto hasFocus = hasKeyboardFocus(true);

        for (int i = heldKeys.size() - 1; i >= 0; --i) {
            auto key = heldKeys[i];
            if (!KeyPress::isKeyCurrentlyDown(key.getKeyCode())) {
                if (gemCanvas->keyUpCallback)
                    gemCanvas->keyUpCallback(key);
                heldKeys.remove(i);
            }
        }

        auto checkMod = [&](bool& state, bool pressed, int code, std::string name) {
            if (pressed != state) {
                state = pressed;
                if (state) {
                    if (gemCanvas->keyCallback)
                        gemCanvas->keyCallback(KeyPress(code));
                } else {
                    if (gemCanvas->keyUpCallback)
                        gemCanvas->keyUpCallback(KeyPress(code));
                }
            }
        };

        checkMod(shiftDown, hasFocus && mods.isShiftDown(), 340, "Shift");
        checkMod(ctrlDown, hasFocus && mods.isCtrlDown(), 341, "Control");
        checkMod(altDown, hasFocus && mods.isAltDown(), 342, "Alt");
        checkMod(cmdDown, hasFocus && mods.isCommandDown(), 343, "Super");
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (gemCanvas && gemCanvas->mouseCallback)
            gemCanvas->mouseCallback(e, true);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (gemCanvas && gemCanvas->mouseCallback)
            gemCanvas->mouseCallback(e, false);
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (gemCanvas && gemCanvas->motionCallback)
            gemCanvas->motionCallback(e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (gemCanvas && gemCanvas->motionCallback)
            gemCanvas->motionCallback(e);
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        if (gemCanvas && gemCanvas->scrollCallback)
            gemCanvas->scrollCallback(e, wheel);
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (!gemCanvas || !gemCanvas->keyCallback)
            return false;
        gemCanvas->keyCallback(key);
        heldKeys.add(key);
        return false;
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto obj = ptr.get<t_gemcanvas>()) {
                obj->canvas.requestedWidth = width;
                obj->canvas.requestedHeight = height;
            }

            object->updateBounds();
        }
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto obj = ptr.get<t_gemcanvas>()) {
            obj->x_obj.te_xpix = b.getX();
            obj->x_obj.te_ypix = b.getY();
            obj->canvas.requestedWidth = b.getWidth();
            obj->canvas.requestedHeight = b.getHeight();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            int const fw = gemCanvas ? gemCanvas->requestedWidth.load() : w;
            int const fh = gemCanvas ? gemCanvas->requestedHeight.load() : h;
            return { x, y, fw > 0 ? fw : w, fh > 0 ? fh : h };
        }
        return { };
    }
};
#endif
