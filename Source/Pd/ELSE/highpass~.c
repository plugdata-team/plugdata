// Porres 2017

#include "m_pd.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define HALF_LOG2 log(2)/2

typedef struct _highpass {
    t_object    x_obj;
    t_inlet    *x_inlet_freq; // x_inlet_freq
    t_inlet    *x_inlet_q;
    t_outlet   *x_out;
    t_float     x_nyq;
    int     x_bypass;
    int     x_bw;
    double  x_xnm1;
    double  x_xnm2;
    double  x_ynm1;
    double  x_ynm2;
    } t_highpass;

static t_class *highpass_class;

static t_int *highpass_perform(t_int *w)
{
    t_highpass *x = (t_highpass *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    double xnm1 = x->x_xnm1;
    double xnm2 = x->x_xnm2;
    double ynm1 = x->x_ynm1;
    double ynm2 = x->x_ynm2;
    t_float nyq = x->x_nyq;
    while (nblock--)
    {
        double xn = *in1++, f = *in2++, reson = *in3++;
        double q, omega, alphaQ, cos_w, a0, a1, a2, b0, b1, b2, yn;
        
        if (f < 0.000001)
            f = 0.000001;
        if (f > nyq - 0.000001)
            f = nyq - 0.000001;
        
        omega = f * PI/nyq; // hz2rad
        
        if (x->x_bw) // reson is bw in octaves
            {
            if (reson < 0.000001)
                reson = 0.000001;
            q = 1 / (2 * sinh(HALF_LOG2 * reson * omega/sin(omega)));
            }
        else
            q = reson;
            
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1 + cos_w) / (2 * b0);
        a1 = -(1 + cos_w) / b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
        
        yn = a0 * xn + a1 * xnm1 + a2 * xnm2 + b1 * ynm1 + b2 * ynm2;
        
        if(x->x_bypass)
            *out++ = xn;
        else
            *out++ = yn;
        
        xnm2 = xnm1;
        xnm1 = xn;
        ynm2 = ynm1;
        ynm1 = yn;
    }
    x->x_xnm1 = xnm1;
    x->x_xnm2 = xnm2;
    x->x_ynm1 = ynm1;
    x->x_ynm2 = ynm2;
    return (w + 7);
}

static void highpass_dsp(t_highpass *x, t_signal **sp)
{
    x->x_nyq = sp[0]->s_sr / 2;
    dsp_add(highpass_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void highpass_clear(t_highpass *x)
{
    x->x_xnm1 = x->x_xnm2 = x->x_ynm1 = x->x_ynm2 = 0.;
}

static void highpass_bypass(t_highpass *x, t_floatarg f)
{
    x->x_bypass = (int)(f != 0);
}

static void highpass_bw(t_highpass *x)
{
    x->x_bw = 1;
}

static void highpass_q(t_highpass *x)
{
    x->x_bw = 0;
}

static void *highpass_new(t_symbol *s, int argc, t_atom *argv)
{
    t_highpass *x = (t_highpass *)pd_new(highpass_class);
    float freq = 0;
    float reson = 1;
    int bw = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0)
    {
        if(argv -> a_type == A_FLOAT)
        { //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum)
            {
                case 0:
                    freq = argval;
                    break;
                case 1:
                    reson = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else if (argv -> a_type == A_SYMBOL)
        {
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(strcmp(curarg->s_name, "-bw")==0)
            {
                bw = 1;
                argc -= 1;
                argv += 1;
            }
            else
            {
                goto errstate;
            };
        }
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_bw = bw;
    
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, freq);
    x->x_inlet_q = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_q, reson);
    x->x_out = outlet_new((t_object *)x, &s_signal);

    return (x);
    errstate:
        pd_error(x, "highpass~: improper args");
        return NULL;
}

void highpass_tilde_setup(void)
{
    highpass_class = class_new(gensym("highpass~"), (t_newmethod)highpass_new, 0,
        sizeof(t_highpass), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(highpass_class, (t_method)highpass_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(highpass_class, nullfn, gensym("signal"), 0);
    class_addmethod(highpass_class, (t_method)highpass_clear, gensym("clear"), 0);
    class_addmethod(highpass_class, (t_method)highpass_bypass, gensym("bypass"), A_DEFFLOAT, 0);
    class_addmethod(highpass_class, (t_method)highpass_bw, gensym("bw"), 0);
    class_addmethod(highpass_class, (t_method)highpass_q, gensym("q"), 0);
}
