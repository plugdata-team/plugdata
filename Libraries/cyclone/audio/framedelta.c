/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define FRAMEDELTA_INISIZE  512

typedef struct _framedelta
{
    t_object  x_obj;
    t_inlet  *framedelta;
    int       x_size;
    t_float  *x_frame;
    t_float   x_frameini[FRAMEDELTA_INISIZE];
} t_framedelta;

static t_class *framedelta_class;

static t_int *framedelta_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *frame = (t_float *)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
	float f = *in++;
	*out++ = f - *frame;  /* CHECKME */
	*frame++ = f;
    }
    return (w + 5);
}

static void framedelta_dsp(t_framedelta *x, t_signal **sp)
{
    int nblock = sp[0]->s_n;
    if (nblock > x->x_size)
	x->x_frame = grow_nodata(&nblock, &x->x_size, x->x_frame,
				 FRAMEDELTA_INISIZE, x->x_frameini,
				 sizeof(*x->x_frame));
    memset(x->x_frame, 0, nblock * sizeof(*x->x_frame));  /* CHECKME */
    dsp_add(framedelta_perform, 4, nblock, x->x_frame,
	    sp[0]->s_vec, sp[1]->s_vec);
}

static void framedelta_free(t_framedelta *x)
{
    if (x->x_frame != x->x_frameini)
	freebytes(x->x_frame, x->x_size * sizeof(*x->x_frame));
}

static void *framedelta_new(t_symbol *s, int ac, t_atom *av)
{
    t_framedelta *x = (t_framedelta *)pd_new(framedelta_class);
    int size;
    x->x_size = FRAMEDELTA_INISIZE;
    x->x_frame = x->x_frameini;
    if ((size = sys_getblksize()) > FRAMEDELTA_INISIZE)
	x->x_frame = grow_nodata(&size, &x->x_size, x->x_frame,
				 FRAMEDELTA_INISIZE, x->x_frameini,
				 sizeof(*x->x_frame));
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void framedelta_tilde_setup(void)
{
    framedelta_class = class_new(gensym("framedelta~"),
				 (t_newmethod)framedelta_new,
				 (t_method)framedelta_free,
				 sizeof(t_framedelta), 0, 0);
    class_addmethod(framedelta_class, nullfn, gensym("signal"), 0);
    class_addmethod(framedelta_class, (t_method) framedelta_dsp, gensym("dsp"), A_CANT, 0);
}
