// Porres 2016

#include "m_pd.h"
#include <common/api.h>
#include <math.h>
#include <string.h>

#define SCALE_MININ  0.
#define SCALE_MAXIN  127.
#define SCALE_MINOUT  0.
#define SCALE_MAXOUT  1.
#define SCALE_EXPO  1.

typedef struct _scale
{
    t_object x_obj;
    t_inlet  *x_inlet_1;
    t_inlet  *x_inlet_2;
    t_inlet  *x_inlet_3;
    t_inlet  *x_inlet_4;
    t_inlet  *x_inlet_5;
    t_int    x_classic;
} t_scale;

static t_class *scale_class;


static void scale_classic(t_scale *x, t_floatarg f)
{
    x->x_classic = (int)f;
}

static t_int *scale_perform(t_int *w)
{
    t_scale *x = (t_scale *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *in4 = (t_float *)(w[6]);
    t_float *in5 = (t_float *)(w[7]);
    t_float *in6 = (t_float *)(w[8]);
    t_float *out = (t_float *)(w[9]);
    t_int classic_flag = x->x_classic;
    while (nblock--)
    {
    float in = *in1++;
    float il = *in2++; // Input LOW
    float ih = *in3++; // Input HIGH
    float ol = *in4++; // Output LOW
    float oh = *in5++; // Output HIGH
    float p = *in6++; // power (exponential) factor
    float output;
    if (classic_flag != 0)
        {p = p <= 1 ? 1 : p;
        if (p == 1) output = ((in - il) / (ih - il) == 0) ? ol
            : (((in - il) / (ih - il)) > 0) ? (ol + (oh - ol) * pow((in - il) / (ih - il), p))
            : (ol + (oh - ol) * -(pow(((-in + il) / (ih - il)), p)));
        else
            {output = ol + (oh - ol) * ((oh - ol) * exp((il - ih) * log(p)) * exp(in * log(p)));
            if (oh - ol <= 0) output = output * -1;
            
            }
        }
        else {
        p = p <= 0 ? 0 : p;
        output = ((in - il) / (ih - il) == 0) ? ol :
        (((in - il) / (ih - il)) > 0) ?
        (ol + (oh - ol) * pow((in - il) / (ih - il), p)) :
        (ol + (oh - ol) * -(pow(((-in + il) / (ih - il)), p)));
        }
    *out++ = output;
    }
    return (w + 10);
}

static void scale_dsp(t_scale *x, t_signal **sp)
{
    dsp_add(scale_perform, 9, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void *scale_free(t_scale *x)
{
		inlet_free(x->x_inlet_1);
        inlet_free(x->x_inlet_2);
        inlet_free(x->x_inlet_3);
        inlet_free(x->x_inlet_4);
        inlet_free(x->x_inlet_5);
        return (void *)x;
}

static void *scale_new(t_symbol *s, int argc, t_atom *argv)
{
    t_scale *x = (t_scale *)pd_new(scale_class);
    t_float min_in, max_in, min_out, max_out, exponential;
    min_in = SCALE_MININ;
    max_in = SCALE_MAXIN;
    min_out = SCALE_MINOUT;
    max_out = SCALE_MAXOUT;
    exponential = SCALE_EXPO;
    t_int classic_exp;
    classic_exp = 0;
    
	int argnum = 0;
	while(argc > 0){
		if(argv -> a_type == A_FLOAT)
        {
			t_float argval = atom_getfloatarg(0,argc,argv);
				switch(argnum){
					case 0:
						min_in = argval;
						break;
					case 1:
						max_in = argval;
						break;
                    case 2:
                        min_out = argval;
                        break;
                    case 3:
                        max_out = argval;
                        break;
                    case 4:
                        exponential = argval;
                        break;
					default:
						break;
				};
				argc--;
				argv++;
				argnum++;
        }
        
// attribute
        else if(argv -> a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(strcmp(curarg->s_name, "@classic")==0){
                if(argc >= 2){
                    t_float argval = atom_getfloatarg(1, argc, argv);
                    classic_exp = (int)argval;
                    argc-=2;
                    argv+=2;
                }
                else{
                    goto errstate;
                };
            }
            else{
                goto errstate;
            };
            
        }
        else{
            goto errstate;
        };
    };
    
	x->x_inlet_1 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_inlet_1, min_in);
    x->x_inlet_2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_2, max_in);
    x->x_inlet_3 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_3, min_out);
    x->x_inlet_4 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_4, max_out);
    x->x_inlet_5 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_5, exponential);

    outlet_new((t_object *)x, &s_signal);
    
    x->x_classic = classic_exp;
    
    return (x);
	errstate:
		pd_error(x, "scale~: improper args");
		return NULL;
}

CYCLONE_OBJ_API void scale_tilde_setup(void)
{
    scale_class = class_new(gensym("scale~"),
				(t_newmethod)scale_new,
                (t_method)scale_free,
				sizeof(t_scale), 0, A_GIMME, 0);
    class_addmethod(scale_class, nullfn, gensym("signal"), 0);
    class_addmethod(scale_class, (t_method)scale_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(scale_class, (t_method) scale_classic, gensym("classic"), A_DEFFLOAT, 0);
}
