/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"
#include <string.h>

#define DELAY_DEFMAXSIZE  512 // default buffer size
#define DELAY_GUARD 4 // guard points for 4-point interpolation
#define DELAY_EXTRA 1 // one sample extra delay for 4-point interpolation


typedef struct _delay
{
    t_object  x_obj;
    t_glist	 *x_glist;
    t_inlet  *x_inlet;
    t_float  *x_buf;
    t_float  *x_bufend;
    t_float  *x_whead;
    int       x_sr; //sample rate
    int	      x_maxsize; // buffer size in samples
    int		  x_maxsofar; // largest maxsize so far
    int		  x_delsize; // current delay in samples
    int		  x_prevdelsize; // previous delay in samples
    t_float   x_ramplength; // length of ramp in ms
    double    x_rampinc; // samplewise ramp increment
    double    x_rampvals[2]; // current ramp scalar values [0] current [1] previous
	int 	  x_remain; // number of samples remaining in ramp
	int 	  x_hasfeeders; // true if right inlet has signal input
	t_float	  x_bufini[DELAY_DEFMAXSIZE + 2*DELAY_GUARD - 1]; // default stack buffer
} t_delay;

static t_class *delay_class;



static void delay_clear(t_delay *x)
{
// 	int i;
//     for (i = 0; i < x->x_maxsize; i++) {
//         x->x_buf[i] = 0;
    memset(x->x_buf, 0, (x->x_maxsize + 2*DELAY_GUARD - 1) * sizeof(*x->x_buf));
    
}

static void delay_bounds(t_delay *x)
{
	if (x->x_hasfeeders)
	{
		x->x_whead = x->x_buf + DELAY_GUARD - 1;
    	x->x_bufend = x->x_buf + x->x_maxsize + 2*DELAY_GUARD - 1;
	}
	else
	{
		x->x_whead = x->x_buf;
    	x->x_bufend = x->x_buf + x->x_maxsize;
	}
}

static void delay_maxsize(t_delay *x, t_float f)
{
//	   int maxsize = x->x_maxsize = (f > 1 ? (int)f : 1);
//     t_float *buf = (t_float *)getbytes(maxsize * sizeof(*buf));
//     if (x->x_delsize > x->x_maxsize)
// 	x->x_delsize = x->x_maxsize;
//     x->x_buf = x->x_whead = buf;
//     x->x_bufend = buf + maxsize - 1;
int maxsize = (f < 1 ? 1 : (int)f);
if (maxsize > x->x_maxsofar)
{
	x->x_maxsofar = maxsize;
	if (x->x_buf == x->x_bufini)
	{
		if (!(x->x_buf = (t_float *)getbytes((maxsize + 2*DELAY_GUARD - 1) * sizeof(*x->x_buf))))
		{
			x->x_buf = x->x_bufini;
			x->x_maxsize = DELAY_DEFMAXSIZE;
			pd_error(x, "unable to resize buffer; using size %d", DELAY_DEFMAXSIZE);
		}
	}
	else if (x->x_buf)
	{
		if (!(x->x_buf = (t_float *)resizebytes(x->x_buf, 
			(x->x_maxsize + 2*DELAY_GUARD - 1) * sizeof(*x->x_buf),
			(maxsize + 2*DELAY_GUARD - 1) * sizeof(*x->x_buf))))
		{
			x->x_buf = x->x_bufini;
			x->x_maxsize = DELAY_DEFMAXSIZE;
			pd_error(x, "unable to resize buffer; using size %d", DELAY_DEFMAXSIZE);
		}		
	}
}
x->x_maxsize = maxsize;
if (x->x_delsize > maxsize)
	x->x_delsize = maxsize;
x->x_remain = 0;
delay_clear(x);
delay_bounds(x);
} 


static void delay_delay(t_delay *x, t_floatarg f)
{
	x->x_prevdelsize = x->x_delsize;
    x->x_delsize = (f > 0 ? (int)f : 0);
    if (x->x_delsize > x->x_maxsize)
		x->x_delsize = x->x_maxsize;
	x->x_remain = (int)(x->x_ramplength * x->x_sr * 0.001);
	x->x_rampinc = (double)(1.0)/(double)(x->x_remain);
	x->x_rampvals[0] = 0.0; /*current*/
	x->x_rampvals[1] = 1.0;	/*previous*/
}

static void delay_ramp(t_delay *x, t_floatarg f)
{
	x->x_ramplength = (f < 0. ? 0. : f);
}

