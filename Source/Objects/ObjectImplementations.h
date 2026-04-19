/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include "Utility/GlobalMouseListener.h"
#include <raw_keyboard_input/raw_keyboard_input.h>
#include <Objects/ImplementationBase.h>

class CanvasMouseObject final : public ImplementationBase
    , public pd::MessageListener
    , public MouseListener {

    AtomicValue<bool> zero = false;
    Point<int> lastPosition;
    Point<int> zeroPosition;
    Component::SafePointer<Canvas> cnv;
    Component::SafePointer<Canvas> parentCanvas;

public:
    CanvasMouseObject(t_gobj* ptr, t_canvas const* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
    {
        pd->registerMessageListener(this->ptr.getRawUnchecked<void>(), this);
    }

    ~CanvasMouseObject() override
    {
        pd->unregisterMessageListener(this);

        if (!cnv)
            return;

        cnv->removeMouseListener(this);
    }

    void update() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (cnv) {
            cnv->removeMouseListener(this);
        }

        char* text;
        int size;

        t_glist* canvasToFind;
        if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
            binbuf_gettext(mouse->x_obj.te_binbuf, &text, &size);

            int depth = 0;

            auto const tokens = StringArray::fromTokens(String::fromUTF8(text, size), false);
            if (tokens.size() > 1 && tokens[1].containsOnly("0123456789")) {
                depth = tokens[1].getIntValue();
            }

            if (depth > 0) {
                canvasToFind = mouse->x_canvas->gl_owner;
            } else {
                canvasToFind = mouse->x_canvas;
            }
        } else {
            return;
        }

        cnv = getMainCanvas(canvasToFind);

        freebytes(text, static_cast<size_t>(size) * sizeof(char));

        if (!cnv)
            return;

        cnv->addMouseListener(this, true);
    }

    bool getMousePos(MouseEvent const& e, Point<int>& pos)
    {
        auto relativeEvent = e.getEventRelativeTo(cnv);

        pos = cnv->getLocalPoint(e.originalComponent, e.getPosition()) - cnv->canvasOrigin;
        bool const positionChanged = lastPosition != pos;
        lastPosition = pos;

        if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
            auto const* x = mouse->x_canvas;

            if (mouse->x_pos) {
                pos -= Point<int>(x->gl_obj.te_xpix, x->gl_obj.te_ypix);
            }
        }

        return positionChanged;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (pd->isPerformingGlobalSync)
            return;

        if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
            if (!cnv || (mouse->x_enable_edit_mode || !getValue<bool>(cnv->locked))) {
                outlet_float(mouse->x_obj.ob_outlet, 1.0);
            }
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
            if (!cnv || (mouse->x_enable_edit_mode || getValue<bool>(cnv->locked))) {
                outlet_float(mouse->x_obj.ob_outlet, 0.0f);
            }
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        Point<int> pos;
        bool const positionChanged = getMousePos(e, pos);

        if (zero) {
            zeroPosition = pos;
            zero = false;
        }

        pos -= zeroPosition;

        if (positionChanged) {
            if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
                if (!cnv || (mouse->x_enable_edit_mode || getValue<bool>(cnv->locked))) {
                    outlet_float(mouse->x_outlet_y, static_cast<float>(pos.y));
                    outlet_float(mouse->x_outlet_x, static_cast<float>(pos.x));
                }
            }
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }

    void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override
    {
        if (!cnv || pd->isPerformingGlobalSync)
            return;

        if (hash(symbol->s_name) == hash("zero")) {
            zero = true;
        }
    }
};

class CanvasVisibleObject final : public ImplementationBase
    , public ComponentListener
    , public Timer {

    bool lastFocus = false;
    Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    ~CanvasVisibleObject() override
    {
        if (!cnv)
            return;

        cnv->removeComponentListener(this);
    }

    void update() override
    {
        if (auto canvas_vis = ptr.get<t_fake_canvas_vis>()) {
            cnv = getMainCanvas(canvas_vis->x_canvas);
        }

        if (!cnv)
            return;

        cnv->addComponentListener(this);
        startTimer(100);
    }

    void updateVisibility()
    {
        if (pd->isPerformingGlobalSync)
            return;

        // We use a safepointer to the canvas to determine if it's still open
        bool const showing = cnv != nullptr;

        if (showing && !lastFocus) {
            lastFocus = true;
            if (auto vis = ptr.get<t_fake_canvas_vis>()) {
                outlet_float(vis->x_obj.ob_outlet, 1.0f);
            }
        } else if (lastFocus && !showing) {
            lastFocus = false;
            if (auto vis = ptr.get<t_fake_canvas_vis>()) {
                outlet_float(vis->x_obj.ob_outlet, 0.0f);
            }
        }
    }

    void componentBroughtToFront(Component& component) override
    {
        updateVisibility();
    }

    void componentVisibilityChanged(Component& component) override
    {
        updateVisibility();
    }

    void timerCallback() override
    {
        updateVisibility();
    }
};

