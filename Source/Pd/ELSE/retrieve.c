// based on cyclone/grab

#include "m_pd.h"
#include "m_imp.h"

struct _outlet{
    t_object        *o_owner;
    struct _outlet  *o_next;
    t_outconnect    *o_connections;
    t_symbol        *o_sym;
};

t_outconnect *outlet_connections(t_outlet *o){ // obj_starttraverseoutlet() replacement
    return(o ? o->o_connections : 0); // magic
}

t_outconnect *outlet_nextconnection(t_outconnect *last, t_object **destp, int *innop){
    t_inlet *dummy;
    return(obj_nexttraverseoutlet(last, destp, &dummy, innop));
}

static t_class *bindlist_class = 0;

typedef struct _bindelem{
    t_pd                *e_who;
    struct _bindelem    *e_next;
}t_bindelem;

typedef struct _bindlist{
    t_pd b_pd;
    t_bindelem *b_list;
}t_bindlist;

typedef struct _retrieve{
    t_object        x_obj;
    t_symbol       *x_target;           // bound symbol
    t_outconnect  **x_grabcons;         // retrieven connections
// traversal helpers:
    t_object       *x_retrieven;            // currently retrieven object
    t_outconnect   *x_retrieven_connection; // a connection to retrieven object
    t_bindelem     *x_bindelem;
}t_retrieve;

static t_class *retrieve_class;

static void retrieve_start(t_retrieve *x){
    x->x_retrieven_connection = 0;
    x->x_bindelem = 0;
    if(x->x_target){
        t_pd *proxy = x->x_target->s_thing;
        t_object *obj;
        if(proxy && bindlist_class){
            if(*proxy == bindlist_class){
                x->x_bindelem = ((t_bindlist *)proxy)->b_list;
                while(x->x_bindelem){
                    obj = pd_checkobject(x->x_bindelem->e_who);
                    if(obj){
                        x->x_retrieven_connection = outlet_connections(obj->ob_outlet);
                        return;
                    }
                    x->x_bindelem = x->x_bindelem->e_next;
                }
            }
            else if((obj = pd_checkobject(proxy)))
                x->x_retrieven_connection = outlet_connections(obj->ob_outlet);
        }
    }
}

static t_pd *retrieve_next(t_retrieve *x){
nextremote:
    if(x->x_retrieven_connection){
        int inno;
        x->x_retrieven_connection = outlet_nextconnection(x->x_retrieven_connection, &x->x_retrieven, &inno);
        if(x->x_retrieven){
            if(inno){
                if(x->x_target)
                    pd_error(x, "[retrieve]: right outlet must feed leftmost inlet");
                else
                    pd_error(x, "[retrieve]: remote proxy must feed leftmost inlet");
            }
            else{
                t_outlet *op;
                t_outlet *goutp;
                int goutno = 1;
                while(goutno--){
                    x->x_grabcons[goutno] = obj_starttraverseoutlet(x->x_retrieven, &goutp, goutno);
                    goutp->o_connections = obj_starttraverseoutlet((t_object *)x, &op, goutno);
                }
                return((t_pd *)x->x_retrieven);
            }
        }
    }
    if(x->x_bindelem)
        while((x->x_bindelem = x->x_bindelem->e_next)){
            t_object *obj = pd_checkobject(x->x_bindelem->e_who);
            if(obj){
                x->x_retrieven_connection = outlet_connections(obj->ob_outlet);
                goto
                    nextremote;
            }
        }
    return(0);
}

static void retrieve_restore(t_retrieve *x){
    t_outlet *goutp;
    int goutno = 1;
    while(goutno--){
        obj_starttraverseoutlet(x->x_retrieven, &goutp, goutno);
        goutp->o_connections = x->x_grabcons[goutno];
    }
}

static void retrieve_bang(t_retrieve *x){
    t_pd *retrieven;
    retrieve_start(x);
    while((retrieven = retrieve_next(x))){
        pd_bang(retrieven);
        retrieve_restore(x);
    }
}

static void retrieve_set(t_retrieve *x, t_symbol *s){
    if(s && s != &s_)
        x->x_target = s;
}

static void *retrieve_new(t_symbol *s){
    t_retrieve *x = (t_retrieve *)pd_new(retrieve_class);
    t_outconnect **grabcons = getbytes(sizeof(*grabcons));
    if(!grabcons)
        return(0);
    x->x_grabcons = grabcons;
    outlet_new((t_object *)x, &s_anything);
    x->x_target = (s && s != &s_) ? s : 0;
    return(x);
}

static void retrieve_free(t_retrieve *x){
    if(x->x_grabcons)
        freebytes(x->x_grabcons, sizeof(*x->x_grabcons));
}

void retrieve_setup(void){
    t_symbol *s = gensym("retrieve");
    retrieve_class = class_new(s, (t_newmethod)retrieve_new,
        (t_method)retrieve_free, sizeof(t_retrieve), 0, A_DEFSYMBOL, 0);
    class_addbang(retrieve_class, retrieve_bang);
    class_addmethod(retrieve_class, (t_method)retrieve_set, gensym("set"), A_SYMBOL, 0);
    if(!bindlist_class){
        t_class *c = retrieve_class;
        pd_bind(&retrieve_class, s);
        pd_bind(&c, s);
        bindlist_class = *s->s_thing;
        if(!s->s_thing || !bindlist_class || bindlist_class->c_name != gensym("bindlist"))
            error("[retrieve]: failure to initialize remote taking feature");
        pd_unbind(&c, s);
        pd_unbind(&retrieve_class, s);
    }
}
