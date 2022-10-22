
// Else "mouse" component
struct MouseObject final : public TextBase {
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

    MouseObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
        cnv->addMouseListener(this, true);
        
    }

    ~MouseObject()
    {
        cnv->removeMouseListener(this);
    }

    void mouseDown(MouseEvent const& e) override
    {
        t_atom args[1];
        SETFLOAT(args, 0);

        pd_typedmess((t_pd*)ptr, gensym("_up"), 1, args);
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        t_atom args[1];
        SETFLOAT(args, 1);

        pd_typedmess((t_pd*)ptr, gensym("_up"), 1, args);
    }
    
    void mouseMove(MouseEvent const& e) override
    {
        auto pos = e.getPosition();

        t_atom args[2];
        SETFLOAT(args, pos.x);
        SETFLOAT(args + 1, pos.y);

        pd_typedmess((t_pd*)ptr, gensym("_getscreen"), 2, args);
    }
    
    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }
};