class CanvasZoomObject final : public ImplementationBase
    , public Value::Listener {

    float lastScale;
    Value zoomScaleValue = SynchronousValue();

    Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    void update() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (cnv) {
            cnv->zoomScale.removeListener(this);
        }

        if (auto canvasZoom = ptr.get<t_fake_zoom>()) {
            cnv = getMainCanvas(canvasZoom->x_canvas);
        }

        if (!cnv)
            return;

        zoomScaleValue.referTo(cnv->zoomScale);
        zoomScaleValue.addListener(this);
        lastScale = getValue<float>(zoomScaleValue);
    }

    void valueChanged(Value&) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        auto const newScale = getValue<float>(zoomScaleValue);
        if (!approximatelyEqual(lastScale, newScale)) {
            if (auto zoom = ptr.get<t_fake_zoom>()) {
                outlet_float(zoom->x_obj.ob_outlet, newScale);
            }

            lastScale = newScale;
        }
    }
};

class CanvasEditObject final : public ImplementationBase
    , public Value::Listener {

    bool wasLocked;
    Value isLocked = SynchronousValue();

    Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    void update() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (cnv) {
            cnv->locked.removeListener(this);
        }

        if (auto canvasEdit = ptr.get<t_fake_edit>()) {
            cnv = getMainCanvas(canvasEdit->x_canvas);
        }

        if (!cnv)
            return;

        isLocked.referTo(cnv->locked);
        isLocked.addListener(this);
        wasLocked = getValue<float>(isLocked);
    }

    void valueChanged(Value&) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        auto const locked = getValue<float>(isLocked);
        if (wasLocked != locked) {
            if (auto edit = ptr.get<t_fake_edit>()) {
                outlet_float(edit->x_obj.ob_outlet, !locked);
            }
            wasLocked = locked;
        }
    }
};

