/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/GlobalMouseListener.h"

class SubpatchImpl : public ImplementationBase
    , public pd::MessageListener {
public:
    SubpatchImpl(void* ptr, PluginProcessor* pd)
        : ImplementationBase(ptr, pd)
    {
        pd->registerMessageListener(ptr, this);
    }

    ~SubpatchImpl()
    {
        pd->unregisterMessageListener(ptr, this);
        closeOpenedSubpatchers();
    }

    void receiveMessage(pd::MessageSymbol const& message, int argc, t_atom* argv) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        auto atoms = pd::Atom::fromAtoms(argc, argv);

        bool isVisMessage = message.name == "vis";
        if (isVisMessage && atoms[0].getFloat()) {
            MessageManager::callAsync([_this = WeakReference(this)] {
                if (_this)
                    _this->openSubpatch(_this->subpatch);
            });
        } else if (isVisMessage) {
            MessageManager::callAsync([_this = WeakReference(this)] {
                if (_this)
                    _this->closeOpenedSubpatchers();
            });
        }
    }

    pd::Patch* subpatch = nullptr;

    JUCE_DECLARE_WEAK_REFERENCEABLE(SubpatchImpl);
};

// Wrapper for Pd's key, keyup and keyname objects
class KeyObject final : public ImplementationBase
    , public KeyListener
    , public ModifierKeyListener {

    Array<KeyPress> heldKeys;
    Array<uint32> keyPressTimes;

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

    KeyObject(void* ptr, PluginProcessor* pd, KeyObjectType keyObjectType)
        : ImplementationBase(ptr, pd)
        , type(keyObjectType)
    {
    }

    ~KeyObject()
    {
        if (auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor())) {
            editor->removeModifierKeyListener(this);
            editor->removeKeyListener(this);
        }
    }
        
    void update() override
    {
        if (auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor())) {
            // Capture key events for whole window
            editor->addKeyListener(this);
            editor->addModifierKeyListener(this);
        }
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (pd->isPerformingGlobalSync)
            return false;

        auto const keyIdx = heldKeys.indexOf(key);
        auto const alreadyDown = keyIdx >= 0;
        auto const currentTime = Time::getMillisecondCounter();
        if (alreadyDown && currentTime - keyPressTimes[keyIdx] > 15) {
            keyPressTimes.set(keyIdx, currentTime);
        } else if (!alreadyDown) {
            heldKeys.add(key);
            keyPressTimes.add(Time::getMillisecondCounter());
        } else {
            return false;
        }

        int keyCode = key.getKeyCode();

        if (type == Key) {
            t_symbol* dummy;
            parseKey(keyCode, dummy);
            pd->enqueueDirectMessages(ptr, keyCode);
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

            pd->enqueueDirectMessages(ptr, { 1.0f, keysym });
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
                        pd->enqueueDirectMessages(ptr, keyCode);
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
                        pd->enqueueDirectMessages(ptr, { 1.0f, keysym });
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
        } else if (keynum == KeyPress::backspaceKey)
            keysym = pd->generateSymbol("BackSpace");
        else if (keynum == KeyPress::tabKey)
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
#if JUCE_MAC
        else if (keynum >= 65 && keynum <= 90) {
            keynum += 32;
        }
#endif
    }
};

class CanvasActiveObject final : public ImplementationBase
    , public FocusChangeListener {

    struct t_fake_active {
        t_object x_obj;
        void* x_proxy;
        t_symbol* x_cname;
        int x_right_click;
        int x_on;
        int x_name;
    };

    bool lastFocus = 0;

    t_symbol* lastFocussedName;
    t_symbol* canvasName;
    Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    ~CanvasActiveObject()
    {
        Desktop::getInstance().removeFocusChangeListener(this);
    }

    void update() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        void* patch;
        sscanf(static_cast<t_fake_active*>(ptr)->x_cname->s_name, ".x%lx.c", (unsigned long*)&patch);

        cnv = getMainCanvas(patch);
        if (!cnv)
            return;

        lastFocus = cnv->hasKeyboardFocus(true);
        Desktop::getInstance().addFocusChangeListener(this);

        auto* y = cnv->patch.getPointer();
        char buf[MAXPDSTRING];
        snprintf(buf, MAXPDSTRING - 1, ".x%lx.c", (unsigned long)y);
        buf[MAXPDSTRING - 1] = 0;
        canvasName = pd->generateSymbol(buf);
    };

    void globalFocusChanged(Component* focusedComponent) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (!focusedComponent) {
            pd->enqueueDirectMessages(ptr, "_focus", { canvasName, 0.0f });
            lastFocus = 0;
            return;
        }

        bool shouldHaveFocus = focusedComponent == cnv;

        auto* name = canvasName;
        Canvas* focusedCanvas = nullptr;

        auto* active = static_cast<t_fake_active*>(ptr);

        if (active->x_name) {
            focusedCanvas = dynamic_cast<Canvas*>(focusedComponent);
            if (!focusedCanvas) {
                focusedCanvas = focusedComponent->findParentComponentOfClass<Canvas>();
            }
            if (!focusedCanvas)
                return;

            char buf[MAXPDSTRING];
            snprintf(buf, MAXPDSTRING - 1, ".x%lx", (unsigned long)focusedCanvas->patch.getPointer());
            buf[MAXPDSTRING - 1] = 0;

            name = gensym(buf);

            if (lastFocussedName != name) {
                pd->enqueueDirectMessages(ptr, "_focus", { canvasName, static_cast<float>(shouldHaveFocus) });
                lastFocussedName = name;
            }
            return;
        }

        if (shouldHaveFocus != lastFocus) {
            pd->enqueueDirectMessages(ptr, "_focus", { canvasName, static_cast<float>(shouldHaveFocus) });
            lastFocus = shouldHaveFocus;
        }
    }
};

