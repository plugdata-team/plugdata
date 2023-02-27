/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Wrapper for Pd's key, keyup and keyname objects
class KeyObject final : public TextBase
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