// Else "mouse" component
class MouseObject final : public ImplementationBase
    , public Timer {

public:
    MouseObject(t_gobj* ptr, t_canvas const* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
        , mouseSource(Desktop::getInstance().getMainMouseSource())
    {
        lastPosition = mouseSource.getScreenPosition();
        lastMouseDownTime = mouseSource.getLastMouseDownTime();
        startTimer(timerInterval);

        globalMouseListener.globalMouseWheel = [this](MouseEvent const& e, MouseWheelDetails const& details) {
            if (auto obj = this->ptr.get<void>()) {
                if (details.deltaY != 0.0f)
                    this->pd->sendDirectMessage(obj.get(), "_wheel", { details.deltaY * 50.f, 0.f });
                if (details.deltaX != 0.0f)
                    this->pd->sendDirectMessage(obj.get(), "_wheel", { details.deltaX * 50.f, 1.f });
            }
        };
    }

    void timerCallback() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (lastPosition != mouseSource.getScreenPosition()) {

            auto pos = mouseSource.getScreenPosition();
            if (auto obj = ptr.get<void>()) {
                pd->sendDirectMessage(obj.get(), "_getscreen", { pos.x, pos.y });
            }

            lastPosition = pos;
        }
        if (mouseSource.isDragging()) {
            if (!isDown) {
                if (auto obj = ptr.get<void>()) {
                    pd->sendDirectMessage(obj.get(), "_up", { 0.0f });
                }
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (mouseSource.getLastMouseDownTime() > lastMouseDownTime) {
            if (!isDown) {
                if (auto obj = ptr.get<void>()) {
                    pd->sendDirectMessage(obj.get(), "_up", { 0.0f });
                }
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (isDown) {
            if (auto obj = ptr.get<void>()) {
                pd->sendDirectMessage(obj.get(), "_up", { 1.0f });
            }
            isDown = false;
        }
    }

    MouseInputSource mouseSource;
    GlobalMouseListener globalMouseListener;

    Time lastMouseDownTime;
    Point<float> lastPosition;
    bool isDown = false;
    int const timerInterval = 30;
};

class MouseStateObject final : public ImplementationBase
    , public pd::MessageListener
    , public MouseListener {

    Point<int> lastPosition;
    Point<int> currentPosition;

    GlobalMouseListener mouseListener;

public:
    MouseStateObject(t_gobj* object, t_canvas const* parent, PluginProcessor* pd)
        : ImplementationBase(object, parent, pd)
    {
        pd->registerMessageListener(ptr.getRawUnchecked<void>(), this);

        mouseListener.globalMouseDown = [this](MouseEvent const&) {
            if (auto obj = this->ptr.get<t_object>()) {
                outlet_float(obj->ob_outlet, 1.0f);
            }
        };
        mouseListener.globalMouseUp = [this](MouseEvent const&) {
            if (auto obj = this->ptr.get<t_object>()) {
                outlet_float(obj->ob_outlet, 0.0f);
            }
        };
    }

    ~MouseStateObject() override
    {
        pd->unregisterMessageListener(this);
    }

    void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (hash(symbol->s_name) == hash("bang")) {
            auto const currentPosition = Desktop::getMousePosition();

            if (auto obj = ptr.get<t_fake_mousestate>()) {
                outlet_float(obj->x_hposout, currentPosition.x);
                outlet_float(obj->x_vposout, currentPosition.y);
                outlet_float(obj->x_hdiffout, currentPosition.x - lastPosition.x);
                outlet_float(obj->x_vdiffout, currentPosition.y - lastPosition.y);

                lastPosition = currentPosition;
            }
        }
    }
};

class KeycodeObject final : public ImplementationBase
    , public ModifierKeyListener {

public:
    std::unique_ptr<Keyboard> keyboard;
    Component::SafePointer<PluginEditor> attachedEditor = nullptr;

    KeycodeObject(t_gobj* ptr, t_canvas const* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
    {
    }

    ~KeycodeObject() override
    {
        if (attachedEditor) {
            attachedEditor->removeModifierKeyListener(this);
            // attachedEditor->removeKeyListener(this);
        }
    }
#if !JUCE_IOS
    void update() override
    {
        if (auto const* canvas = getMainCanvas(cnv, true)) {
            attachedEditor = canvas->editor;
            attachedEditor->addModifierKeyListener(this);
            // attachedEditor->addKeyListener(this);
            keyboard.reset(nullptr);
            keyboard = std::unique_ptr<Keyboard>(KeyboardFactory::instance(attachedEditor));

            // Install callbacks
            keyboard->onKeyDownFn = [&](int const keynum) {
                auto const hid = OSUtils::keycodeToHID(keynum);

                if (auto obj = ptr.get<t_fake_keycode>()) {
                    outlet_float(obj->x_outlet2, hid);
                    outlet_float(obj->x_outlet1, 1.0f);
                }
            };
            keyboard->onKeyUpFn = [&](int const keynum) {
                auto const hid = OSUtils::keycodeToHID(keynum);

                if (auto obj = ptr.get<t_fake_keycode>()) {
                    outlet_float(obj->x_outlet2, hid);
                    outlet_float(obj->x_outlet1, 0.0f);
                }
            };
        }
    }
#endif
};

class MouseFilterObject final : public ImplementationBase
    , public GlobalMouseListener {

    class MouseFilterProxy {
    public:
        MouseFilterProxy()
            : pd(nullptr)
        {
        }

        explicit MouseFilterProxy(pd::Instance* instance)
            : pd(instance)
        {
        }

        void setState(bool const newState)
        {
            if (newState != state) {
                state = newState;
                pd->setThis();
                pd->lockAudioThread();
                pd->sendMessage("#hammergui", "_up", { pd::Atom(!state) });
                pd->unlockAudioThread();
            }
        }

    private:
        pd::Instance* pd;
        bool state = false;
    };

    static inline UnorderedMap<pd::Instance*, MouseFilterProxy> proxy;

public:
    MouseFilterObject(t_gobj* object, t_canvas const* parent, PluginProcessor* pd)
        : ImplementationBase(object, parent, pd)
    {
        if (!proxy.count(pd)) {
            proxy[pd] = MouseFilterProxy(pd);

            globalMouseDown = [pd](MouseEvent const&) {
                proxy[pd].setState(true);
            };

            globalMouseUp = [pd](MouseEvent const&) {
                proxy[pd].setState(false);
            };
        }
    }
};
