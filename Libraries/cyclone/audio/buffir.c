/* Copyright (c) 2003-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"

#define BUFFIR_DEFSIZE    0
#define BUFFIR_MAXSIZE  4096

typedef struct _buffir
{
    t_object    x_obj;
    t_cybuf   *x_cybuf;
    t_inlet    *x_offlet;
    t_inlet    *x_sizlet;
    t_float *x_lohead;
    t_float *x_hihead;
    t_float *x_histlo;
    t_float *x_histhi;
    t_float  x_histbuf[2 * BUFFIR_MAXSIZE];
    int      x_checked;
} t_buffir;

static t_class *buffir_class;

static void buffir_setrange(t_buffir *x, t_floatarg f1, t_floatarg f2)
{
    int off = (int)f1;
    int siz = (int)f2;
    if (off < 0)
	off = 0;
    if (siz <= 0)
	siz = BUFFIR_DEFSIZE;
	if (siz > BUFFIR_MAXSIZE)
	siz = BUFFIR_MAXSIZE;
	pd_float((t_pd *)x->x_offlet, off);
	pd_float((t_pd *)x->x_sizlet, siz);
}

static void buffir_clear(t_buffir *x)
{
    memset(x->x_histlo, 0, 2 * BUFFIR_MAXSIZE * sizeof(*x->x_histlo));
    x->x_lohead = x->x_histlo;
    x->x_hihead = x->x_histhi = x->x_histlo + BUFFIR_MAXSIZE;
}

static void buffir_set(t_buffir *x, t_symbol *s, t_floatarg f1, t_floatarg f2)
{
    cybuf_setarray(x->x_cybuf, s);
    buffir_setrange(x, f1, f2);
}


static void buffir_float(t_buffir *x, t_float f)
{
    pd_error(x, "buffir~: no method for 'float'");
}

static t_int *buffir_perform(t_int *w)
{
    t_buffir *x = (t_buffir *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[6]);
    t_float *lohead = x->x_lohead;
    t_float *hihead = x->x_hihead;
    t_cybuf *c = x->x_cybuf;
    if (c->c_playable)
    {	

	t_float *oin = (t_float *)(w[4]);
	t_float *sin = (t_float *)(w[5]);
	int bufnpts = c->c_npts;
	t_word *vec = c->c_vectors[0];  /* playable implies nonzero (mono) */
	while (nblock--)
	{

	    /* CHECKME every sample or once per block.
	       If once per block, then LATER think about performance. */
	    /* CHECKME rounding */
	    int off = (int)*oin++;
	    int npoints = (int)*sin++;
	    if (off < 0)
		off = 0;
	    if (npoints > BUFFIR_MAXSIZE)
		npoints = BUFFIR_MAXSIZE;
	    if (npoints > bufnpts - off)
		npoints = bufnpts - off;
	    if (npoints > 0)
	    {
	    if (!(x->x_checked))
	    {
	    x->x_checked = 1;
	    }
		t_word *coefp = vec;
		t_float *hp = hihead;
		t_float sum = 0.;
		*lohead++ = *hihead++ = *xin++;
		while (npoints--)
			sum += coefp[off++].w_float * *hp--;
			//sum += coefp->w_float * *hp--;
		*out++ = sum;
	    }
	    else
	    {
		*lohead++ = *hihead++ = *xin++;
		*out++ = 0.;
	    }
	    if (lohead >= x->x_histhi)
	    {
		lohead = x->x_histlo;
		hihead = x->x_histhi;
	    }
	}
    }
    else while (nblock--)
    {
	*lohead++ = *hihead++ = *xin++;
	*out++ = 0.;
	if (lohead >= x->x_histhi)
	{
	    lohead = x->x_histlo;
	    hihead = x->x_histhi;
	}
    }
    x->x_lohead = lohead;
    x->x_hihead = hihead;
    return (w + 7);
}

static void buffir_dsp(t_buffir *x, t_signal **sp)
{
	x->x_checked = 0;
    cybuf_checkdsp(x->x_cybuf); 
    dsp_add(buffir_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void buffir_free(t_buffir *x)
{
    inlet_free(x->x_offlet);
    inlet_free(x->x_sizlet);
    cybuf_free(x->x_cybuf);
}

static void *buffir_new(t_symbol *s, t_floatarg f1, t_floatarg f2)
{
    /* CHECKME always the first channel used. */
    /* three auxiliary signals: main, offset and size inputs */
    t_buffir *x = (t_buffir *)pd_new(buffir_class);
    x->x_cybuf = cybuf_init((t_class *)x, s, 1, 0);
    if (x->x_cybuf)
    {
	
	
        x->x_offlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_offlet, f1);
	x->x_sizlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_sizlet, f2);
	
	outlet_new(&x->x_obj, gensym("signal"));
	x->x_histlo = x->x_histbuf;
	x->x_histhi = x->x_histbuf+BUFFIR_MAXSIZE;
	x->x_checked = 0;
	buffir_clear(x);
	buffir_setrange(x, f1, f2);
    }
    return (x);
}

CYCLONE_OBJ_API void buffir_tilde_setup(void)
{
    buffir_class = class_new(gensym("buffir~"), (t_newmethod)buffir_new, (t_method)buffir_free,
    sizeof(t_buffir), CLASS_DEFAULT, A_DEFSYM, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_domainsignalin(buffir_class, -1);
    class_addfloat(buffir_class, buffir_float);
    class_addmethod(buffir_class, (t_method)buffir_dsp, gensym("dsp"), A_CANT, 0);
    //class_addmethod(c, (t_method)arsic_enable, gensym("enable"), 0);
    class_addmethod(buffir_class, (t_method)buffir_clear, gensym("clear"), 0);
    class_addmethod(buffir_class, (t_method)buffir_set, gensym("set"), A_SYMBOL,
                    A_DEFFLOAT, A_DEFFLOAT, 0);
}
