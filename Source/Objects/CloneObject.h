/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
t_glist* clone_get_instance(t_gobj*, int);
int clone_get_n(t_gobj*);

struct t_copy {
    t_glist* c_gl;
    int c_on; /* DSP running */
};

struct t_in {
    t_class* i_pd;
    struct _clone* i_owner;
    int i_signal;
    int i_n;
};

struct t_out {
    t_class* o_pd;
    t_outlet* o_outlet;
    int o_signal;
    int o_n;
};

struct t_fake_clone {
    t_object x_obj;
    t_canvas* x_canvas; /* owning canvas */
    int x_n;            /* number of copies */
    t_copy* x_vec;      /* the copies */
    int x_nin;
    t_in* x_invec; /* inlet proxies */
    int x_nout;
    t_out** x_outvec; /* outlet proxies */
    t_symbol* x_s;    /* name of abstraction */
    int x_argc;       /* creation arguments for abstractions */
    t_atom* x_argv;
    int x_phase;                      /* phase for round-robin input message forwarding */
    int x_startvoice;                 /* number of first voice, 0 by default */
    unsigned int x_suppressvoice : 1; /* suppress voice number as $1 arg */
    unsigned int x_distributein : 1;  /* distribute input signals across clones */
    unsigned int x_packout : 1;       /* pack output signals */
};
}

class CloneObject final : public TextBase {

    pd::Patch subpatch;

public:
    CloneObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch({ nullptr, nullptr })
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (clone_get_n(gobj) > 0) {
            subpatch = { clone_get_instance(gobj, 0), cnv->pd };
        }
    }

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
        auto* sym = static_cast<t_fake_clone*>(ptr)->x_s;
        return sym ? String::fromUTF8(sym->s_name) : String();
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case objectMessage::msg_vis: {
            if (atoms.size() > 2) {
                // TODO: implement this!
            }
            break;
        }
        default:
            break;
        }
    }
};
