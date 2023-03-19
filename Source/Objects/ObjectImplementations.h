/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SubpatchImpl : public ImplementationBase
{
public:
    using ImplementationBase::ImplementationBase;

    ~SubpatchImpl()
    {
        closeOpenedSubpatchers();
    }
    
    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms)
    {
        switch(hash(symbol.toRawUTF8())) {
            case hash("vis"): {
                if (atoms[0].getFloat() == 1) {
                    openSubpatch(subpatch);
                } else {
                    closeOpenedSubpatchers();
                }
            }
        }
    }
    
    std::unique_ptr<pd::Patch> subpatch = nullptr;
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
        if(auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor())) {
            // Capture key events for whole window
            editor->addKeyListener(this);
            editor->addModifierKeyListener(this);
        }
    }

    ~KeyObject()
    {
        if(auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor())) {
            editor->removeModifierKeyListener(this);
            editor->removeKeyListener(this);
        }
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
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
            pd_float((t_pd*)ptr, keyCode);
        } else if (type == KeyName) {

            String keyString = key.getTextDescription().fromLastOccurrenceOf(" ", false, false);
            if (!key.getModifiers().isShiftDown())
                keyString = keyString.toLowerCase();

            t_symbol* keysym = pd->generateSymbol(keyString);
            parseKey(keyCode, keysym);

            t_atom argv[2];
            SETFLOAT(argv, 1.0f);
            SETSYMBOL(argv + 1, keysym);

            pd_list((t_pd*)ptr, pd->generateSymbol("list"), 2, argv);
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
        if (!isKeyDown) {
            for (int n = heldKeys.size() - 1; n >= 0; n--) {
                auto key = heldKeys[n];

                bool keyDown = key.getKeyCode() < 0 ? isKeyDown : key.isCurrentlyDown();

                if (!keyDown) {
                    int keyCode = key.getKeyCode();

                    if (type == KeyUp) {
                        t_symbol* dummy;
                        parseKey(keyCode, dummy);
                        pd_float((t_pd*)ptr, keyCode);
                    } else if (type == KeyName) {

                        String keyString = key.getTextDescription().fromLastOccurrenceOf(" ", false, false);
                        if (!key.getModifiers().isShiftDown())
                            keyString = keyString.toLowerCase();

                        t_symbol* keysym = pd->generateSymbol(keyString);
                        parseKey(keyCode, keysym);

                        t_atom argv[2];
                        SETFLOAT(argv, 0.0f);
                        SETSYMBOL(argv + 1, keysym);

                        pd_list((t_pd*)ptr, pd->generateSymbol("list"), 2, argv);
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
            keysym = pd->generateSymbol("Tab");
        else if (keynum == KeyPress::returnKey)
            keysym = pd->generateSymbol("Return");
        else if (keynum == KeyPress::escapeKey)
            keysym = pd->generateSymbol("Escape");
        else if (keynum == KeyPress::spaceKey)
            keysym = pd->generateSymbol("Space");
        else if (keynum == KeyPress::deleteKey)
            keysym = pd->generateSymbol("Delete");

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
    }
};


class CanvasActiveObject final : public ImplementationBase
    , public FocusChangeListener {

        struct t_fake_active{
            t_object         x_obj;
            void            *x_proxy;
            t_symbol        *x_cname;
            int              x_right_click;
            int              x_on;
            int              x_name;
        };
        
    bool lastFocus = 0;

    t_symbol* canvasName;
    Component::SafePointer<Canvas> cnv;

public:
    CanvasActiveObject(void* ptr, PluginProcessor* pd)
        : ImplementationBase(ptr, pd)
    {
        void* patch;
        sscanf(static_cast<t_fake_active*>(ptr)->x_cname->s_name, ".x%lx.c", (unsigned long*)&patch);
        
        cnv = getMainCanvas(patch);
        if(!cnv) return;
        
        lastFocus = cnv->hasKeyboardFocus(true);
        Desktop::getInstance().addFocusChangeListener(this);

        auto* y = cnv->patch.getPointer();
        char buf[MAXPDSTRING];
        snprintf(buf, MAXPDSTRING - 1, ".x%lx.c", (unsigned long)y);
        buf[MAXPDSTRING - 1] = 0;
        canvasName = pd->generateSymbol(buf);
    }

    ~CanvasActiveObject()
    {
        Desktop::getInstance().removeFocusChangeListener(this);
    }

    void globalFocusChanged(Component* focusedComponent) override
    {
        if (!focusedComponent) {
            t_atom args[2];
            SETSYMBOL(args, canvasName);
            SETFLOAT(args + 1, 0);
            pd_typedmess((t_pd*)ptr, pd->generateSymbol("_focus"), 2, args);
            lastFocus = 0;
            return;
        }

        bool shouldHaveFocus = focusedComponent == cnv;

        if (shouldHaveFocus != lastFocus) {
            t_atom args[2];
            SETSYMBOL(args, canvasName);
            SETFLOAT(args + 1, static_cast<float>(shouldHaveFocus));
            pd_typedmess((t_pd*)ptr, pd->generateSymbol("_focus"), 2, args);
            lastFocus = shouldHaveFocus;
        }
    }
};

class CanvasMouseObject final : public ImplementationBase, public MouseListener {

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

    
    Point<int> lastPosition;
    Component::SafePointer<Canvas> cnv;

public:
    CanvasMouseObject(void* ptr, PluginProcessor* pd)
    : ImplementationBase(ptr, pd)
    {
        char* text;
        int size;
        
        binbuf_gettext(static_cast<t_fake_canvas_mouse*>(ptr)->x_obj.te_binbuf, &text, &size);

        int depth = 0;
        for(auto& arg : StringArray::fromTokens(String::fromUTF8(text, size), false))
        {
            if(arg.containsOnly("0123456789"))
            {
                depth = arg.getIntValue();
                break;
            }
        }
        
        if(depth > 0)
        {
            cnv = getMainCanvas(static_cast<t_fake_canvas_mouse*>(ptr)->x_canvas->gl_owner);
        }
        else {
            cnv = getMainCanvas(static_cast<t_fake_canvas_mouse*>(ptr)->x_canvas);
        }
        
        if(!cnv) return;

        cnv->addMouseListener(this, true);
    }

    ~CanvasMouseObject()
    {
        if(!cnv) return;
        
        cnv->removeMouseListener(this);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!cnv || !static_cast<bool>(cnv->locked.getValue()))
            return;

        auto pos = e.getPosition();
        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        outlet_float(mouse->x_outlet_y, (float)pos.y);
        outlet_float(mouse->x_outlet_x, (float)pos.x);
        outlet_float(mouse->x_obj.ob_outlet, 1.0);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!cnv || !static_cast<bool>(cnv->locked.getValue()))
            return;

        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);
        outlet_float(mouse->x_obj.ob_outlet, 0.0f);
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (!cnv || !static_cast<bool>(cnv->locked.getValue()))
            return;

        auto pos = e.getPosition();
        
        if(pos == lastPosition) return;
        
        lastPosition = pos;
        
        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        outlet_float(mouse->x_outlet_y, (float)pos.y);
        outlet_float(mouse->x_outlet_x, (float)pos.x);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
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
    CanvasVisibleObject(void* ptr, PluginProcessor* pd)
        : ImplementationBase(ptr, pd)
    {
        cnv = getMainCanvas(static_cast<t_fake_canvas_vis*>(ptr)->x_canvas);
        
        lastFocus = cnv->hasKeyboardFocus(true);
        cnv->addComponentListener(this);
        startTimer(100);
    }

    ~CanvasVisibleObject()
    {
        if(!cnv) return;
        
        cnv->removeComponentListener(this);
    }

    void updateVisibility()
    {
        if(!cnv) return;
        
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

class CanvasZoomObject final : public ImplementationBase, public Value::Listener {
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
    CanvasZoomObject(void* ptr, PluginProcessor* pd)
    : ImplementationBase(ptr, pd)
    {
        cnv = getMainCanvas(static_cast<t_fake_zoom*>(ptr)->x_canvas);
        if(!cnv) return;
        
        zoomScaleValue.referTo(cnv->editor->getZoomScaleValueForCanvas(cnv));
        zoomScaleValue.addListener(this);
        lastScale = static_cast<float>(zoomScaleValue.getValue());
    }

    void valueChanged(Value& v) override
    {
        float newScale = static_cast<float>(zoomScaleValue.getValue());
        if (lastScale != newScale) {
            auto* zoom = static_cast<t_fake_zoom*>(ptr);
            outlet_float(zoom->x_obj.ob_outlet, newScale);
            lastScale = newScale;
        }
    }
};

class CanvasEditObject final : public ImplementationBase, public Value::Listener {
    struct t_fake_edit {
        t_object x_obj;
        void* x_proxy;
        t_canvas* x_canvas;
        int x_edit;
    };

    bool lastEditMode;
    Component::Component::SafePointer<Canvas> cnv;

public:
    CanvasEditObject(void* ptr, PluginProcessor* pd)
    : ImplementationBase(ptr, pd)
    {
        cnv = getMainCanvas(static_cast<t_fake_edit*>(ptr)->x_canvas);
        if(!cnv) return;
        
        // Don't use lock method, because that also responds to temporary lock
        lastEditMode = static_cast<float>(cnv->locked.getValue());
        cnv->locked.addListener(this);
    }

    void valueChanged(Value& v) override
    {
        bool editMode = static_cast<bool>(cnv->locked.getValue());
        if (lastEditMode != editMode) {
            auto* edit = static_cast<t_fake_edit*>(ptr);
            outlet_float(edit->x_obj.ob_outlet, edit->x_edit = editMode);
            lastEditMode = editMode;
        }
    }
};
