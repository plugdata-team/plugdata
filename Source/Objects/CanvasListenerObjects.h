struct CanvasActiveObject final : public TextBase, public FocusChangeListener {

    bool lastFocus = 0;
    
    t_symbol* canvasName;
    
    CanvasActiveObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        lastFocus = cnv->hasKeyboardFocus(true);
        Desktop::getInstance().addFocusChangeListener(this);
        
        auto* y = cnv->patch.getPointer();
        char buf[MAXPDSTRING];
        snprintf(buf, MAXPDSTRING-1, ".x%lx.c", (unsigned long)y);
        buf[MAXPDSTRING-1] = 0;
        canvasName = gensym(buf);
    }
    
    ~CanvasActiveObject() {
        Desktop::getInstance().removeFocusChangeListener(this);
    }
    
    void globalFocusChanged(Component* focusedComponent) override
    {
        if(!focusedComponent) {
            t_atom args[2];
            SETSYMBOL(args, canvasName);
            SETFLOAT(args + 1, 0);
            pd_typedmess((t_pd*)ptr, gensym("_focus"), 2, args);
            lastFocus = 0;
            return;
        }
        
        bool shouldHaveFocus = focusedComponent == cnv || focusedComponent->findParentComponentOfClass<Canvas>() == cnv;
        
        if(shouldHaveFocus != lastFocus) {            
            t_atom args[2];
            SETSYMBOL(args, canvasName);
            SETFLOAT(args + 1, static_cast<float>(shouldHaveFocus));
            pd_typedmess((t_pd*)ptr, gensym("_focus"), 2, args);
            lastFocus = shouldHaveFocus;
        }
    }
    
};

struct CanvasMouseObject final : public TextBase
{
    
    struct t_fake_canvas_mouse
    {
        t_object                x_obj;
        void*                   x_proxy;
        t_outlet*               x_outlet_x;
        t_outlet*               x_outlet_y;
        t_canvas*               x_canvas;
        int                     x_edit;
        int                     x_pos;
        int                     x_offset_x;
        int                     x_offset_y;
        int                     x_x;
        int                     x_y;
    };


    bool lastFocus = 0;
    
    CanvasMouseObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        lastFocus = cnv->hasKeyboardFocus(true);
        setInterceptsMouseClicks(false, false);
        cnv->addMouseListener(this, true);
    }
    
    ~CanvasMouseObject() {
        cnv->removeMouseListener(this);
    }
    
    void mouseDown(MouseEvent const& e) override
    {
        auto pos = e.getPosition();
        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        outlet_float(mouse->x_outlet_y, (float)pos.x);
        outlet_float(mouse->x_outlet_x, (float)pos.y);
        outlet_float(mouse->x_obj.ob_outlet, 1.0);
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);
        outlet_float(mouse->x_obj.ob_outlet, 0.0f);
    }
    
    void mouseMove(MouseEvent const& e) override
    {
        auto pos = e.getPosition();
        auto* mouse = static_cast<t_fake_canvas_mouse*>(ptr);

        outlet_float(mouse->x_outlet_y, (float)pos.x);
        outlet_float(mouse->x_outlet_x, (float)pos.y);
    }
    
    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }
};


struct CanvasVisibleObject final : public TextBase, public ComponentListener, public Timer
{
    struct t_fake_canvas_vis{
        t_object            x_obj;
        void*               x_proxy;
        t_canvas*           x_canvas;
    };


    bool lastFocus = 0;
    
    CanvasVisibleObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        lastFocus = cnv->hasKeyboardFocus(true);
        setInterceptsMouseClicks(false, false);
        cnv->addComponentListener(this);
        startTimer(100);
    }
    
    ~CanvasVisibleObject() {
        cnv->addComponentListener(this);
    }
    
    void updateVisibility() {
        if(lastFocus != cnv->isShowing()) {
            auto* vis = static_cast<t_fake_canvas_vis*>(ptr);
           
            lastFocus = cnv->isShowing();
            outlet_float(vis->x_obj.ob_outlet, static_cast<int>(cnv->isShowing()));
        }
    }
    
    void componentBroughtToFront (Component &component) override
    {
        updateVisibility();
    }
    
    void componentVisibilityChanged (Component &component) override
    {
        updateVisibility();
    }
    
    void timerCallback() override {
        updateVisibility();
    }
};


struct CanvasZoomObject final : public TextBase, public Value::Listener
{
    struct t_fake_zoom{
        t_object        x_obj;
        void*   x_proxy;
        t_canvas*       x_canvas;
        int            x_zoom;
    };

    
    float lastScale;
    
    CanvasZoomObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        lastScale = static_cast<float>(cnv->main.zoomScale.getValue());
        cnv->main.zoomScale.addListener(this);
    }
    
    void valueChanged(Value& v) override {
        
        float newScale = static_cast<float>(cnv->main.zoomScale.getValue());
        if(lastScale != newScale) {
            auto* zoom = static_cast<t_fake_zoom*>(ptr);
            outlet_float(zoom->x_obj.ob_outlet, newScale);
            lastScale = newScale;
        }
    }
};

struct CanvasEditObject final : public TextBase, public Value::Listener
{
    struct t_fake_edit
    {
        t_object        x_obj;
        void*   x_proxy;
        t_canvas*       x_canvas;
        int            x_edit;
    };
    
    bool lastEditMode;
    
    CanvasEditObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        // Don't use lock method, because that also responds to temporary lock
        lastEditMode = static_cast<float>(pd->locked.getValue());
        pd->locked.addListener(this);
    }
    
    void valueChanged(Value& v) override {
        
        bool editMode = static_cast<bool>(pd->locked.getValue());
        if(lastEditMode != editMode) {
            auto* edit = static_cast<t_fake_edit*>(ptr);
            outlet_float(edit->x_obj.ob_outlet, edit->x_edit = editMode);
            lastEditMode = editMode;
        }
    }
};
