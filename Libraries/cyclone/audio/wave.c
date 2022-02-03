/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"
#include "common/shared.h"

#define CYWAVEMAXOUT 64 //max number of outs
#define CYWAVEINTERP 1 //default interp mode

// CHECKME (the refman): the extra channels are not played

//adding wave_getarraysmp, adding attributes, dearsiced/cybufed - Derek Kwan 2016


typedef struct _wave
{
    t_object    x_obj;
    t_cybuf   *x_cybuf;
    int               x_bufsize;
    int               x_interp_mode;
    int               x_numouts; //number of outputs
    float             x_ksr; //sample rate in ms
    t_float           x_f; //dummy
    t_float	          x_bias;
    t_float           x_tension;
    t_inlet 		 *x_startlet;
    t_inlet 		 *x_endlet;
    t_float         *x_in; //main inlet signal vector
    t_float         *x_st; //start inlet vector
    t_float         *x_e; //end inlet vector
    t_float         **x_ovecs; // output vectors
} t_wave;

static t_class *wave_class;


static void wave_float(t_wave *x, t_float f)
{
    pd_error(x, "wave~: no method for 'float'");
}

/* interpolation functions w/ jump table -- code was cleaner than a massive
switch - case block in the wave_perform() routine; might be very slightly
less efficient this way but I think the clarity was worth it.

The Max/MSP wave~ class seems to copy all of its interpolation functions
directly from http://paulbourke.net/miscellaneous/interpolation/ without
attribution. Note that "cubic" is not the same interpolator as in tabread4~. 
For convenience, I've added that interpolator as wave_lagrange().
  -- Matt Barber */

static void wave_interp_bias(t_wave *x, t_floatarg f)
{
	x->x_bias = f;
	return;
}

static void wave_interp_tension(t_wave *x, t_floatarg f)
{
	x->x_tension = f;
	return;
}


static void wave_interp(t_wave *x, t_floatarg f)
{
	
	x->x_interp_mode = f < 0 ? 0 : f;
	x->x_interp_mode = x->x_interp_mode > 7 ? 7 : x->x_interp_mode;
    cybuf_setminsize(x->x_cybuf,(x->x_interp_mode >= 4 ? 4 : 1));
    cybuf_playcheck(x->x_cybuf);
}

static void wave_set(t_wave *x, t_symbol *s)
{
    cybuf_setarray(x->x_cybuf, s);

}
  
/*stupid hacks; sorry. This saves a lot of typing.*/
#define BOUNDS_CHECK() \
	if (spos < 0) spos = 0; \
	else if (spos > maxindex) spos = maxindex; \
	if (epos > maxindex || epos <= 0) epos = maxindex; \
	else if (epos < spos) epos = spos; \
	int siz = (int)(epos - spos + 1.5); \
	int ndx; \
	int ch = nch; \
	if (phase < 0) phase = 0; \
	else if (phase > 1.0) phase = 0; \
	int sposi = (int)spos; \
	int eposi = sposi + siz
	
	
#define INDEX_2PT(TYPE) \
	int ndx1; \
	TYPE a, b; \
	TYPE xpos = phase*siz + spos; \
	ndx = (int)xpos; \
	TYPE frac = xpos - ndx; \
	if (ndx == eposi) ndx = sposi; \
	ndx1 = ndx + 1; \
	if (ndx1 == eposi) ndx1 = sposi
	
#define INDEX_4PT() \
	int ndxm1, ndx1, ndx2; \
	double a, b, c, d; \
	double xpos = phase*siz + spos; \
	ndx = (int)xpos; \
	double frac = xpos - ndx; \
	if (ndx == eposi) ndx = sposi; \
	ndxm1 = ndx - 1; \
	if (ndxm1 < sposi) ndxm1 = eposi - 1; \
	ndx1 = ndx + 1; \
	if (ndx1 == eposi) ndx1 = sposi; \
	ndx2 = ndx1 + 1; \
	if (ndx2 == eposi) ndx2 = sposi;


