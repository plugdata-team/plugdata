/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


/*adding a second outlet for minimum value's idx, adding minimum_free, rewriting minimum_list
plus, de-fitter and loud-izing! basically, rewrote the whole thing...

NOTE: in min, sending a list with one float totally borks up the test value, decided to set min value to this instead
also, the test value set from a list retains its index when floats are sent after
(if test < float input, old idx from list gets sent out right inlet)
here,  just set to 1

-alexandre notes that the test value from a list is the min of the list not counting the first elt, despite what the docs say (test value being the second smallest). I'm following the docs in this case.

- Derek Kwan 2016
*/

#include "m_pd.h"
#include <common/api.h>

#define PDCYMINN 256 //max numbers allowable in list

typedef struct _minimum
{
    t_object  x_ob;
    t_float   x_lastmin;
    t_float   x_lastidx;
    t_float   x_test;
    t_outlet * x_minlet;
    t_outlet * x_idxlet; 
} t_minimum;

static t_class *minimum_class;

static void minimum_bang(t_minimum *x)
{
    outlet_float(x->x_idxlet, x->x_lastidx);
    outlet_float(x->x_minlet, x->x_lastmin);
}

static void minimum_float(t_minimum *x, t_float f)
{   x->x_lastidx = f < x->x_test ? 0 : 1;
    x->x_lastmin = f < x->x_test ? f : x->x_test;
    outlet_float(x->x_idxlet, x->x_lastidx);
    outlet_float(x->x_minlet, x->x_lastmin);
		
}

static void minimum_list(t_minimum *x, t_symbol *s, int argc, t_atom *argv){
    if(argc && argc <= PDCYMINN){
        int numfloats = 0; //counting floats, mainly we're interested if there's more than one float - DK
        t_float second = 0;//second smallest
        t_float first = 0;//smallest
        int idx = 0; //current index 
        int fidx = 0; //index of smallest
        t_float curf = 0; //declare here so accessible outside while
        while(argc){
            if(argv->a_type == A_FLOAT){
                numfloats++;
                curf = atom_getfloatarg(0, argc, argv);
                if(numfloats == 1){
                    second = curf;
                    first = curf;
                    fidx = idx;
                }
                else{
                    if(curf < first){
                        second = first;
                        first = curf;
                        fidx = idx;
                    }
                    else if(curf < second || numfloats == 2){
                        second = curf;
                    };
                };

            };
            argc--;
            argv++;
            idx++;
        };
        if(numfloats >= 1){
            x->x_lastidx = fidx;
            x->x_test = second;
            x->x_lastmin = first;
            outlet_float(x->x_idxlet, x->x_lastidx);
            outlet_float(x->x_minlet, x->x_lastmin);

        };
        
    };

}

static void *minimum_free(t_minimum *x)
{
	outlet_free(x->x_minlet);
	outlet_free(x->x_idxlet);
	return (void *)x;
}

static void *minimum_new(t_floatarg f)
{
    t_minimum *x = (t_minimum *)pd_new(minimum_class);
    x->x_lastmin = 0;  /* CHECKME */
    x->x_lastidx = 0;
    x->x_test = f;
    floatinlet_new((t_object *)x, &x->x_test);
    x->x_minlet = outlet_new((t_object *)x, &s_float);
    x->x_idxlet = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void minimum_setup(void)
{
    minimum_class = class_new(gensym("minimum"),
			      (t_newmethod)minimum_new, (t_method)minimum_free,
			      sizeof(t_minimum), 0, A_DEFFLOAT, 0);
    class_addbang(minimum_class, minimum_bang);
    class_addfloat(minimum_class, minimum_float);
    class_addlist(minimum_class, minimum_list);
}
