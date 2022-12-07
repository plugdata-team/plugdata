/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


/*adding a second outlet for maximum value's idx, adding maximum_free, rewriting maximum_list
plus, de-fitter and loud-izing! basically rewrote the whole thing...

NOTE: in max, sending a list with one float totally borks up the test value, decided to set max value to this instead
also, the test value set from a list retains its index when floats are sent after
(if test > float input, old idx from list gets sent out right inlet)
here,  just set to 1

-alexandre notes that the test value from a list is the max of the list not counting the first elt, despite what the docs say (test value being the second largest). I'm following the docs in this case.

- Derek Kwan 2016
*/

#include "m_pd.h"
#include <common/api.h>

#define PDCYMAXN 256 //max numbers allowable in list

typedef struct _maximum
{
    t_object  x_ob;
    t_float   x_lastmax;
    t_float   x_lastidx;
    t_float   x_test;
    t_outlet * x_maxlet;
    t_outlet * x_idxlet; 
} t_maximum;

static t_class *maximum_class;

static void maximum_bang(t_maximum *x)
{
    outlet_float(x->x_idxlet, x->x_lastidx);
    outlet_float(x->x_maxlet, x->x_lastmax);
}

static void maximum_float(t_maximum *x, t_float f)
{   x->x_lastidx = f > x->x_test ? 0 : 1;
    x->x_lastmax = f > x->x_test ? f : x->x_test;
    outlet_float(x->x_idxlet, x->x_lastidx);
    outlet_float(x->x_maxlet, x->x_lastmax);
		
}

static void maximum_list(t_maximum *x, t_symbol *s, int argc, t_atom *argv){
    if(argc && argc <= PDCYMAXN){
        int numfloats = 0; //counting floats, mainly we're interested if there's more than one float - DK
        t_float second = 0;//second largest
        t_float first = 0;//largest
        int idx = 0; //current index 
        int fidx = 0; //index of largest
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
                    if(curf > first){
                        second = first;
                        first = curf;
                        fidx = idx;
                    }
                    else if(curf > second || numfloats == 2){
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
            x->x_lastmax = first;
            outlet_float(x->x_idxlet, x->x_lastidx);
            outlet_float(x->x_maxlet, x->x_lastmax);

        };
        
    };

}

static void *maximum_free(t_maximum *x)
{
	outlet_free(x->x_maxlet);
	outlet_free(x->x_idxlet);
	return (void *)x;
}

static void *maximum_new(t_floatarg f)
{
    t_maximum *x = (t_maximum *)pd_new(maximum_class);
    x->x_lastmax = 0;  /* CHECKME */
    x->x_lastidx = 0;
    x->x_test = f;
    floatinlet_new((t_object *)x, &x->x_test);
    x->x_maxlet = outlet_new((t_object *)x, &s_float);
    x->x_idxlet = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void maximum_setup(void)
{
    maximum_class = class_new(gensym("maximum"),
			      (t_newmethod)maximum_new, (t_method)maximum_free,
			      sizeof(t_maximum), 0, A_DEFFLOAT, 0);
    class_addbang(maximum_class, maximum_bang);
    class_addfloat(maximum_class, maximum_float);
    class_addlist(maximum_class, maximum_list);
}
