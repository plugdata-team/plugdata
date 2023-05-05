// porres 2023, inspired by and modified from Tom Erbe's [+delay~]

#include <math.h>
#include "m_pd.h"
#include "buffer.h"

#define DELSIZE 1048576

static t_class *fdelay_class;

typedef struct _fdelay{
	t_object    x_obj;
    t_inlet    *x_delt_in;
    t_inlet    *x_fb_in;
	float       x_sr;
	long        x_readPos;
	long        x_writePos;
	long        x_delaySize;
	long        x_delayMask;
	float       x_delay[DELSIZE];
	float       x_cutoff;
	float       x_reson;
    float       x_xfade;
	int         x_freeze;
	float       X1;
	float       X2;
	float       Y1;
	float       Y2;
    float       dcIn;
	float       dcOut;
}t_fdelay;

static void fdelay_cutoff(t_fdelay *x, t_floatarg f){
    x->x_cutoff = f < 20.0 ? 20.0 : f > 20000.0 ? 20000.0 : f;
}

static void fdelay_reson(t_fdelay *x, t_floatarg f){
    x->x_reson = f < 0 ? 0 : f > 1 ? 1 : f;
}

static void fdelay_freeze(t_fdelay *x, t_floatarg f){
    x->x_freeze = (int)(f != 0);
}

static void fdelay_wet(t_fdelay *x, t_floatarg f){
    x->x_xfade = (f < 0 ? 0 : f > 1 ? 1 : f) * M_PI * 0.5;
}

static void fdelay_clear(t_fdelay *x){
    for(int i = 0; i < DELSIZE; i++) // clear delay
        x->x_delay[i] = 0.0;
    x->X1 = x->X2 = x->Y1 = x->Y2 = 0.0; // clear biquad filter
    x->dcOut = x->dcIn = 0.0f; // clear DC filter
}

static t_int *fdelay_perform(t_int *w){
	t_fdelay *x = (t_fdelay *)(w[1]);
	t_float  *input = (t_float *)(w[2]);
    t_float  *tin = (t_float *)(w[3]);
    t_float  *fbin = (t_float *)(w[4]);
	t_float  *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
    // filter coefficients get calculated only once per block
	float dcR = 1.0f - (126.0f/x->x_sr);
    float cutoff = x->x_cutoff;
    float filterQ = (1.0 - x->x_reson) * (sqrt(2.0) - 0.1) + 0.1;
    if(cutoff > x->x_sr * 0.5)
        cutoff = x->x_sr * 0.5;
    float f0 = cutoff/x->x_sr;
    float C = (f0 < 0.1) ? 1.0 / (f0 * M_PI) : tan((0.5 - f0) * M_PI);
    float A1 = 1.0 / ( 1.0 + filterQ * C + C * C);
    float A2 = 2.0 * A1;
    float A3 = A1;
    float B1 = 2.0 * ( 1.0 - C * C) * A1;
    float B2 = ( 1.0 - filterQ * C + C * C) * A1;
	for(int i = 0; i < n; i++){
        t_float dry = input[i];
        t_float in = dry;
        t_float time = tin[i];
        t_float fb = fbin[i];
        if(x->x_freeze){ // freeze check
            in = 0;
            fb = fb >= 0 ? 1 : -1;
        }
// determine the delay time and the readPos
		float delayTime = time * x->x_sr/1000; // time in samples
        if(delayTime < 1)
            delayTime = 1;
        if(delayTime >= DELSIZE - 2)
            delayTime = DELSIZE - 2;
        float delayTimeL = delayTime + 2.0; // a 2 sample buffer for interpolation
		long delayLong = (long)delayTimeL;
		float frac = 1.0 - (delayTimeL - (float)delayLong);
		x->x_readPos = x->x_writePos - delayLong;
		x->x_readPos &= x->x_delayMask;
// read from delay line with interpolation
        float a = x->x_delay[(x->x_readPos - 1) & x->x_delayMask];
        float b = x->x_delay[(x->x_readPos + 0) & x->x_delayMask];
        float c = x->x_delay[(x->x_readPos + 1) & x->x_delayMask];
        float d = x->x_delay[(x->x_readPos + 2) & x->x_delayMask];
        float del = interp_spline(frac, a, b, c, d);
// lowpass resonant filter
        float filter = A1*del + A2*x->X1 + A3*x->X2 - B1*x->Y1 - B2*x->Y2;
        x->X2 = x->X1;
        x->X1 = del;
        x->Y2 = x->Y1;
        x->Y1 = filter;
        if(x->x_cutoff >= 20000) // bypass the filter
            filter = del;
        if(fabs(fb) > 1.0f){ // DC filter
            float dcOut0 = filter - x->dcIn + dcR * x->dcOut;
            x->dcIn = filter;
            x->dcOut = filter = dcOut0;
        }
        float wet = atan(filter); // softclip
        *(x->x_delay+x->x_writePos) = (in + wet*fb); // write to the delay line
        out[i] = dry*cos(x->x_xfade) + wet*sin(x->x_xfade);
		x->x_writePos++;
		x->x_writePos &= x->x_delayMask;
	}
	return(w+7);
};

