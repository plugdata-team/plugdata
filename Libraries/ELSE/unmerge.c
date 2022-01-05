
#include "m_pd.h"

typedef struct _unmerge{
    t_object    x_obj;
    int         x_nouts; //number of outlets not including extra outlet
    float       x_size;
    int         x_trim;
    t_outlet  **x_outlets; //nouts + 1 for extra outlet
}t_unmerge;

static t_class *unmerge_class;

static void unmerge_list(t_unmerge *x, t_symbol *s, int ac, t_atom *av){
    int size = x->x_size < 1 ? 1 : x->x_size;
    int nouts = x->x_nouts;
    int length = size * nouts;
    int extra = (ac - length);
    if(extra > 0){ // extra outlet
        if(extra == 1 && av->a_type == A_FLOAT)
            outlet_float(x->x_outlets[nouts], (av+length)->a_w.w_float);
        else if(av->a_type == A_FLOAT) // if first is float... out list
            outlet_list(x->x_outlets[nouts],  &s_list, extra, av+length);
        else{
            if(!x->x_trim)
                outlet_anything(x->x_outlets[nouts], &s_list, extra, av+length);
            else{
                s = atom_getsymbolarg(0, length, av+length);
                outlet_anything(x->x_outlets[nouts], s, extra-1, av+length+1);
            }
        }
        ac -= extra;
    };
    for(int i = (nouts - 1); i >= 0; i--){
        int j = (i * size);
        int n = ac - j;
        if(n > 0){
            if(n == 1 && av->a_type == A_FLOAT)
                outlet_float(x->x_outlets[i], (av+j)->a_w.w_float);
            else if(av->a_type == A_FLOAT)
                outlet_list(x->x_outlets[i],  &s_list, n, av+j);
            else if(!x->x_trim)
                outlet_anything(x->x_outlets[i],  &s_list, n, av+j);
            else{
                s = atom_getsymbolarg(0, n, av+j);
                outlet_anything(x->x_outlets[i], s, n-1, av+j+1);
            }
        }
        else
            n = 0;
        ac -= n;
    }
}

static void unmerge_anything(t_unmerge * x, t_symbol *s, int ac, t_atom * av){
    t_atom *newlist = t_getbytes((ac+1) * sizeof(*newlist));
    SETSYMBOL(&newlist[0], s);
    for(int i = 0; i < ac; i++)
        newlist[i+1] = av[i];
    unmerge_list(x, NULL, ac+1, newlist);
    t_freebytes(newlist, (ac+1) * sizeof(*newlist));
}

static void unmerge_float(t_unmerge *x, t_float f){
    outlet_float(x->x_outlets[0], f);
}

static void unmerge_symbol(t_unmerge *x, t_symbol *s){
    outlet_symbol(x->x_outlets[0], s);
}

static void unmerge_free(t_unmerge *x){
    if (x->x_outlets)
        freebytes(x->x_outlets, (x->x_nouts+1) * sizeof(*x->x_outlets));
}

static void *unmerge_new(t_symbol *s, int ac, t_atom* av){
    t_unmerge *x = (t_unmerge *)pd_new(unmerge_class);
    t_symbol *dummy = s;
    dummy = NULL;
    int n = 0;
    x->x_size = 0;
/////////////////////////////////////////////////////////////////////////////////////
    if(ac <= 3){
        int argnum = 0;
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                t_float aval = atom_getfloatarg(0, ac, av);
                switch(argnum){
                    case 0:
                        n = (int)aval;
                        break;
                    case 1:
                        x->x_size = aval;
                        break;
                    default:
                        break;
                };
                ac--;
                av++;
                argnum++;
            }
            else if(av->a_type == A_SYMBOL){
                t_symbol *curarg = atom_getsymbolarg(0, ac, av);
                if(curarg == gensym("-trim")){
                    x->x_trim = 1;
                    ac--;
                    av++;
                    argnum++;
                }
                else
                    goto errstate;
            }
        };
    }
/////////////////////////////////////////////////////////////////////////////////////
    x->x_nouts = n < 2 ? 2 : n > 512 ? 512 : n;
    x->x_outlets = (t_outlet **)getbytes((x->x_nouts+1) * sizeof(t_outlet *));
    floatinlet_new(&x->x_obj, &x->x_size);
    for(int i = 0; i <= x->x_nouts; i++)
        x->x_outlets[i] = outlet_new(&x->x_obj, &s_anything);
    return(x);
    errstate:
        pd_error(x, "[unmerge]: improper args");
        return NULL;
}

void unmerge_setup(void){
    unmerge_class = class_new(gensym("unmerge"), (t_newmethod)unmerge_new,
        (t_method)unmerge_free, sizeof(t_unmerge), 0, A_GIMME, 0);
    class_addfloat(unmerge_class, unmerge_float);
    class_addlist(unmerge_class, unmerge_list);
    class_addanything(unmerge_class, unmerge_anything);
    class_addsymbol(unmerge_class, unmerge_symbol);
}

