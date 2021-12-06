// Porres 2017

#include "m_pd.h"

static t_class *randf_class;

typedef struct _randf
{
    t_object   x_obj;
    int        x_val;
    t_float    x_randf;
    t_float    x_lastin;
    t_int      x_trig_bang;
    t_inlet   *x_low_let;
    t_inlet   *x_high_let;
    t_outlet  *x_outlet;
} t_randf;

static void randf_seed(t_randf *x, t_floatarg f)
{
    x->x_val = (int)f * 1319;
}

static void randf_bang(t_randf *x){
    x->x_trig_bang = 1;
}

static t_int *randf_perform(t_int *w)
{
    t_randf *x = (t_randf *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_sample *)(w[6]);
    int val = x->x_val;
    t_float randf = x->x_randf;
    t_float lastin = x->x_lastin;
    while (nblock--)
        {
        float trig = *in1++;
        float out_low = *in2++; // Output LOW
        float out_high = *in3++; // Output HIGH
        float range = out_high - out_low; // range
        t_int trigger = 0;
        if(x->x_trig_bang){
            trigger = 1;
            x->x_trig_bang = 0;
        }
        else
            trigger = (trig > 0 && lastin <= 0) || (trig < 0 && lastin >= 0);
        if(trigger){ // update
            randf = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
            randf = out_low + range * (randf + 1) / 2;
            val = val * 435898247 + 382842987;
        }
        *out++ = randf;
        lastin = trig;
        }
    x->x_val = val;
    x->x_randf = randf; // current output
    x->x_lastin = lastin; // last input
    return(w+7);
}

static void randf_dsp(t_randf *x, t_signal **sp)
{
    dsp_add(randf_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *randf_free(t_randf *x)
{
    inlet_free(x->x_low_let);
    inlet_free(x->x_high_let);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *randf_new(t_symbol *s, int ac, t_atom *av){
    t_randf *x = (t_randf *)pd_new(randf_class);
    t_symbol *dummy = s;
    dummy = NULL;
/////////////////////////////////////////////////////////////////////////////////////
    float low;
    float high;
    if(ac)
        {
        if(ac == 1)
            {
            if(av -> a_type == A_FLOAT)
                {
                low = 0;
                high = atom_getfloat(av);
                }
            else goto errstate;
            }
        else if(ac == 2)
            {
            int argnum = 0;
            while(ac)
                {
                if(av -> a_type != A_FLOAT)
                    {
                    goto errstate;
                    }
                else
                    {
                    t_float curf = atom_getfloatarg(0, ac, av);
                    switch(argnum)
                        {
                        case 0:
                            low = curf;
                            break;
                        case 1:
                            high = curf;
                            break;
                        default:
                            break;
                        };
                    };
                    argnum++;
                    ac--;
                    av++;
                };
            }
        else goto errstate;
        }
    else
        {
        low = -1;
        high = 1;
        }
/////////////////////////////////////////////////////////////////////////////////////
    static int init_seed = 74599;
    init_seed *= 1319;
    t_int seed = init_seed;
    x->x_val = seed; // load seed value
//
    x->x_lastin = 0;
//
    x->x_low_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_low_let, low);
    x->x_high_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_high_let, high);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
//
    return (x);
//
    errstate:
        pd_error(x, "randf~: improper args");
        return NULL;
}

void setup_rand0x2ef_tilde(void){
    randf_class = class_new(gensym("rand.f~"), (t_newmethod)randf_new,
        (t_method)randf_free, sizeof(t_randf), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(randf_class, nullfn, gensym("signal"), 0);
    class_addmethod(randf_class, (t_method)randf_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randf_class, (t_method)randf_seed, gensym("seed"), A_DEFFLOAT, 0);
    class_addbang(randf_class, (t_method)randf_bang);
}