static void fdelay_dsp(t_fdelay *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(fdelay_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec,
        sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
};

static void *fdelay_free(t_fdelay *x){
    inlet_free(x->x_delt_in);
    inlet_free(x->x_fb_in);
    return(void *)x;
}

static void *fdelay_new(t_symbol *s, int ac, t_atom *av){
    t_fdelay *x = (t_fdelay *)pd_new(fdelay_class);
    float time = 0, fb = 0, cutoff = 20000.0, reson = 0.0, wet = 0.5;
    x->x_freeze = 0;
    x->x_sr = sys_getsr();
    if(ac){
        while(av->a_type == A_SYMBOL){
            s = atom_getsymbol(av);
            if(s == gensym("-cutoff")){
                ac--, av++;
                cutoff = atom_getfloat(av);
            }
            else if(s == gensym("-reson")){
                ac--, av++;
                reson = atom_getfloat(av);
            }
            else if(s == gensym("-wet")){
                ac--, av++;
                wet = atom_getfloat(av);
            }
            ac--, av++;
        }
        time = atom_getfloat(av);
        ac--, av++;
        fb = atom_getfloat(av);
    }
    x->x_cutoff = cutoff < 20 ? 20 : cutoff > 20000 ? 20000 : cutoff;
    x->x_reson = reson < 0 ? 0 : reson > 1 ? 1 : reson;
    fdelay_wet(x, wet);
    x->x_readPos = x->x_writePos = 0.0;
    x->x_delaySize = DELSIZE, x->x_delayMask = DELSIZE-1;
    for(int i = 0; i < DELSIZE; i++) // init delay memory
        x->x_delay[i] = 0.0;
    x->X1 = x->X2 = x->Y1 = x->Y2 = 0.0; // biquad filter memory
    x->dcOut = x->dcIn = 0.0f; // DC filter memory
    x->x_delt_in = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_delt_in, time);
    x->x_fb_in = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_fb_in, fb);
    outlet_new(&x->x_obj, &s_signal);
    return(void *)x;
};

void filterdelay_tilde_setup(void){
	fdelay_class = class_new(gensym("filterdelay~"), (t_newmethod)fdelay_new,
        (t_method)fdelay_free, sizeof(t_fdelay), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(fdelay_class, nullfn, gensym("signal"), 0);
	class_addmethod(fdelay_class, (t_method)fdelay_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(fdelay_class, (t_method)fdelay_clear, gensym("clear"), 0);
	class_addmethod(fdelay_class, (t_method)fdelay_cutoff, gensym("cutoff"), A_DEFFLOAT, 0);
	class_addmethod(fdelay_class, (t_method)fdelay_reson, gensym("reson"), A_DEFFLOAT, 0);
	class_addmethod(fdelay_class, (t_method)fdelay_freeze, gensym("freeze"), A_DEFFLOAT, 0);
    class_addmethod(fdelay_class, (t_method)fdelay_wet, gensym("wet"), A_DEFFLOAT, 0);
}
