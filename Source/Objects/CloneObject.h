
extern "C" {
t_glist* clone_get_instance(t_gobj*, int);
int clone_get_n(t_gobj*);

typedef struct _copy {
    t_glist* c_gl;
    int c_on; /* DSP running */
} t_fake_copy;

typedef struct _in {
    t_class* i_pd;
    struct _clone* i_owner;
    int i_signal;
    int i_n;
} t_fake_in;

typedef struct _out {
    t_class* o_pd;
    t_outlet* o_outlet;
    int o_signal;
    int o_n;
} t_fake_out;

typedef struct _clone {
    t_object x_obj;
    int x_n;            /* number of copies */
    t_fake_copy* x_vec; /* the copies */
    int x_nin;
    t_fake_in* x_invec; /* inlet proxies */
    int x_nout;
    t_fake_out** x_outvec; /* outlet proxies */
    t_symbol* x_s;         /* name of abstraction */
    int x_argc;            /* creation arguments for abstractions */
    t_atom* x_argv;
    int x_phase;
    int x_startvoice;    /* number of first voice, 0 by default */
    int x_suppressvoice; /* suppress voice number as $1 arg */
} t_fake_clone;
}

struct CloneObject final : public TextBase {
    CloneObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch({ nullptr, nullptr })
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (clone_get_n(gobj) > 0) {
            subpatch = { clone_get_instance(gobj, 0), cnv->pd };
        }
    }

    void updateValue() override
    {
        // Pd sometimes sets the isgraph flag too late...
        // In that case we tell the object to create the gui
        if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            object->setType(currentText, ptr);
        }
    };

    ~CloneObject()
    {
        closeOpenedSubpatchers();
    }

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    String getText() override
    {
        return String::fromUTF8(static_cast<t_fake_clone*>(ptr)->x_s->s_name);
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

protected:
    pd::Patch subpatch;
};
