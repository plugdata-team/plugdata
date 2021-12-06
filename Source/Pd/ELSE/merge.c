
#include "m_pd.h"
#include <stdlib.h>
#include <string.h>

static t_class *merge_class;
static t_class* merge_inlet_class;

struct _merge_inlet;

typedef struct _merge{
    t_object              x_obj;
    int                   x_numinlets;
    int                   x_length; // total length of all atoms from merge_inlet
    int                   x_trim;
    struct _merge_inlet*  x_ins;
}t_merge;

typedef struct _merge_inlet{
    t_class*    x_pd;
    t_atom*     x_atoms;
    int         x_numatoms;
    int         x_trig;
    int         x_id;
    t_merge*    x_owner;
}t_merge_inlet;

static void atoms_copy(int ac, t_atom *from, t_atom *to){
    for(int i = 0; i < ac; i++)
        to[i] = from[i];
}

static void merge_output(t_merge *x){
    t_atom * outatom;
    outatom = (t_atom *)getbytes(x->x_length * sizeof(t_atom));
    int offset = 0;
    for(int i = 0; i < x->x_numinlets; i++){
        int curnum = x->x_ins[i].x_numatoms; // number of atoms for current inlet
        atoms_copy(curnum, x->x_ins[i].x_atoms, outatom + offset); // copy them over to outatom
        offset += curnum;
    };
    if(x->x_trim && outatom->a_type == A_SYMBOL){
        t_symbol *selector = atom_getsymbolarg(0, x->x_length, outatom);
        outlet_anything(x->x_obj.ob_outlet, selector, x->x_length-1, outatom+1);
    }
    else
        outlet_list(x->x_obj.ob_outlet, &s_list, x->x_length, outatom);
    freebytes(outatom, x->x_length * sizeof(t_atom));
}

static void merge_inlet_atoms(t_merge_inlet *x, int ac, t_atom * av ){
    t_merge * owner = x->x_owner;
    freebytes(x->x_atoms, x->x_numatoms * sizeof(t_atom));
    owner->x_length -= x->x_numatoms;
    x->x_atoms = (t_atom *)getbytes(ac * sizeof(t_atom));
    owner->x_length += ac;
    x->x_numatoms = (t_int)ac;
    atoms_copy(ac, av, x->x_atoms);
}

static void merge_inlet_list(t_merge_inlet *x, t_symbol* s, int ac, t_atom* av){
    t_symbol *dummy = s;
    dummy = NULL;
    merge_inlet_atoms(x, ac, av);
    if(x->x_trig == 1)
        merge_output(x->x_owner);
}

static void merge_inlet_bang(t_merge_inlet *x){
    merge_output(x->x_owner);
}

static void merge_inlet_anything(t_merge_inlet *x, t_symbol* s, int ac, t_atom* av){
    // we want to treat "bob tom" and "list bob tom" as the same
    // default way is to treat first symbol as selector, we don't want this!
    if(strcmp(s->s_name, "list") != 0){
        t_atom * tofeed = (t_atom *)getbytes((ac+1)*sizeof(t_atom));
        SETSYMBOL(tofeed, s);
        atoms_copy(ac, av, tofeed+1);
        merge_inlet_list(x, 0, ac+1, tofeed);
        freebytes(tofeed, (ac+1)*sizeof(t_atom));
    }
    else
        merge_inlet_list(x, 0, ac, av);
}

static void merge_inlet_float(t_merge_inlet *x, float f){
    t_atom * newatom;
    newatom = (t_atom *)getbytes(1 * sizeof(t_atom));
    SETFLOAT(newatom, f);
    merge_inlet_list(x, 0, 1, newatom);
    freebytes(newatom, 1*sizeof(t_atom));
}

static void merge_inlet_symbol(t_merge_inlet *x, t_symbol* s){
    t_atom * newatom;
    newatom = (t_atom *)getbytes(1 * sizeof(t_atom));
    SETSYMBOL(newatom, s);
    merge_inlet_list(x, 0, 1, newatom);
    freebytes(newatom, 1*sizeof(t_atom));
}

static void* merge_free(t_merge *x){
    for(int i = 0; i < x->x_numinlets; i++)
        freebytes(x->x_ins[i].x_atoms, x->x_ins[i].x_numatoms*sizeof(t_atom));
    freebytes(x->x_ins, x->x_numinlets * sizeof(t_merge_inlet));
    return (void *)free;
}

static void *merge_new(t_symbol *s, int ac, t_atom* av){
    t_merge *x = (t_merge *)pd_new(merge_class);
    t_symbol *dummy = s;
    dummy = NULL;
    t_float numinlets = 2;
    int hot = 0;
    x->x_trim = 0;
    int * triggervals;
    triggervals = (int *)calloc(x->x_numinlets, sizeof(int));
    triggervals[0] = 1;
/////////////////////////////////////////////////////////////////////////////////////
    if(ac <= 3){
        int argnum = 0;
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                t_float aval = atom_getfloatarg(0, ac, av);
                switch(argnum){
                    case 0:
                        numinlets = aval;
                        break;
                    case 1:
                        hot = (int)(aval != 0);
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
    int i;
    int n = (int)numinlets;
    x->x_numinlets = n < 2 ? 2 : n > 512 ? 512 : n;
    if(hot){
        for(i = 0; i < x->x_numinlets; i++)
            triggervals[i] = 1;
    };
    x->x_ins = (t_merge_inlet *)getbytes(x->x_numinlets * sizeof(t_merge_inlet));
    x->x_length = x->x_numinlets;
    for(i = 0; i < x->x_numinlets; ++i){
        x->x_ins[i].x_pd    = merge_inlet_class;
        x->x_ins[i].x_atoms = (t_atom *)getbytes(1 * sizeof(t_atom));
        SETFLOAT(x->x_ins[i].x_atoms, 0);
        x->x_ins[i].x_numatoms = 1;
        x->x_ins[i].x_owner = x;
        x->x_ins[i].x_trig = triggervals[i];
        x->x_ins[i].x_id = i;
        inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
    };
    outlet_new(&x->x_obj, &s_list);
    free(triggervals);
    return (x);
    errstate:
        pd_error(x, "[merge]: improper args");
        return NULL;
}

extern void merge_setup(void){
    t_class* c = NULL;
    c = class_new(gensym("merge-inlet"), 0, 0, sizeof(t_merge_inlet), CLASS_PD, 0);
    if(c){
        class_addbang(c,    (t_method)merge_inlet_bang);
        class_addfloat(c,   (t_method)merge_inlet_float);
        class_addsymbol(c,  (t_method)merge_inlet_symbol);
        class_addlist(c,    (t_method)merge_inlet_list);
        class_addanything(c,(t_method)merge_inlet_anything);
    }
    merge_inlet_class = c;
    c = class_new(gensym("merge"), (t_newmethod)merge_new, (t_method)merge_free,
                  sizeof(t_merge), CLASS_NOINLET, A_GIMME, 0);
    merge_class = c;
}
