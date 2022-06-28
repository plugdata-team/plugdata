/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is an entirely rewritten version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

// porres 2016/2017, de-loud - no capital letter as default

#include "m_pd.h"
#include <common/api.h>

#define histo_DEFSIZE  128

typedef struct _histo
{
    t_object   x_ob;
    int        x_size;
    unsigned  *x_hist;  /* LATER consider using 64 bits */
    int        x_lastinput;
    t_outlet  *x_countout;
} t_histo;

static t_class *histo_class;

static void histo_clear(t_histo *x)
{
    int i = x->x_size;
    while (i--) x->x_hist[i] = 0;
    /* CHECKED: last input is kept */
}

static void histo_doit(t_histo *x, int val, int doincr)
{
    if (val >= 0 && val < x->x_size)
    {
	if (doincr)
	{
	    /* CHECKED: only in-range numbers are stored */
	    x->x_lastinput = val;
	    x->x_hist[val]++;
	}
	outlet_float(x->x_countout, x->x_hist[val]);
	/* CHECKED: out-of-range numbers are never passed thru */
	outlet_float(((t_object *)x)->ob_outlet, val);
    }
}

static void histo_bang(t_histo *x)
{
    histo_doit(x, x->x_lastinput, 0);
}

static void histo_float(t_histo *x, t_floatarg f)
{
    int i = (int)f;
    if (i != f) pd_error(x, "histo: doesn't understand 'noninteger float'");
    histo_doit(x, i, 1);
}


static void histo_ft1(t_histo *x, t_floatarg f)
{
    /* CHECKED: floats are accepted in second inlet (truncated) */
    histo_doit(x, (int)f, 0);
}

static void histo_free(t_histo *x)
{
    if (x->x_hist)
	freebytes(x->x_hist, x->x_size * sizeof(*x->x_hist));
}

static void *histo_new(t_floatarg f)
{
    t_histo *x;
    int size = (int)f;
    unsigned *hist;
    if (size < 1)  /* CHECKED: 1 is allowed */
	size = histo_DEFSIZE;
    if (!(hist = (unsigned *)getbytes(size * sizeof(*hist))))
	return (0);
    x = (t_histo *)pd_new(histo_class);
    x->x_size = size;
    x->x_hist = hist;
    x->x_lastinput = 0;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_countout = outlet_new((t_object *)x, &s_float);
    histo_clear(x);
    return (x);
}

CYCLONE_OBJ_API void histo_setup(void){
    histo_class = class_new(gensym("histo"), (t_newmethod)histo_new,
        (t_method)histo_free, sizeof(t_histo), 0, A_DEFFLOAT, 0);
    class_addbang(histo_class, histo_bang);
    class_addfloat(histo_class, histo_float);
    class_addmethod(histo_class, (t_method)histo_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(histo_class, (t_method)histo_clear, gensym("clear"), 0);
}

CYCLONE_OBJ_API void Histo_setup(void){
    histo_class = class_new(gensym("Histo"), (t_newmethod)histo_new,
        (t_method)histo_free, sizeof(t_histo), 0, A_DEFFLOAT, 0);
    class_addbang(histo_class, histo_bang);
    class_addfloat(histo_class, histo_float);
    class_addmethod(histo_class, (t_method)histo_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(histo_class, (t_method)histo_clear, gensym("clear"), 0);
    pd_error(histo_class, "Cyclone: please use [histo] instead of [Histo] to suppress this error");
    class_sethelpsymbol(histo_class, gensym("histo"));
}
