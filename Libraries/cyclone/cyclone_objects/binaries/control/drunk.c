/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* The first version of this code was written by Nicola Bernardini.
   It was entirely reimplemented in the hope of adapting it to the
   cyclone's guidelines. */

#include "m_pd.h"
#include <common/api.h>
#include "control/rand.h"

#define DRUNK_DEFMAXVALUE  128
#define DRUNK_DEFMAXSTEP   2

typedef struct _drunk
{
    t_object      x_ob;
    int           x_value;
    int           x_maxvalue;
    int           x_maxstep;
    int           x_minstep;
    unsigned int  x_seed;
    unsigned int  x_bitseed;
} t_drunk;

static t_class *drunk_class;

static void drunk_set(t_drunk *x, t_floatarg f)
{
    int i = (int)f;  /* CHECKED float silently truncated */
    if (i > x->x_maxvalue)
	x->x_value = x->x_maxvalue;  /* CHECKED */
    else if (i < 0)
	x->x_value = 0;  /* CHECKED */
    else x->x_value = i;
}

/* CHECKED: this is a superposition of two rngs -- the random bit generator,
   and the random integer generator, which is the same (CHECKED) as the one
   used in the 'random' class (quite lame: period 35730773, nonuniform for
   large ranges, so I would rather not RE it...) */
#define RBIT1      1
#define RBIT2      2
#define RBIT5      16
#define RBIT18     131072
#define RBIT_MASK  (RBIT1 + RBIT2 + RBIT5)

static void drunk_bang(t_drunk *x)
{
    int rnd = rand_int(&x->x_seed, x->x_maxstep) + x->x_minstep;
    int val;
    if (x->x_bitseed & RBIT18)
    {
	x->x_bitseed = ((x->x_bitseed ^ RBIT_MASK) << 1) | RBIT1;
	if ((val = x->x_value + rnd) > x->x_maxvalue)
	    val = x->x_value - rnd;  /* CHECKED */
	if (val < 0)
	    val = 0;  /* CHECKED (needed for maxstep > maxvalue) */
    }
    else
    {
	x->x_bitseed <<= 1;
	if ((val = x->x_value - rnd) < 0)
	    val = x->x_value + rnd;  /* CHECKED */
	if (val > x->x_maxvalue)
	    val = x->x_maxvalue;  /* CHECKED (needed for maxstep > maxvalue) */
    }
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = val);
}

static void drunk_float(t_drunk *x, t_float f)
{
    drunk_set(x, f);
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void drunk_ft1(t_drunk *x, t_floatarg f)
{
    int i = (int)f;  /* CHECKED float silently truncated */
    x->x_maxvalue = (i < 0 ? 1 : i);  /* CHECKED zero allowed */
    /* CHECKED maxstep not updated */
}

static void drunk_ft2(t_drunk *x, t_floatarg f)
{
    int i = (int)f;  /* CHECKED float silently truncated */
    if (i < 0)
    {
	x->x_minstep = 1;
	i = -i;
    }
    else x->x_minstep = 0;
    /* CHECKED maxstep not clipped to the maxvalue */
    x->x_maxstep = (x->x_minstep ? i - 1 : i);  /* CHECKED zero allowed */
}

/* apparently, bitseed cannot be controlled, but LATER recheck */
static void drunk_seed(t_drunk *x, t_floatarg f)
{
    int i = (int)f;  /* CHECKED */
    if (i < 0)
	i = 1;  /* CHECKED */
    rand_seed(&x->x_seed, (unsigned int)i);
}

static void drunk_state(t_drunk *x){
    post("drunk seed: %u", x->x_seed); 
}

static void *drunk_new(t_floatarg f1, t_floatarg f2)
{
    t_drunk *x = (t_drunk *)pd_new(drunk_class);
    x->x_maxvalue = ((int)f1 > 0 ? (int)f1 : 128);  /* CHECKED */
    x->x_maxstep = 2;
    x->x_minstep = 0;
    if ((int)f2)  /* CHECKED */
	drunk_ft2(x, f2);
    x->x_value = x->x_maxvalue / 2;  /* CHECKED */
    rand_seed(&x->x_seed, 0);  /* CHECKED third arg silently ignored */
    x->x_bitseed = 123456789;  /* FIXME */
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft2"));
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void drunk_setup(void)
{
    drunk_class = class_new(gensym("drunk"),
			    (t_newmethod)drunk_new, 0,
			    sizeof(t_drunk), 0,
			    A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(drunk_class, drunk_bang);
    class_addfloat(drunk_class, drunk_float);
    class_addmethod(drunk_class, (t_method)drunk_ft1,
		    gensym("ft1"), A_FLOAT, 0);
    class_addmethod(drunk_class, (t_method)drunk_ft2,
		    gensym("ft2"), A_FLOAT, 0);
    /* CHECKED list is auto-unfolded */
    class_addmethod(drunk_class, (t_method)drunk_seed,
		    gensym("seed"), A_FLOAT, 0);  /* CHECKED arg obligatory */
    class_addmethod(drunk_class, (t_method)drunk_set,
		    gensym("set"), A_FLOAT, 0);  /* CHECKED arg obligatory */
    class_addmethod(drunk_class, (t_method)drunk_state,
		    gensym("state"), 0);  /* CHECKED arg obligatory */
}
