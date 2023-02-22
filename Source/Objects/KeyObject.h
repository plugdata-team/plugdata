/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Wrapper for Pd's key, keyup and keyname objects
class KeyObject final : public TextBase
    , public KeyListener, public ModifierKeyListener {

    Array<KeyPress> heldKeys;

public:
    enum KeyObjectType {
        Key,
        KeyUp,
        KeyName
    };
        
    const int shiftKey = -1;
    const int commandKey = -2;
    const int altKey = -3;
    const int ctrlKey = -4;
    

    KeyObjectType type;

    KeyObject(void* ptr, Object* object, KeyObjectType keyObjectType)
        : TextBase(ptr, object)
        , type(keyObjectType)
    {
        // Capture key events for whole window
        cnv->editor->addKeyListener(this);
        cnv->editor->addModifierKeyListener(this);
    }

    ~KeyObject()
    {
        cnv->editor->removeModifierKeyListener(this);
        cnv->editor->removeKeyListener(this);
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        heldKeys.add(key);

        int keyCode = key.getKeyCode();
        
        if (type == Key) {
            pd_float((t_pd*)ptr, keyCode);
        } else if (type == KeyName) {
            
            String keyString = key.getTextDescription().fromLastOccurrenceOf("+ ", false, false);
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
        if(isHeld) keyPressed(KeyPress(shiftKey), nullptr);
        else keyStateChanged(false, nullptr);
    }
    void commandKeyChanged(bool isHeld) override
    {
        if(isHeld) keyPressed(KeyPress(commandKey), nullptr);
        else keyStateChanged(false, nullptr);
    }
    void altKeyChanged(bool isHeld) override
    {
        if(isHeld) keyPressed(KeyPress(altKey), nullptr);
        else keyStateChanged(false, nullptr);
    }
    void ctrlKeyChanged(bool isHeld) override
    {
        if(isHeld) keyPressed(KeyPress(ctrlKey), nullptr);
        else keyStateChanged(false, nullptr);
    }
        
    void spaceKeyChanged(bool isHeld) override
    {
        if(isHeld) keyPressed(KeyPress(KeyPress::spaceKey), nullptr);
        else keyStateChanged(false, nullptr);
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
                        pd_float((t_pd*)ptr, keyCode);
                    } else if (type == KeyName) {
                        
                        String keyString = key.getTextDescription().fromLastOccurrenceOf("+ ", false, false);
                        if (!key.getModifiers().isShiftDown())
                            keyString = keyString.toLowerCase();

                        t_symbol* keysym = pd->generateSymbol(keyString);
                        parseKey(keyCode, keysym);
                        
                        t_atom argv[2];
                        SETFLOAT(argv, 0.0f);
                        SETSYMBOL(argv + 1, keysym);

                        pd_list((t_pd*)ptr, pd->generateSymbol("list"), 2, argv);
                    }
                    
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
        }
        else if (keynum == commandKey) {
            keysym = pd->generateSymbol("Meta_L");
            keynum = 0;
        }
        else if (keynum == altKey) {
            keysym = pd->generateSymbol("Alt_L");
            keynum = 0;
        }
        else if (keynum == ctrlKey) {
            keysym = pd->generateSymbol("Control_L");
            keynum = 0;
        }
        else if (keynum == 8)
            keysym = pd->generateSymbol("BackSpace");
        else if (keynum == 9)
            keysym = pd->generateSymbol("Tab");
        else if (keynum == 10)
            keysym = pd->generateSymbol("Return");
        else if (keynum == 27)
            keysym = pd->generateSymbol("Escape");
        else if (keynum == 32)
            keysym = pd->generateSymbol("Space");
        else if (keynum == 127)
            keysym = pd->generateSymbol("Delete");

        else if (keynum == 30 || keynum == 63232)
            keynum = 0, keysym = pd->generateSymbol("Up");
        else if (keynum == 31 || keynum == 63233)
            keynum = 0, keysym = pd->generateSymbol("Down");
        else if (keynum == 28 || keynum == 63234)
            keynum = 0, keysym = pd->generateSymbol("Left");
        else if (keynum == 29 || keynum == 63235)
            keynum = 0, keysym = pd->generateSymbol("Right");
        else if (keynum == 63273)
            keynum = 0, keysym = pd->generateSymbol("Home");
        else if (keynum == 63275)
            keynum = 0, keysym = pd->generateSymbol("End");
        else if (keynum == 63276)
            keynum = 0, keysym = pd->generateSymbol("Prior");
        else if (keynum == 63277)
            keynum = 0, keysym = pd->generateSymbol("Next");
        else if (keynum == 63236)
            keynum = 0, keysym = pd->generateSymbol("F1");
        else if (keynum == 63237)
            keynum = 0, keysym = pd->generateSymbol("F2");
        else if (keynum == 63238)
            keynum = 0, keysym = pd->generateSymbol("F3");
        else if (keynum == 63239)
            keynum = 0, keysym = pd->generateSymbol("F4");
        else if (keynum == 63240)
            keynum = 0, keysym = pd->generateSymbol("F5");
        else if (keynum == 63241)
            keynum = 0, keysym = pd->generateSymbol("F6");
        else if (keynum == 63242)
            keynum = 0, keysym = pd->generateSymbol("F7");
        else if (keynum == 63243)
            keynum = 0, keysym = pd->generateSymbol("F8");
        else if (keynum == 63244)
            keynum = 0, keysym = pd->generateSymbol("F9");
        else if (keynum == 63245)
            keynum = 0, keysym = pd->generateSymbol("F10");
        else if (keynum == 63246)
            keynum = 0, keysym = pd->generateSymbol("F11");
        else if (keynum == 63247)
            keynum = 0, keysym = pd->generateSymbol("F12");
    }
};