class CanvasMouseObject final : public ImplementationBase
    , public MouseListener
    , public pd::MessageListener {

    struct t_fake_canvas_mouse {
        t_object x_obj;
        void* x_proxy;
        t_outlet* x_outlet_x;
        t_outlet* x_outlet_y;
        t_canvas* x_canvas;
        int x_edit;
        int x_pos;
        int x_offset_x;
        int x_offset_y;
        int x_x;
        int x_y;
    };

    std::atomic<bool> zero = false;
    Point<int> lastPosition;
    Point<int> zeroPosition;
    Component::SafePointer<Canvas> cnv;
    Component::SafePointer<Canvas> parentCanvas;

public:
    using ImplementationBase::ImplementationBase;
    CanvasMouseObject(void* ptr, PluginProcessor* pd)
        : ImplementationBase(ptr, pd)
    {
        pd->registerMessageListener(ptr, this);
    }

    ~CanvasMouseObject()
    {

        pd->unregisterMessageListener(ptr, this);
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

        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        binbuf_gettext(mouse->x_obj.te_binbuf, &text, &size);

        int depth = 0;
        for (auto& arg : StringArray::fromTokens(String::fromUTF8(text, size), false)) {
            if (arg.containsOnly("0123456789")) {
                depth = arg.getIntValue();
                break;
            }
        }

        if (depth > 0) {
            cnv = getMainCanvas(mouse->x_canvas->gl_owner);
        } else {
            cnv = getMainCanvas(mouse->x_canvas);
        }

        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

        if (!cnv)
            return;

        cnv->addMouseListener(this, true);
    }

    bool getMousePos(MouseEvent const& e, Point<int>& pos)
    {
        auto relativeEvent = e.getEventRelativeTo(cnv);

        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);
        auto* x = mouse->x_canvas;

        pos = cnv->getLocalPoint(e.originalComponent, e.getPosition()) - cnv->canvasOrigin;

        bool positionChanged = lastPosition != pos;

        lastPosition = pos;

        if (mouse->x_pos) {
            pos -= Point<int>(x->gl_obj.te_xpix, x->gl_obj.te_ypix);
        }

        return positionChanged;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (!cnv || !getValue<bool>(cnv->locked))
            return;

        Point<int> pos;
        getMousePos(e, pos);

        pos -= zeroPosition;

        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        outlet_float(mouse->x_outlet_y, (float)pos.y);
        outlet_float(mouse->x_outlet_x, (float)pos.x);
        outlet_float(mouse->x_obj.ob_outlet, 1.0);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (!cnv || !getValue<bool>(cnv->locked))
            return;

        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);
        outlet_float(mouse->x_obj.ob_outlet, 0.0f);
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (!cnv || !getValue<bool>(cnv->locked))
            return;

        Point<int> pos;
        bool positionChanged = getMousePos(e, pos);

        if (zero) {
            zeroPosition = pos;
            zero = false;
        }

        pos -= zeroPosition;

        if (positionChanged) {
            auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

            outlet_float(mouse->x_outlet_y, (float)pos.y);
            outlet_float(mouse->x_outlet_x, (float)pos.x);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }

    void receiveMessage(pd::MessageSymbol const& message, int argc, t_atom* argv) override
    {
        if (!cnv || pd->isPerformingGlobalSync)
            return;

        if (message.hash == hash("zero")) {
            zero = true;
        }
    }
};

