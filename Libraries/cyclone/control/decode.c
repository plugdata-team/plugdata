/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is an entirely rewritten version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

#include "m_pd.h"
#include <common/api.h>

#define decode_C74MAXOUTS  512  // CHECKED (max goes up higher but freezes) */
#define decode_DEFOUTS     1

typedef struct _decode
{
    t_object    x_ob;
    int         x_numouts;
    int         x_onout;
    int         x_allon;   /* submaster switch */
    int         x_alloff;  /* master switch */
    t_outlet  **x_outs;
    t_outlet   *x_outbuf[decode_C74MAXOUTS];
} t_decode;

static t_class *decode_class;

/* CHECKED: all outlets deliver after any action */
/* CHECKED: outlets output in right-to-left order */


static void decode_deliver(t_decode *x)
{
    int i = x->x_numouts;
    if (x->x_alloff)
	while (i--) outlet_float(x->x_outs[i], 0);
    else if (x->x_allon)
	while (i--) outlet_float(x->x_outs[i], 1);
    else
	while (i--) outlet_float(x->x_outs[i], (i == x->x_onout ? 1 : 0));
}

static void decode_float(t_decode *x, t_floatarg f)
{
    int val = (int)f;
    /* CHECKED: out-of-range input is clipped, not ignored */
    if (val < 0)
	val = 0;
    else if (val >= x->x_numouts)
	val = x->x_numouts - 1;
    /* CHECKED: while in all-off mode, input is stored, not ignored */
    x->x_onout = val;
    decode_deliver(x);
}


static void decode_bang(t_decode *x)
{
    decode_deliver(x);
}

static void decode_allon(t_decode *x, t_floatarg f)
{
    x->x_allon = (f != 0);
    decode_deliver(x);
}

static void decode_alloff(t_decode *x, t_floatarg f)
{
    x->x_alloff = (f != 0);
    decode_deliver(x);
}

static void decode_free(t_decode *x)
{
    if (x->x_outs != x->x_outbuf)
	freebytes(x->x_outs, x->x_numouts * sizeof(*x->x_outs));
}

static void *decode_new(t_floatarg val){
    t_decode *x;
    int i, nouts = (int)val;
    t_outlet **outs;
    if (nouts < 1)
	nouts = decode_DEFOUTS;
    if (nouts > decode_C74MAXOUTS){
        nouts = decode_C74MAXOUTS;
        if (!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
            return (0);
    }
    else outs = 0;
    x = (t_decode *)pd_new(decode_class);
    x->x_numouts = nouts;
    x->x_outs = (outs ? outs : x->x_outbuf);
    x->x_onout = 0;
    x->x_allon = 0;
    x->x_alloff = 0;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft2"));
    for (i = 0; i < nouts; i++)
	x->x_outs[i] = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void decode_setup(void)
{
    decode_class = class_new(gensym("decode"), (t_newmethod)decode_new,
        (t_method)decode_free, sizeof(t_decode), 0, A_DEFFLOAT, 0);
    class_addfloat(decode_class, decode_float);
    class_addbang(decode_class, decode_bang);
    class_addmethod(decode_class, (t_method)decode_allon, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(decode_class, (t_method)decode_alloff, gensym("ft2"), A_FLOAT, 0);
}

CYCLONE_OBJ_API void Decode_setup(void)
{
    decode_class = class_new(gensym("Decode"), (t_newmethod)decode_new,
        (t_method)decode_free, sizeof(t_decode), 0, A_DEFFLOAT, 0);
    class_addfloat(decode_class, decode_float);
    class_addbang(decode_class, decode_bang);
    class_addmethod(decode_class, (t_method)decode_allon, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(decode_class, (t_method)decode_alloff, gensym("ft2"), A_FLOAT, 0);
    class_sethelpsymbol(decode_class, gensym("decode"));
    pd_error(decode_class, "Cyclone: please use [decode] instead of [Decode] to suppress this error");
}
