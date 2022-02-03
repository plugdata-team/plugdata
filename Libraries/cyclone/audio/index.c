//Derek Kwan 2016 - de-arsiced,.. cybuffed

/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER: 'click' method */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"

#define INDEX_MAXCHANNELS  64

typedef struct _index
{
    t_object    x_obj;
    t_cybuf   *x_cybuf;
    int         x_channum; //requested channel number (1-indexed)
    t_inlet  *x_phaselet;
    t_outlet *x_outlet;
} t_index;

static t_class *index_class;

static void index_set(t_index *x, t_symbol *s){
    cybuf_setarray(x->x_cybuf, s);
}

static t_int *index_perform(t_int *w)
{
    t_index *x = (t_index *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *out = (t_float *)(w[4]);

    t_cybuf *c = x->x_cybuf;
    if (c->c_playable)
    {	
	t_float *xin = (t_float *)(w[3]);
	int index, maxindex = c->c_npts - 1;
	t_word *vp = c->c_vectors[0];
	if (vp)  /* handle array swapping on the fly via ft1 */
	{
	    while (nblock--)
	    {
		index = (int)(*xin++ + 0.5);
		if (index < 0)
		    index = 0;
		else if (index > maxindex)
		    index = maxindex;
		*out++ = vp[index].w_float;
	    }
	}
	else while (nblock--) *out++ = 0;
    }
    else while (nblock--) *out++ = 0;
    return (w + 5);
}

static void index_float(t_index *x, t_float f)
{
    pd_error(x, "index~: no method for 'float'");
}

static void index_ft1(t_index *x, t_floatarg f)
{
    int ch = (f < 1) ? 1 : (f > CYBUF_MAXCHANS) ? CYBUF_MAXCHANS : (int) f;
    x->x_channum = ch;
    cybuf_getchannel(x->x_cybuf, ch, 1);
}

static void index_dsp(t_index *x, t_signal **sp)
{
    cybuf_checkdsp(x->x_cybuf); 
    dsp_add(index_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void index_free(t_index *x)
{
    inlet_free(x->x_phaselet);
    outlet_free(x->x_outlet);
    cybuf_free(x->x_cybuf);
}

static void *index_new(t_symbol *s, t_floatarg f)
{
    //like peek~, changing so it doesn't default to 0 but 1 for the new cybuf
    //single channel mode, not sure how much of a diff it makes... - DK
    int ch = (f < 1) ? 1 : (f > CYBUF_MAXCHANS) ? CYBUF_MAXCHANS : (int) f;
    /* two signals:  index input, value output */
    t_index *x = (t_index *)pd_new(index_class);
    x->x_cybuf = cybuf_init((t_class *)x, s, 1, ch);
    if (x->x_cybuf){
	x->x_channum = ch;
        x->x_phaselet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
	x->x_outlet = outlet_new(&x->x_obj, gensym("signal"));
    };
    return (x);
}

CYCLONE_OBJ_API void index_tilde_setup(void)
{
    index_class = class_new(gensym("index~"),
			    (t_newmethod)index_new,
			    (t_method)index_free,
			    sizeof(t_index), 0,
			    A_DEFSYM, A_DEFFLOAT, 0);
    class_addmethod(index_class, (t_method)index_dsp, gensym("dsp"), A_CANT, 0);
    class_domainsignalin(index_class, -1);
     class_addfloat(index_class, (t_method)index_float);
    class_addmethod(index_class, (t_method)index_set,
		    gensym("set"), A_SYMBOL, 0);
    class_addmethod(index_class, (t_method)index_ft1,
		    gensym("ft1"), A_FLOAT, 0);
}