static void wave_nointerp(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		ndx = (int)(phase*siz + spos);
		ndx = (ndx >= eposi ? sposi : ndx);
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			out[iblock] = (vp ? vp[ndx].w_float : 0);
		}	
	}
	return;
}

static void wave_linear(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		double phase = (double)(*xin++);
		double spos = (double)(*sin++) * ksr;
		double epos = (double)(*ein++) * ksr;
		BOUNDS_CHECK();
		INDEX_2PT(double);
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndx].w_float;
				b = (double)vp[ndx1].w_float;
				out[iblock] = (t_float)(a * (1.0 - frac) + b * frac);
			}
			else out[iblock] = 0;
		}
	}
	return;
}

static void wave_linlq(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_2PT(float);
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = vp[ndx].w_float;
				b = vp[ndx1].w_float;
				out[iblock] = a + frac * (b - a);
			}
			else out[iblock] = 0;
		}
	}
	return;
}

static void wave_cosine(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_2PT(double);
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndx].w_float;
				b = (double)vp[ndx1].w_float;
				frac = (1 - cos(frac * M_PI)) / 2.0;
				out[iblock] = (t_float)(a * (1 - frac) + b * (frac));
			}
			else out[iblock] = 0;
		}
		
	}
	return;
}

static void wave_cubic(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_4PT();
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndxm1].w_float;
				b = (double)vp[ndx].w_float;
				c = (double)vp[ndx1].w_float;
				d = (double)vp[ndx2].w_float;
				double p0, p1, p2;
				p0 = d - a + b - c;
				p1 = a - b - p0;
				p2 = c - a;
				out[iblock] = (t_float)(b+frac*(p2+frac*(p1+frac*p0)));	
			}
			else out[iblock] = 0;
		}
		
	}
	return;
}

static void wave_spline(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_4PT();
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndxm1].w_float;
				b = (double)vp[ndx].w_float;
				c = (double)vp[ndx1].w_float;
				d = (double)vp[ndx2].w_float;
				double p0, p1, p2;
				p0 = 0.5*(d - a) + 1.5*(b - c);
				p2 = 0.5*(c - a);
				p1 = a - b + p2 - p0;
				out[iblock] = (t_float)(b+frac*(p2+frac*(p1+frac*p0)));	
			}
			else out[iblock] = 0;
		}
		
	}
	return;
}

static void wave_hermite(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_4PT();
		double tension = (double)x->x_tension;
		double bias = (double)x->x_bias;
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndxm1].w_float;
				b = (double)vp[ndx].w_float;
				c = (double)vp[ndx1].w_float;
				d = (double)vp[ndx2].w_float;
				double p0, p1, p2, p3, m0, m1;
				double frac2 = frac*frac;
				double frac3 = frac*frac2;
				double cminusb = c - b;
				double bias1 = 1. - bias;
				bias++;
				tension = 0.5 * (1. - tension);
				m0 = tension * ((b-a)*bias + cminusb*bias1);
				m1 = tension * (cminusb*bias + (d-c)*bias1);
				p2 = frac3 - frac2;
				p0 = 2*p2 - frac2 + 1.;
				p1 = p2 - frac2 + frac;
				p3 = frac2 - 2*p2;
				out[iblock] = (t_float)(p0*b + p1*m0 + p2*m1 + p3*c);	
			}
			else out[iblock] = 0;
		}
		
	}
	return;
}

static void wave_lagrange(t_wave *x,
	t_int *outp, t_float *xin, t_float *sin, t_float *ein,
	int nblock, int nch, int maxindex, float ksr, t_word **vectable)
{
	int iblock;
	for (iblock = 0; iblock < nblock; iblock++)
	{
		t_float phase = *xin++;
		t_float spos = *sin++ * ksr;
		t_float epos = *ein++ * ksr;
		BOUNDS_CHECK();
		INDEX_4PT();
		while (ch--)
		{
			t_word *vp = vectable[ch];
			t_float *out = (t_float *)(outp[ch]);
			if (vp)
			{
				a = (double)vp[ndxm1].w_float;
				b = (double)vp[ndx].w_float;
				c = (double)vp[ndx1].w_float;
				d = (double)vp[ndx2].w_float;
				double cminusb = c-b;
				out[iblock] = (t_float)(b + frac * (
					cminusb - (1. - frac)/6. * (
						(d - a - 3.0*cminusb) * frac + d + 2.0*a - 3.0*b
					)
				));	
			}
			else out[iblock] = 0;
		}
		
	}
	return;
}

