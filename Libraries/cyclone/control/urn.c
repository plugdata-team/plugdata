/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER think again about avoiding memory allocation overhead in run-time.
   One would need to use a creation argument greater than any future right
   inlet value.  But this is incompatible (max uses a `static', max-size
   array), and should be put somewhere in the docs... */

#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"
#include "control/rand.h"

#define URN_INISIZE      128  // LATER rethink
#define URN_MAXSIZE    65536  // in max is 4096

typedef struct _urn{
    t_object         x_ob;
    int              x_count;
    int              x_doing;
    int              x_size;   // as allocated (in bytes)
    int              x_range;  // as used
    unsigned short  *x_urn;
    unsigned short   x_urnini[URN_INISIZE];
    unsigned int     x_seed;
    t_outlet        *x_bangout;
}t_urn;

static t_class *urn_class;

static int urn_resize(t_urn *x, t_float f){
    int range = (int)f;
    if(range < 1){ // CHECKED (the same for > max)
        pd_error(x, "[urn]: illegal size %.0f", f);
        return(0);
    }
    if(range > URN_MAXSIZE){
        pd_error(x, "[urn]: illegal size %.0f", f);
        return(0);
    }
    x->x_range = range;
    if(range > x->x_size)
        x->x_urn = grow_nodata(&x->x_range, &x->x_size, x->x_urn,
            URN_INISIZE, x->x_urnini, sizeof(*x->x_urn));
    return(1);
}

static void urn_bang(t_urn *x){
    if(x->x_count){
        x->x_doing = 1;
        int ndx = rand_int(&x->x_seed, x->x_count);
        unsigned short pick = x->x_urn[ndx];
        x->x_urn[ndx] = x->x_urn[--x->x_count];
        outlet_float(((t_object *)x)->ob_outlet, pick);
    } /* CHECKED: start banging when the first bang is input
       into an empty urn (and not when the last value is output).
       CHECKED: keep banging until cleared. */
    else{
        x->x_doing = 0;
        outlet_bang(x->x_bangout);
    }
}

static void urn_clear(t_urn *x){
    x->x_count = x->x_range;
    for(int i = 0; i < x->x_count; i++)
        x->x_urn[i] = i;
    x->x_doing = 0;
}

static void urn_ft1(t_urn *x, t_floatarg f){
    if(urn_resize(x, f))  // CHECKED cleared only if a legal resize
        urn_clear(x);
}

static void urn_seed(t_urn *x, t_floatarg f){
    if(!x->x_doing){
        unsigned int i = (unsigned int)f;  // CHECKED
        if(i < 0)
            i = 1;  // CHECKED
        rand_seed(&x->x_seed, i);
    }
}

static void urn_free(t_urn *x){
    if(x->x_urn != x->x_urnini)
        freebytes(x->x_urn, x->x_size * sizeof(*x->x_urn));
}

static void *urn_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_urn *x = (t_urn *)pd_new(urn_class);
    x->x_size = URN_INISIZE;
    x->x_urn = x->x_urnini;
    x->x_doing = 0;
    int size = 1;
    int seed = 0;
    if(ac){
        int argnum = 0; //current argument
        while(ac){
            if(av->a_type == A_FLOAT){
                t_float curf = atom_getfloatarg(0, ac, av);
                switch(argnum){
                    case 0:
                        size = curf;
                        break;
                    case 1:
                        seed = curf;
                        break;
                    default:
                        break;
                };
                argnum++;
            };
            ac--;
            av++;
        };
    }
    if(size < 1)
        size = 1;
    if(size > URN_MAXSIZE)
        size = URN_MAXSIZE;
    if(seed < 0)
        seed = 1;
    urn_resize(x, size);
    urn_seed(x, seed);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    urn_clear(x);
    return(x);
}

CYCLONE_OBJ_API void urn_setup(void){
    urn_class = class_new(gensym("urn"), (t_newmethod)urn_new, (t_method)urn_free,
        sizeof(t_urn), 0, A_GIMME, 0);
    class_addbang(urn_class, urn_bang);
    class_addfloat(urn_class, urn_bang); // in max it doesn't handle floats, just ints
    class_addmethod(urn_class, (t_method)urn_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(urn_class, (t_method)urn_seed, gensym("seed"), A_FLOAT, 0);
    class_addmethod(urn_class, (t_method)urn_clear, gensym("clear"), 0);
}
