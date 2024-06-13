/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/GlobalMouseListener.h"
#include <raw_keyboard_input/raw_keyboard_input.h>

class SubpatchImpl : public ImplementationBase
    , public pd::MessageListener {
public:
    WeakReference<pd::Instance> pdWeakRef;

    SubpatchImpl(t_gobj* ptr, t_canvas* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
        , pdWeakRef(pd)
    {
        pd->registerMessageListener(this->ptr.getRawUnchecked<void>(), this);
    }

    ~SubpatchImpl() override
    {
        if (pdWeakRef)
            pdWeakRef->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
        closeOpenedSubpatchers();
    }

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        bool isVisMessage = hash(symbol->s_name) == hash("vis");
        if (isVisMessage && atoms[0].getFloat()) {
            openSubpatch(subpatch);
        } else if (isVisMessage) {
            closeOpenedSubpatchers();
        }
    }

    pd::Patch* subpatch = nullptr;

    JUCE_DECLARE_WEAK_REFERENCEABLE(SubpatchImpl)
};

// Wrapper for Pd's key, keyup and keyname objects
class KeyObject final : public ImplementationBase
    , public KeyListener
    , public ModifierKeyListener {

    Array<KeyPress> heldKeys;
    Array<double> keyPressTimes;

    int const shiftKey = -1;
    int const commandKey = -2;
    int const altKey = -3;
    int const ctrlKey = -4;

public:
    enum KeyObjectType {
        Key,
        KeyUp,
        KeyName
    };
    KeyObjectType type;
    Component::SafePointer<PluginEditor> attachedEditor = nullptr;

    KeyObject(t_gobj* ptr, t_canvas* parent, PluginProcessor* pd, KeyObjectType keyObjectType)
        : ImplementationBase(ptr, parent, pd)
        , type(keyObjectType)
    {
    }

    ~KeyObject() override
    {
        if (attachedEditor) {
            attachedEditor->removeModifierKeyListener(this);
            attachedEditor->removeKeyListener(this);
        }
    }

    void update() override
    {
        auto* canvas = getMainCanvas(cnv, true);
        if (canvas) {
            attachedEditor = canvas->editor;
            attachedEditor->addModifierKeyListener(this);
            attachedEditor->addKeyListener(this);
        }
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (pd->isPerformingGlobalSync)
            return false;

        auto const keyIdx = heldKeys.indexOf(key);
        auto const alreadyDown = keyIdx >= 0;
        auto const currentTime = Time::getMillisecondCounterHiRes();
        if (alreadyDown && currentTime - keyPressTimes[keyIdx] > 80) {
            keyPressTimes.set(keyIdx, currentTime);
        } else if (!alreadyDown) {
            heldKeys.add(key);
            keyPressTimes.add(currentTime);
        } else {
            return false;
        }

        int keyCode = key.getKeyCode();

        if (type == Key) {
            t_symbol* dummy;
            parseKey(keyCode, dummy);
            if (auto obj = ptr.get<t_pd>())
                pd->sendDirectMessage(obj.get(), keyCode);
        } else if (type == KeyName) {

            String keyString = key.getTextDescription().fromLastOccurrenceOf(" ", false, false);

            if (keyString.startsWith("#")) {
                keyString = String::charToString(key.getTextCharacter());
            }
            if (!key.getModifiers().isShiftDown()) {
                keyString = keyString.toLowerCase();
            }

            t_symbol* keysym = pd->generateSymbol(keyString);
            parseKey(keyCode, keysym);

            if (auto obj = ptr.get<t_pd>())
                pd->sendDirectMessage(obj.get(), { 1.0f, keysym });
        }

        // Never claim the keypress
        return false;
    }

    void shiftKeyChanged(bool isHeld) override
    {
        if (isHeld)
            keyPressed(KeyPress(shiftKey), nullptr);
        else
            keyStateChanged(false, nullptr);
    }
    void commandKeyChanged(bool isHeld) override
    {
        if (isHeld)
            keyPressed(KeyPress(commandKey), nullptr);
        else
            keyStateChanged(false, nullptr);
    }
    void altKeyChanged(bool isHeld) override
    {
        if (isHeld)
            keyPressed(KeyPress(altKey), nullptr);
        else
            keyStateChanged(false, nullptr);
    }
    void ctrlKeyChanged(bool isHeld) override
    {
        if (isHeld)
            keyPressed(KeyPress(ctrlKey), nullptr);
        else
            keyStateChanged(false, nullptr);
    }

    void spaceKeyChanged(bool isHeld) override
    {
        if (isHeld)
            keyPressed(KeyPress(KeyPress::spaceKey), nullptr);
        else
            keyStateChanged(false, nullptr);
    }

    bool keyStateChanged(bool isKeyDown, Component* originatingComponent) override
    {
        if (pd->isPerformingGlobalSync)
            return false;

        if (!isKeyDown) {
            for (int n = heldKeys.size() - 1; n >= 0; n--) {
                auto key = heldKeys[n];

                bool keyDown = key.getKeyCode() < 0 ? isKeyDown : key.isCurrentlyDown();

                if (!keyDown) {
                    int keyCode = key.getKeyCode();

                    if (type == KeyUp) {
                        t_symbol* dummy;
                        parseKey(keyCode, dummy);
                        if (auto obj = ptr.get<t_pd>())
                            pd->sendDirectMessage(obj.get(), keyCode);
                    } else if (type == KeyName) {

                        String keyString = key.getTextDescription().fromLastOccurrenceOf(" ", false, false);

                        if (keyString.startsWith("#")) {
                            keyString = String::charToString(key.getTextCharacter());
                        }
                        if (!key.getModifiers().isShiftDown()) {
                            keyString = keyString.toLowerCase();
                        }

                        t_symbol* keysym = pd->generateSymbol(keyString);
                        parseKey(keyCode, keysym);
                        if (auto obj = ptr.get<t_pd>())
                            pd->sendDirectMessage(obj.get(), { 0.0f, keysym });
                    }

                    keyPressTimes.remove(n);
                    heldKeys.remove(n);
                }
            }
        }

        // Never claim the keychange
        return false;
    }

    void parseKey(int& keynum, t_symbol*& keysym)
    {
        if (keynum == shiftKey) {
            keysym = pd->generateSymbol("Shift_L");
            keynum = 0;
        } else if (keynum == commandKey) {
            keysym = pd->generateSymbol("Meta_L");
            keynum = 0;
        } else if (keynum == altKey) {
            keysym = pd->generateSymbol("Alt_L");
            keynum = 0;
        } else if (keynum == ctrlKey) {
            keysym = pd->generateSymbol("Control_L");
            keynum = 0;
        } else if (keynum == KeyPress::backspaceKey) {
            keysym = pd->generateSymbol("BackSpace");
            keynum = 8;
        } else if (keynum == KeyPress::tabKey)
            keynum = 9, keysym = pd->generateSymbol("Tab");
        else if (keynum == KeyPress::returnKey)
            keynum = 10, keysym = pd->generateSymbol("Return");
        else if (keynum == KeyPress::escapeKey)
            keynum = 27, keysym = pd->generateSymbol("Escape");
        else if (keynum == KeyPress::spaceKey)
            keynum = 32, keysym = pd->generateSymbol("Space");
        else if (keynum == KeyPress::deleteKey)
            keynum = 127, keysym = pd->generateSymbol("Delete");

        else if (keynum == KeyPress::upKey)
            keynum = 0, keysym = pd->generateSymbol("Up");
        else if (keynum == KeyPress::downKey)
            keynum = 0, keysym = pd->generateSymbol("Down");
        else if (keynum == KeyPress::leftKey)
            keynum = 0, keysym = pd->generateSymbol("Left");
        else if (keynum == KeyPress::rightKey)
            keynum = 0, keysym = pd->generateSymbol("Right");
        else if (keynum == KeyPress::homeKey)
            keynum = 0, keysym = pd->generateSymbol("Home");
        else if (keynum == KeyPress::endKey)
            keynum = 0, keysym = pd->generateSymbol("End");
        else if (keynum == KeyPress::pageUpKey)
            keynum = 0, keysym = pd->generateSymbol("Prior");
        else if (keynum == KeyPress::pageDownKey)
            keynum = 0, keysym = pd->generateSymbol("Next");
        else if (keynum == KeyPress::F1Key)
            keynum = 0, keysym = pd->generateSymbol("F1");
        else if (keynum == KeyPress::F2Key)
            keynum = 0, keysym = pd->generateSymbol("F2");
        else if (keynum == KeyPress::F3Key)
            keynum = 0, keysym = pd->generateSymbol("F3");
        else if (keynum == KeyPress::F4Key)
            keynum = 0, keysym = pd->generateSymbol("F4");
        else if (keynum == KeyPress::F5Key)
            keynum = 0, keysym = pd->generateSymbol("F5");
        else if (keynum == KeyPress::F6Key)
            keynum = 0, keysym = pd->generateSymbol("F6");
        else if (keynum == KeyPress::F7Key)
            keynum = 0, keysym = pd->generateSymbol("F7");
        else if (keynum == KeyPress::F8Key)
            keynum = 0, keysym = pd->generateSymbol("F8");
        else if (keynum == KeyPress::F9Key)
            keynum = 0, keysym = pd->generateSymbol("F9");
        else if (keynum == KeyPress::F10Key)
            keynum = 0, keysym = pd->generateSymbol("F10");
        else if (keynum == KeyPress::F11Key)
            keynum = 0, keysym = pd->generateSymbol("F11");
        else if (keynum == KeyPress::F12Key)
            keynum = 0, keysym = pd->generateSymbol("F12");

        else if (keynum == KeyPress::numberPad0)
            keynum = 48, keysym = pd->generateSymbol("0");
        else if (keynum == KeyPress::numberPad1)
            keynum = 49, keysym = pd->generateSymbol("1");
        else if (keynum == KeyPress::numberPad2)
            keynum = 50, keysym = pd->generateSymbol("2");
        else if (keynum == KeyPress::numberPad3)
            keynum = 51, keysym = pd->generateSymbol("3");
        else if (keynum == KeyPress::numberPad4)
            keynum = 52, keysym = pd->generateSymbol("4");
        else if (keynum == KeyPress::numberPad5)
            keynum = 53, keysym = pd->generateSymbol("5");
        else if (keynum == KeyPress::numberPad6)
            keynum = 54, keysym = pd->generateSymbol("6");
        else if (keynum == KeyPress::numberPad7)
            keynum = 55, keysym = pd->generateSymbol("7");
        else if (keynum == KeyPress::numberPad8)
            keynum = 56, keysym = pd->generateSymbol("8");
        else if (keynum == KeyPress::numberPad9)
            keynum = 57, keysym = pd->generateSymbol("9");

            // on macOS, alphanumeric characters are offset
#if JUCE_MAC || JUCE_WINDOWS
        else if (keynum >= 65 && keynum <= 90) {
            keynum += 32;
        }
#endif
    }
};

