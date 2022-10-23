
// Wrapper for Pd's key, keyup and keyname objects
struct KeyObject final : public TextBase, public KeyListener {

    enum KeyObjectType
    {
        Key,
        KeyUp,
        KeyName
    };

    KeyObjectType type;
    std::vector<KeyPress> heldKeys;
    
    KeyObject(void* ptr, Object* object, KeyObjectType keyObjectType)
        : TextBase(ptr, object), type(keyObjectType)
    {
        cnv->addKeyListener(this);
    }

    ~KeyObject()
    {
        cnv->removeKeyListener(this);
    }
    
    bool keyPressed (const KeyPress& key, Component* originatingComponent) override {
        
        heldKeys.push_back(key);
        
        String keyString = key.getTextDescription();
        if(!key.getModifiers().isShiftDown()) keyString.toLowerCase();

        t_symbol* keysym = gensym(keyString.toRawUTF8());
        int keyCode = key.getKeyCode();
        parseKey(keyCode, keysym);
        
        
        if(type == Key) {
            pd_float((t_pd*)ptr, keyCode);
        }
        else if(type == KeyName) {
            t_atom argv[2];
            SETFLOAT(argv, 1.0f);
            SETSYMBOL(argv + 1, keysym);
            
            pd_list((t_pd*)ptr, gensym("list"), 2, argv);
        }
        
        // Never claim the keypress
        return false;
    }
    
    bool keyStateChanged (bool isKeyDown, Component *originatingComponent) override
    {

        if(!isKeyDown) {
            
            for(int n = heldKeys.size() - 1; n >= 0; n--) {
                auto& key = heldKeys[n];

                if(!key.isCurrentlyDown()) {
                    t_symbol* keysym = gensym(key.getTextDescription().toRawUTF8());
                    int keyCode = key.getKeyCode();
                    parseKey(keyCode, keysym);
                    
                    if(type == KeyUp) {
                        pd_float((t_pd*)ptr, keyCode);
                    }
                    else if(type == KeyName) {
                        t_atom argv[2];
                        SETFLOAT(argv, 0.0f);
                        SETSYMBOL(argv + 1, keysym);
                        
                        pd_list((t_pd*)ptr, gensym("list"), 2, argv);
                    }
                    
                    heldKeys.erase(heldKeys.begin() + n);
                }
            }
            

        }
        
        // Never claim the keychange
        return false;
    }
    
    
    void parseKey(int& keynum, t_symbol*& keysym) {
        if (keynum == 8)   keysym = gensym("BackSpace");
        if (keynum == 9)   keysym = gensym("Tab");
        if (keynum == 10)  keysym = gensym("Return");
        if (keynum == 27)  keysym = gensym("Escape");
        if (keynum == 32)  keysym = gensym("Space");
        if (keynum == 127) keysym = gensym("Delete");
        
        if (keynum == 30 || keynum == 63232)
            keynum = 0, keysym = gensym("Up");
        else if (keynum == 31 || keynum == 63233)
            keynum = 0, keysym = gensym("Down");
        else if (keynum == 28 || keynum == 63234)
            keynum = 0, keysym = gensym("Left");
        else if (keynum == 29 || keynum == 63235)
            keynum = 0, keysym = gensym("Right");
        else if (keynum == 63273)
            keynum = 0, keysym = gensym("Home");
        else if (keynum == 63275)
            keynum = 0, keysym = gensym("End");
        else if (keynum == 63276)
            keynum = 0, keysym = gensym("Prior");
        else if (keynum == 63277)
            keynum = 0, keysym = gensym("Next");
        else if (keynum == 63236)
            keynum = 0, keysym = gensym("F1");
        else if (keynum == 63237)
            keynum = 0, keysym = gensym("F2");
        else if (keynum == 63238)
            keynum = 0, keysym = gensym("F3");
        else if (keynum == 63239)
            keynum = 0, keysym = gensym("F4");
        else if (keynum == 63240)
            keynum = 0, keysym = gensym("F5");
        else if (keynum == 63241)
            keynum = 0, keysym = gensym("F6");
        else if (keynum == 63242)
            keynum = 0, keysym = gensym("F7");
        else if (keynum == 63243)
            keynum = 0, keysym = gensym("F8");
        else if (keynum == 63244)
            keynum = 0, keysym = gensym("F9");
        else if (keynum == 63245)
            keynum = 0, keysym = gensym("F10");
        else if (keynum == 63246)
            keynum = 0, keysym = gensym("F11");
        else if (keynum == 63247)
            keynum = 0, keysym = gensym("F12");
    }
};