static t_int *wave_perform(t_int *w)
{
    /* The jump table; "wif" = "wave interpolation functions." */
	
	static void (* const wif[])(t_wave *x,
		t_int *outp, t_float *xin, t_float *sin, t_float *ein,
		int nblock, int nch, int maxindex, float ksr, t_word **vectable) = 
	{
   		wave_nointerp,
    	wave_linear,
    	wave_linlq,
    	wave_cosine,
    	wave_cubic,
    	wave_spline,
    	wave_hermite,
    	wave_lagrange
    };
    
    t_wave *x = (t_wave *)(w[1]);
    int nblock = (int)(w[2]);
    t_cybuf * c = x->x_cybuf;
   // t_int *outp = w + 6;
    t_int *outp = (t_int *)x->x_ovecs;
    int nch = c->c_numchans;
    if (c->c_playable)
    {	
		//t_wave *x = (t_wave *)sic;
		int npts = c->c_npts;
		float ksr = x->x_ksr;
		t_word **vectable = c->c_vectors;
		//t_float *xin = (t_float *)(w[3]);
		//t_float *sin = (t_float *)(w[4]);
		//t_float *ein = (t_float *)(w[5]);
                t_float *xin = x->x_in;
                t_float *sin = x->x_st;
                t_float *ein = x->x_e;
		int maxindex = npts - 1;
		int interp_mode = x->x_interp_mode;
		/*Choose interpolation function from jump table. The interpolation functions also
		  perform the block loop in order not to make a bunch of per-sample decisions.*/	
		wif[interp_mode](x, outp, xin, sin, ein, nblock, nch, maxindex, ksr, vectable);
    }
    else
    {
	
	int ch = nch;
	while (ch--)
	{
	    t_float *out = (t_float *)outp[ch];
	    int n = nblock;
	    while (n--) *out++ = 0;
	}
    }
    return (w + 3);
}

static void wave_dsp(t_wave *x, t_signal **sp)
{
	
    cybuf_checkdsp(x->x_cybuf); 
    x->x_ksr = sp[0]->s_sr * 0.001;
	int i, nblock = sp[0]->s_n;

    t_signal **sigp = sp;
	x->x_in = (*sigp++)->s_vec; //the first sig in is the input
        x->x_st = (*sigp++)->s_vec; //next is the start input
        x->x_e = (*sigp++)->s_vec; //next is the end input
    for (i = 0; i < x->x_numouts; i++){ //now for the outputs
		*(x->x_ovecs+i) = (*sigp++)->s_vec;
	};
	dsp_add(wave_perform, 2, x, nblock);

}

static void wave_free(t_wave *x)
{

	inlet_free(x->x_startlet);
	inlet_free(x->x_endlet);
        cybuf_free(x->x_cybuf);
	 freebytes(x->x_ovecs, x->x_numouts * sizeof(*x->x_ovecs));
}

