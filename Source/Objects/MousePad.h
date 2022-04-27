
// ELSE mousepad
struct MousePad : public GUIComponent
{
    bool isLocked = false;
    bool isPressed = false;
    
    typedef struct _pad
    {
        t_object x_obj;
        t_glist* x_glist;
        void* x_proxy;  // dont have this object and dont need it
        t_symbol* x_bindname;
        int x_x;
        int x_y;
        int x_w;
        int x_h;
        int x_sel;
        int x_zoom;
        int x_edit;
        unsigned char x_color[3];
    } t_pad;
    
    MousePad(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
        
        // setInterceptsMouseClicks(box->locked, box->locked);
        
        addMouseListener(box, false);
    }
    
    ~MousePad()
    {
        removeMouseListener(box);
        Desktop::getInstance().removeGlobalMouseListener(this);
    }
    
    void paint(Graphics& g) override{
        
    };
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!getScreenBounds().contains(e.getScreenPosition()) || !isLocked) return;
        
        auto* x = static_cast<t_pad*>(gui.getPointer());
        t_atom at[3];
        
        auto relativeEvent = e.getEventRelativeTo(this);
        
        // int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
        x->x_x = (relativeEvent.getPosition().x / (float)getWidth()) * 127.0f;
        x->x_y = (relativeEvent.getPosition().y / (float)getHeight()) * 127.0f;
        
        SETFLOAT(at, 1.0f);
        sys_lock();
        outlet_anything(x->x_obj.ob_outlet, gensym("click"), 1, at);
        sys_unlock();
        
        isPressed = true;
        
        // glist_grab(x->x_glist, &x->x_obj.te_g, (t_glistmotionfn)pad_motion, 0, (float)xpix, (float)ypix);
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        mouseMove(e);
    }
    
    void mouseMove(const MouseEvent& e) override
    {
        if (!getScreenBounds().contains(e.getScreenPosition()) || !isLocked) return;
        
        auto* x = static_cast<t_pad*>(gui.getPointer());
        t_atom at[3];
        
        auto relativeEvent = e.getEventRelativeTo(this);
        
        // int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
        x->x_x = (relativeEvent.getPosition().x / (float)getWidth()) * 127.0f;
        x->x_y = (relativeEvent.getPosition().y / (float)getHeight()) * 127.0f;
        
        SETFLOAT(at, x->x_x);
        SETFLOAT(at + 1, x->x_y);
        
        sys_lock();
        outlet_anything(x->x_obj.ob_outlet, &s_list, 2, at);
        sys_unlock();
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if (!getScreenBounds().contains(e.getScreenPosition()) && !isPressed) return;
        
        auto* x = static_cast<t_pad*>(gui.getPointer());
        t_atom at[1];
        SETFLOAT(at, 0);
        outlet_anything(x->x_obj.ob_outlet, gensym("click"), 1, at);
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
    }

};