class CanvasMouseObject final : public ImplementationBase
    , public MouseListener
    , public pd::MessageListener {

    std::atomic<bool> zero = false;
    Point<int> lastPosition;
    Point<int> zeroPosition;
    Component::SafePointer<Canvas> cnv;
    Component::SafePointer<Canvas> parentCanvas;

public:
    CanvasMouseObject(t_gobj* ptr, t_canvas* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
    {
        pd->registerMessageListener(this->ptr.getRawUnchecked<void>(), this);
    }

    ~CanvasMouseObject() override
    {
        pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
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

            auto tokens = StringArray::fromTokens(String::fromUTF8(text, size), false);
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

        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

        if (!cnv)
            return;

        cnv->addMouseListener(this, true);
    }

    bool getMousePos(MouseEvent const& e, Point<int>& pos)
    {
        auto relativeEvent = e.getEventRelativeTo(cnv);

        pos = cnv->getLocalPoint(e.originalComponent, e.getPosition()) - cnv->canvasOrigin;
        bool positionChanged = lastPosition != pos;
        lastPosition = pos;

        if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
            auto* x = mouse->x_canvas;

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
        bool positionChanged = getMousePos(e, pos);

        if (zero) {
            zeroPosition = pos;
            zero = false;
        }

        pos -= zeroPosition;

        if (positionChanged) {
            if (auto mouse = ptr.get<t_fake_canvas_mouse>()) {
                if (!cnv || (mouse->x_enable_edit_mode || getValue<bool>(cnv->locked))) {
                    outlet_float(mouse->x_outlet_y, (float)pos.y);
                    outlet_float(mouse->x_outlet_x, (float)pos.x);
                }
            }
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override
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
        if(auto canvas_vis = ptr.get<t_fake_canvas_vis>())
        {
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
        bool showing = cnv != nullptr;

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
            cnv->locked.removeListener(this);
        }

        if(auto canvas_zoom = ptr.get<t_fake_zoom>())
        {
            cnv = getMainCanvas(canvas_zoom->x_canvas);
        }
        
        if (!cnv) return;

        zoomScaleValue.referTo(cnv->zoomScale);
        zoomScaleValue.addListener(this);
        lastScale = getValue<float>(zoomScaleValue);
    }

    void valueChanged(Value&) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        auto newScale = getValue<float>(zoomScaleValue);
        if (!approximatelyEqual(lastScale, newScale)) {
            if (auto zoom = ptr.get<t_fake_zoom>()) {
                outlet_float(zoom->x_obj.ob_outlet, newScale);
            }

            lastScale = newScale;
        }
    }
};

// Else "mouse" component
class MouseObject final : public ImplementationBase
    , public Timer {

public:
    MouseObject(t_gobj* ptr, t_canvas* parent, PluginProcessor* pd)
        : ImplementationBase(ptr, parent, pd)
        , mouseSource(Desktop::getInstance().getMainMouseSource())
    {
        lastPosition = mouseSource.getScreenPosition();
        lastMouseDownTime = mouseSource.getLastMouseDownTime();
        startTimer(timerInterval);
        if(auto mouse = this->ptr.get<t_fake_mouse>())
        {
            canvas = mouse->x_glist;
        }
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

    Time lastMouseDownTime;
    Point<float> lastPosition;
    bool isDown = false;
    int const timerInterval = 30;
    t_glist* canvas;
};

class MouseStateObject final : public ImplementationBase
    , public MouseListener
    , public pd::MessageListener {

    Point<int> lastPosition;
    Point<int> currentPosition;

    GlobalMouseListener mouseListener;

public:
    MouseStateObject(t_gobj* object, t_canvas* parent, PluginProcessor* pd)
        : ImplementationBase(object, parent, pd)
    {
        pd->registerMessageListener(ptr.getRawUnchecked<void>(), this);

        mouseListener.globalMouseDown = [this](MouseEvent const& e) {
            if (auto obj = this->ptr.get<t_object>()) {
                outlet_float(obj->ob_outlet, 1.0f);
            }
        };
        mouseListener.globalMouseUp = [this](MouseEvent const& e) {
            if (auto obj = this->ptr.get<t_object>()) {
                outlet_float(obj->ob_outlet, 0.0f);
            }
        };
    }

    ~MouseStateObject()
    {
        pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
    }

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (hash(symbol->s_name) == hash("bang")) {
            auto currentPosition = Desktop::getMousePosition();

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

    KeycodeObject(t_gobj* ptr, t_canvas* parent, PluginProcessor* pd)
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
        auto* canvas = getMainCanvas(cnv, true);
        if (canvas) {
            attachedEditor = canvas->editor;
            attachedEditor->addModifierKeyListener(this);
            // attachedEditor->addKeyListener(this);
            keyboard.reset(nullptr);
            keyboard = std::unique_ptr<Keyboard>(KeyboardFactory::instance(attachedEditor));

            // Install callbacks
            keyboard->onKeyDownFn = [&](int keynum) {
                auto hid = OSUtils::keycodeToHID(keynum);

                if (auto obj = ptr.get<t_fake_keycode>()) {
                    outlet_float(obj->x_outlet2, hid);
                    outlet_float(obj->x_outlet1, 1.0f);
                }
            };
            keyboard->onKeyUpFn = [&](int keynum) {
                auto hid = OSUtils::keycodeToHID(keynum);

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

        MouseFilterProxy(pd::Instance* instance)
            : pd(instance)
        {
        }

        void setState(bool newState)
        {
            if (newState != state) {
                state = newState;
                pd->setThis();
                pd->sendMessage("#hammergui", "_up", { pd::Atom(!state) });
            }
        }

    private:
        pd::Instance* pd;
        bool state = false;
    };

    static inline std::map<pd::Instance*, MouseFilterProxy> proxy;

public:
    MouseFilterObject(t_gobj* object, t_canvas* parent, PluginProcessor* pd)
        : ImplementationBase(object, parent, pd)
    {
        if (!proxy.count(pd)) {
            proxy[pd] = MouseFilterProxy(pd);

            globalMouseDown = [pd](MouseEvent const& e) {
                proxy[pd].setState(true);
            };

            globalMouseUp = [pd](MouseEvent const& e) {
                proxy[pd].setState(false);
            };
        }
    }
};