static void *wave_new(t_symbol *s, int argc, t_atom * argv){

	//mostly copying this for what i did with record~ - DXK
	t_symbol * name = NULL;
	int nameset = 0; //flag if name is set
	int floatarg = 0; //argument counter for floatargs (don't include symbol arg)
	//setting defaults
	t_float stpt = 0;
	t_float endpt = SHARED_FLT_MAX; //default to max float size (hacky i know)
    int numouts = 1; //i'm assuming the default is 1 - DXK
	t_float bias = 0;
	t_float tension = 0;
	t_float interp = CYWAVEINTERP;

	while(argc){
		if(argv->a_type == A_SYMBOL){ // symbol
			if(floatarg == 0 && !nameset){
				//we haven't hit any floatargs, go ahead and set name
				name = atom_getsymbolarg(0, argc, argv);
				nameset = 1; //set nameset flag
				argc--;
				argv++;
			}
			else if(nameset){
				t_symbol * curarg = atom_getsymbolarg(0, argc, argv); 
				//parse args only if array name is set, sound alright? -DXK
				if(argc >=2){ //needs to be at least two args left, the attribute symbol and the arg for it
					t_float curfl = atom_getfloatarg(1, argc, argv);
					if(strcmp(curarg->s_name, "@interp") == 0){
						interp = curfl;
					}
					else if(strcmp(curarg->s_name, "@interp_bias") == 0){
						bias = curfl;
					}
					else if(strcmp(curarg->s_name, "@interp_tension") == 0){
						tension = curfl;
					}
					else{
						goto errstate;
					};
					argc -=2;
					argv+=2;
				}
				else{
					goto errstate;
				};
			};
		}
		else{ // float
            if(nameset){
			//else we're dealing with a float
			switch(floatarg){
				case 0:
					stpt = atom_getfloatarg(0, argc, argv);
					break;
				case 1:
					endpt = atom_getfloatarg(0, argc, argv);
					break;
				case 2:
					numouts = (int)atom_getfloatarg(0, argc, argv);
					break;
				default:
					break;
			};
			floatarg++; //increment the floatarg we're looking at
			argc--;
			argv++;
            }
            else{
                nameset = 1; //set nameset flag and ignore array name
                argc--;
                argv++;
            };
		};
	};

    //some boundschecking
	if(numouts > CYWAVEMAXOUT){
		numouts = CYWAVEMAXOUT;
	}
	else if(numouts < 1){
		numouts = 1;
	};

        t_wave *x = (t_wave *)pd_new(wave_class);
        x->x_cybuf = cybuf_init((t_class *)x, name, numouts, 0);
        x->x_numouts = numouts;
	
        //allocating output vectors
        x->x_ovecs = getbytes(x->x_numouts * sizeof(*x->x_ovecs));
	
	//more boundschecking
	//it looks everything is stored as samples then later converted to ms
	if(stpt < 0){
		stpt = 0;
	}
	else{
		stpt = (t_float)floor(stpt); //basically typecasting to int without actual typecasting
	};

	if(endpt < 0){
			endpt = 0;
	}
	else{//if passed, floor it
		endpt = (t_float)floor(endpt);
	};

        x->x_ksr = (double)sys_getsr() * 0.001;
	wave_interp(x, interp);
	x->x_bias = bias;
	x->x_tension = tension;

	x->x_startlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	x->x_endlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_startlet, stpt);
	pd_float( (t_pd *)x->x_endlet, endpt);

	int i;
	for(i=0; i < numouts; i++){
		outlet_new(&x->x_obj, gensym("signal"));
	};

	return (x);
	errstate:
		post("wave~: improper args");
		return NULL;
	}

CYCLONE_OBJ_API void wave_tilde_setup(void)
{
    wave_class = class_new(gensym("wave~"),
			   (t_newmethod)wave_new,
			   (t_method)wave_free,
			   sizeof(t_wave), 0,
			   A_GIMME, 0);
    class_addmethod(wave_class, (t_method)wave_set,
		    gensym("set"), A_SYMBOL, 0);
    class_addmethod(wave_class, (t_method)wave_interp,
		    gensym("interp"), A_FLOAT, 0);
	class_addmethod(wave_class, (t_method)wave_interp_bias,
			gensym("interp_bias"), A_FLOAT, 0);
	class_addmethod(wave_class, (t_method)wave_interp_tension,
			gensym("interp_tension"), A_FLOAT, 0);
    class_addmethod(wave_class, (t_method)wave_dsp, gensym("dsp"), A_CANT, 0);
    class_domainsignalin(wave_class, -1);
    class_addfloat(wave_class, (t_method)wave_float);
}