static t_int *delay_perform(t_int *w)
{
    t_delay *x = (t_delay *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float *buf = x->x_buf;
    t_float *ep = x->x_bufend;
    t_float *wp = x->x_whead;
    int maxsize = x->x_maxsize;
    
    if (*in2 != x->x_delsize && !x->x_remain)
    {
    	delay_delay(x, *in2);
    }
    
    int nleft = x->x_remain;
    if (nleft)
    {
    	double incr = x->x_rampinc;
    	double currramp = x->x_rampvals[0];
    	double prevramp = x->x_rampvals[1];
    	t_float *prevrp = wp - x->x_prevdelsize;
    	t_float *currrp = wp - x->x_delsize;
    	if (prevrp < buf) prevrp += maxsize + 1;
    	if (currrp < buf) currrp += maxsize + 1;
    	
    	if (nleft > nblock)
    	{
    		nleft -= nblock;
    		while (nblock--)
    		{
    			t_sample f = *in1++;
    			if (PD_BIGORSMALL(f))
    				f = 0.;
    			currramp += incr;
    			prevramp -= incr;*wp = f;
    			*out++ = (t_float)(*prevrp * prevramp + *currrp * currramp);
    			if (prevrp++ == ep) prevrp = buf;
    			if (currrp++ == ep) currrp = buf;
    			if (wp++ == ep) wp = buf;
    		}
    	}
    	else
    	{
    		while (nleft)
    		{
    			nleft--;
    			nblock--;
    			t_sample f = *in1++;
    			if (PD_BIGORSMALL(f))
    				f = 0.;
    			currramp += incr;
    			prevramp -= incr;
    			*wp = f;
    			*out++ = (t_float)(*prevrp * prevramp + *currrp * currramp);
    			if (prevrp++ == ep) prevrp = buf;
				if (currrp++ == ep) currrp = buf;
    			if (wp++ == ep) wp = buf;
    		}
    		while (nblock--)
    		{
    			t_sample f = *in1++;
    			if (PD_BIGORSMALL(f))
    				f = 0.;
    			*wp = f;
    			*out++ = *currrp;
    			if (currrp++ == ep) currrp = buf;
    			if (wp++ == ep) wp = buf;
    		}
    	} /* end else */
    	x->x_rampvals[0] = currramp;
    	x->x_rampvals[1] = prevramp;
    	x->x_remain = nleft;
    }
    else if (x->x_delsize)
    {
    	t_float *rp = wp - x->x_delsize;
    	if (rp < buf) rp += maxsize + 1;
    	while (nblock--)
    	{
    		t_sample f = *in1++;
    		if (PD_BIGORSMALL(f))
    			f = 0.;
    		*out++ = *rp;
    		*wp = f;
    		if (rp++ == ep) rp = buf;
    		if (wp++ == ep) wp = buf;
    	}
    }
    else while (nblock--)
    {
    	t_sample f = *in1++;
    	if (PD_BIGORSMALL(f))
    		f = 0.;
    	*out++ = *wp = f;
    	if (wp++ == ep) wp = buf;
    }
    x->x_whead = wp;
    return (w + 6);
}

static t_int *delay_performsig(t_int *w)
{
	t_delay *x = (t_delay *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float *buf = x->x_buf;
    t_float *bp = x->x_buf + DELAY_GUARD - 1;
    t_float *ep = x->x_bufend;
    t_float *wp = x->x_whead;
    int maxsize = x->x_maxsize;
    t_float *rp;
    
    while (nblock--)
    {
    	t_sample f, del, frac, a, b, c, d, cminusb;
   		int idel;
    	f = *in1++;
    	if (PD_BIGORSMALL(f))
    		f = 0.;
    	*wp = f;
    	del = *in2++;
    	del = (del > 0. ? del : 0.);
    	del = (del < maxsize ? del : maxsize);
    	idel = (int)del;
    	frac = del - (t_sample)idel;
    	rp = wp - idel;
    	if (rp < bp)
    		rp += (maxsize + DELAY_GUARD);
    	d = rp[-3];
        c = rp[-2];
        b = rp[-1];
        a = rp[0];
        cminusb = c-b;
        *out++ = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
    	if (++wp == ep)
    	{
    		buf[0] = wp[-3];
    		buf[1] = wp[-2];
    		buf[2] = wp[-1];
    		wp = bp;
    	}
    }
    x->x_whead = wp;
    return (w + 6);
}

static void delay_dsp(t_delay *x, t_signal **sp)
{
    //memset(x->x_buf, 0, x->x_maxsize * sizeof(*x->x_buf));  /* CHECKED */
    x->x_hasfeeders = magic_inlet_connection(&x->x_obj, x->x_glist, 1, &s_signal);
    x->x_sr = sp[0]->s_sr;
    x->x_remain = 0;
    delay_clear(x);
    delay_bounds(x);
    if (x->x_hasfeeders)
    {
    	dsp_add(delay_performsig, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
    else
    {
    	dsp_add(delay_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    }
}

static void *delay_new(t_floatarg f1, t_floatarg f2)
{
    t_delay *x;
    //t_float *buf = (t_float *)getbytes(maxsize * sizeof(*buf));
    //if (!buf)
	//return (0);
    x = (t_delay *)pd_new(delay_class);
    int maxsize = (f1 > 0 ? (int)f1 : DELAY_DEFMAXSIZE);
    x->x_maxsize = x->x_maxsofar = DELAY_DEFMAXSIZE;
    x->x_buf = x->x_whead = x->x_bufini;
    delay_maxsize(x, maxsize);
    x->x_delsize = (f2 > 0 ? (int)f2 : 0);
    if (x->x_delsize > maxsize)
		x->x_delsize = maxsize;
	x->x_glist = canvas_getcurrent();
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), x->x_delsize);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

static void delay_free(t_delay *x)
{
    if (x->x_buf != x->x_bufini) freebytes(x->x_buf, x->x_maxsofar * sizeof(*x->x_buf));
}

CYCLONE_OBJ_API void delay_tilde_setup(void)
{
    delay_class = class_new(gensym("delay~"),
			    (t_newmethod)delay_new, (t_method)delay_free,
			    sizeof(t_delay), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(delay_class, nullfn, gensym("signal"), 0);
    class_addmethod(delay_class, (t_method)delay_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(delay_class, (t_method)delay_clear,
		    gensym("clear"), 0);
	class_addmethod(delay_class, (t_method)delay_ramp,
			gensym("ramp"), A_FLOAT, 0);
    class_addmethod(delay_class, (t_method)delay_maxsize, gensym("maxsize"), A_DEFFLOAT, 0);
}
