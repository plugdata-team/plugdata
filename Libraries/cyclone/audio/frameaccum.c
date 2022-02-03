/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define FRAMEACCUM_INISIZE  512
#define TWO_PI              (M_PI * 2.)

typedef struct _frameaccum{
    t_object  x_obj;
    t_inlet  *frameaccum;
    int       x_size;
    int       x_wrapFlag;
    t_float  *x_frame;
    t_float   x_frameini[FRAMEACCUM_INISIZE];
}t_frameaccum;

static t_class *frameaccum_class;

static t_int *frameaccum_perform(t_int *w){
    int nblock = (int)(w[1]);
    t_frameaccum *x = (t_frameaccum *)(w[2]);
    t_float *frame = x->x_frame;
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    if(x->x_wrapFlag){
        while (nblock--){
            *frame += *in++;
             double dnorm = *frame + M_PI;
             if (dnorm < 0)
                 *frame = M_PI - fmod(-dnorm, TWO_PI);
             else
                 *frame = fmod(dnorm, TWO_PI) - M_PI;
            *out++ = *frame++;
        }
    }
    else{
        while(nblock--)
            *out++ = (*frame++ += *in++);
    }
    return(w+5);
}

static void frameaccum_dsp(t_frameaccum *x, t_signal **sp){
    int nblock = sp[0]->s_n;
    if(nblock > x->x_size){
        x->x_frame = grow_nodata(&nblock, &x->x_size, x->x_frame,
                FRAMEACCUM_INISIZE, x->x_frameini, sizeof(*x->x_frame));
    }
    memset(x->x_frame, 0, nblock * sizeof(*x->x_frame));  /* CHECKED */
    dsp_add(frameaccum_perform, 4, nblock, x, sp[0]->s_vec, sp[1]->s_vec);
}

static void frameaccum_free(t_frameaccum *x)
{
    if (x->x_frame != x->x_frameini)
	freebytes(x->x_frame, x->x_size * sizeof(*x->x_frame));
}

static void *frameaccum_new(t_floatarg f)
{
    t_frameaccum *x = (t_frameaccum *)pd_new(frameaccum_class);
    x->x_wrapFlag = f != 0;
    int size;
    x->x_size = FRAMEACCUM_INISIZE;
    x->x_frame = x->x_frameini;
    if ((size = sys_getblksize()) > FRAMEACCUM_INISIZE)
	x->x_frame = grow_nodata(&size, &x->x_size, x->x_frame,
				 FRAMEACCUM_INISIZE, x->x_frameini, sizeof(*x->x_frame));
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void frameaccum_tilde_setup(void)
{
    frameaccum_class = class_new(gensym("frameaccum~"),
				 (t_newmethod)frameaccum_new,
				 (t_method)frameaccum_free,
				 sizeof(t_frameaccum), 0, A_DEFFLOAT, 0);
    class_addmethod(frameaccum_class, nullfn, gensym("signal"), 0);
    class_addmethod(frameaccum_class, (t_method) frameaccum_dsp, gensym("dsp"), A_CANT, 0);
}