class CanvasVisibleObject final : public ImplementationBase
    , public ComponentListener
    , public Timer {
    struct t_fake_canvas_vis {
        t_object x_obj;
        void* x_proxy;
        t_canvas* x_canvas;
    };

    bool lastFocus = 0;
    Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    ~CanvasVisibleObject()
    {
        if (!cnv)
            return;

        cnv->removeComponentListener(this);
    }

    void update() override
    {
        cnv = getMainCanvas(static_cast<t_fake_canvas_vis*>(ptr)->x_canvas);

        if (!cnv)
            return;

        lastFocus = cnv->hasKeyboardFocus(true);
        cnv->addComponentListener(this);
        startTimer(100);
    }

    void updateVisibility()
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (!cnv)
            return;

        if (lastFocus != cnv->isShowing()) {
            auto* vis = static_cast<t_fake_canvas_vis*>(ptr);

            lastFocus = cnv->isShowing();
            outlet_float(vis->x_obj.ob_outlet, static_cast<int>(cnv->isShowing()));
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
    struct t_fake_zoom {
        t_object x_obj;
        void* x_proxy;
        t_canvas* x_canvas;
        int x_zoom;
    };

    float lastScale;
    Value zoomScaleValue;

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

        cnv = getMainCanvas(static_cast<t_fake_zoom*>(ptr)->x_canvas);
        if (!cnv)
            return;

        zoomScaleValue.referTo(cnv->zoomScale);
        zoomScaleValue.addListener(this);
        lastScale = getValue<float>(zoomScaleValue);
    }

    void valueChanged(Value& v) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        float newScale = getValue<float>(zoomScaleValue);
        if (lastScale != newScale) {
            auto* zoom = static_cast<t_fake_zoom*>(ptr);
            outlet_float(zoom->x_obj.ob_outlet, newScale);
            lastScale = newScale;
        }
    }
};

class CanvasEditObject final : public ImplementationBase
    , public Value::Listener {
    struct t_fake_edit {
        t_object x_obj;
        void* x_proxy;
        t_canvas* x_canvas;
        int x_edit;
    };

    bool lastEditMode;
    Component::Component::SafePointer<Canvas> cnv;

public:
    using ImplementationBase::ImplementationBase;

    void update() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (cnv) {
            cnv->locked.removeListener(this);
        }

        cnv = getMainCanvas(static_cast<t_fake_edit*>(ptr)->x_canvas);
        if (!cnv)
            return;

        // Don't use lock method, because that also responds to temporary lock
        lastEditMode = getValue<float>(cnv->locked);
        cnv->locked.addListener(this);
    }
    void valueChanged(Value& v) override
    {
        if (pd->isPerformingGlobalSync)
            return;

        int editMode = getValue<bool>(v) ? 0 : 1;
        if (lastEditMode != editMode) {
            auto* edit = static_cast<t_fake_edit*>(ptr);
            outlet_float(edit->x_obj.ob_outlet, edit->x_edit = editMode);
            lastEditMode = editMode;
        }
    }
};

// Else "mouse" component
class MouseObject final : public ImplementationBase
    , public Timer {
    typedef struct _mouse {
        t_object x_obj;
        int x_hzero;
        int x_vzero;
        int x_zero;
        int x_wx;
        int x_wy;
        t_glist* x_glist;
        t_outlet* x_horizontal;
        t_outlet* x_vertical;
    } t_mouse;

public:
    MouseObject(void* ptr, PluginProcessor* pd)
        : ImplementationBase(ptr, pd)
        , mouseSource(Desktop::getInstance().getMainMouseSource())
    {
        lastPosition = mouseSource.getScreenPosition();
        lastMouseDownTime = mouseSource.getLastMouseDownTime();
        startTimer(timerInterval);
        canvas = static_cast<t_mouse*>(ptr)->x_glist;
    }

    void timerCallback() override
    {
        if (pd->isPerformingGlobalSync)
            return;

        if (lastPosition != mouseSource.getScreenPosition()) {

            auto pos = mouseSource.getScreenPosition();

            pd->enqueueDirectMessages(ptr, "_getscreen", { pos.x, pos.y });

            lastPosition = pos;
        }
        if (mouseSource.isDragging()) {
            if (!isDown) {
                pd->enqueueDirectMessages(ptr, "_up", { 0.0f });
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (mouseSource.getLastMouseDownTime() > lastMouseDownTime) {
            if (!isDown) {
                pd->enqueueDirectMessages(ptr, "_up", { 0.0f });
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (isDown) {
            pd->enqueueDirectMessages(ptr, "_up", { 1.0f });
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
