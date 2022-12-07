/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//de-loudized - Derek Kwan 2016

#include "m_pd.h"
#include <common/api.h>

#define SPRAY_MINOUTS  1
/* CHECKED: no upper limit */
#define SPRAY_DEFOUTS  2

typedef struct _spray
{
    t_object    x_ob;
    int         x_offset;
    int         x_mode; //list mode, 0 = reg, 1 = output list all in one oulet
    int         x_nouts;
    t_outlet  **x_outs;
} t_spray;

static t_class *spray_class;

static void spray_float(t_spray *x, t_float f)
{
    pd_error(x, "spray: no method for float");
}

/* LATER decide, whether float in first atom is to be truncated,
   or causing a list to be ignored as in max (CHECKED) */
static void spray_list(t_spray *x, t_symbol *s, int ac, t_atom *av)
{
    int ndx;
    if(ac >= 2 && av->a_type == A_FLOAT
            /* CHECKED: lists with negative effective ndx are ignored */
            && (ndx = (int)av->a_w.w_float - x->x_offset) >= 0
            && ndx < x->x_nouts){
        //normal legacy listmode, spray
        if (!x->x_mode){
            /* CHECKED: ignored atoms (symbols and floats) are counted */
            /* CHECKED: we must spray in right-to-left order */
            t_atom *argp;
            t_outlet **outp;
            int last = ac - 1 + ndx;  /* ndx of last outlet filled (first is 1) */
            if (last > x->x_nouts)
            {
                argp = av + 1 + x->x_nouts - ndx;
                outp = x->x_outs + x->x_nouts;
            }
            else
            {
                argp = av + ac;
                outp = x->x_outs + last;
            }
            /* argp/outp now point to one after the first atom/outlet to deliver */
            for (argp--, outp--; argp > av; argp--, outp--)
                if (argp->a_type == A_FLOAT)
                    outlet_float(*outp, argp->a_w.w_float);
                else if(argp->a_type == A_SYMBOL)
                    outlet_symbol(*outp, argp->a_w.w_symbol);
        }
        else{
            //new listmode, send entire list out outlet specified by first elt
               outlet_list(x->x_outs[ndx],  &s_list, ac-1, av+1);


        };
    };
}

static void spray_free(t_spray *x)
{
    if (x->x_outs)
	freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void spray_offset(t_spray *x, t_float f){
    x->x_offset = f;
}

static void spray_listmode(t_spray *x, t_float f){
    x->x_mode = f > 0 ? 1 : 0;
}

static void *spray_new(t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
    t_spray *x  = (t_spray *)pd_new(spray_class);
    int i, nouts = (int)f1;
    t_outlet **outs;
    if (nouts < SPRAY_MINOUTS)
        nouts = SPRAY_DEFOUTS;
    if (!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
	return (0);
    x = (t_spray *)pd_new(spray_class);
    x->x_nouts = nouts;
    x->x_outs = outs;
    x->x_offset = (int)f2;
    x->x_mode = f3 > 0 ? 1 : 0;
    for (i = 0; i < nouts; i++)
        x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    return (x);
}

CYCLONE_OBJ_API void spray_setup(void)
{
    spray_class = class_new(gensym("spray"),
			    (t_newmethod)spray_new,
			    (t_method)spray_free,
			    sizeof(t_spray), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    /* CHECKED: bang, symbol, anything -- ``doesn't understand'' */
    class_addfloat(spray_class, spray_float);
    class_addlist(spray_class, spray_list);
    class_addmethod(spray_class, (t_method)spray_listmode, gensym("listmode"), A_FLOAT, 0);
    class_addmethod(spray_class, (t_method)spray_offset, gensym("offset"), A_FLOAT, 0);
}
