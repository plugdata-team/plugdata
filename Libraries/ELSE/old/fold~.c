
#include "m_pd.h"

static t_class *fold_tilde_class;

typedef struct _fold_tilde {
	t_object x_obj;
	t_float     x_min;
	t_float     x_max;
	t_float     x_in; // dummy
	t_inlet     *x_minlet;
	t_inlet     *x_maxlet;
	t_outlet    *x_outlet;
} t_fold_tilde;

static t_int *fold_tilde_perform(t_int *w)
{
	t_fold_tilde *x = (t_fold_tilde *)(w[1]);
	t_float *in1 = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *in3 = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
	while (n--){
		float in = *in1++;
		float min = *in2++;
		float max = *in3++;
        float output;
		if(min > max)
            { // swap values
			float temp;
			temp = max;
			max = min;
			min = temp;
			};
        if(min == max)
            output = min;
        else if(in <= max && in >= min)
            output = in; // if in range, = in
        else
            { // folding
            float range = max - min;
            if(in < min)
                {
                float diff = min - in; // positive diff between in and min
                int mag = (int)(diff/range); // in is > range from min
                if(mag % 2 == 0)
                    { // even # of ranges away = counting up from min
                    diff = diff - ((float)mag * range);
                    output = diff + min;
                    }
                else
                    { // odd # of ranges away = counting down from max
                    diff = diff - ((float)mag * range);
                    output = max - diff;
                    };
                }
            else // in > max
                {
                float diff = in - max; // positive diff between in and max
                int mag  = (int)(diff/range); // in is > range from max
                if(mag % 2 == 0)
                    { // even # of ranges away = counting down from max
                    diff = diff - ((float)mag * range);
                    output = max - diff;
                    }
                else
                    { // odd # of ranges away = counting up from min
                    diff = diff - ((float)mag * range);
                    output = diff + min;
                    };
                };
            }
		*out++ = output;
		};
	return (w+7);
}

static void fold_tilde_dsp(t_fold_tilde *x, t_signal **sp)
{
		dsp_add(fold_tilde_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec,
                sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

static void *fold_tilde_new(t_symbol *s, int argc, t_atom *argv){
    t_fold_tilde *x = (t_fold_tilde *)pd_new(fold_tilde_class);
///////////////////////////
    x->x_min = -1.;
    x-> x_max = 1.;
    if(argc == 1)
    {
        if(argv -> a_type == A_FLOAT)
        {
            x->x_min = 0;
            x->x_max = atom_getfloat(argv);
        }
        else goto errstate;
    }
    else if(argc == 2)
        {
        int numargs = 0;
        while(argc > 0 )
            {
            if(argv -> a_type == A_FLOAT)
                { // if nullpointer, should be float or int
                switch(numargs)
                    {
                    case 0: x->x_min = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_max = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    default:
                        argc--;
                        argv++;
                        break;
                    };
                }
            else // not a float
                goto errstate;
            };
        }
    else if(argc > 2)
        goto errstate;
///////////////////////////
    x->x_minlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_maxlet =  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float( (t_pd *) x->x_minlet, x->x_min);
    pd_float( (t_pd *) x->x_maxlet, x->x_max);
    x->x_outlet =  outlet_new(&x->x_obj, gensym("signal"));
    return (x);
errstate:
    pd_error(x, "fold~: improper args");
    return NULL;
}

void fold_tilde_setup(void){
	fold_tilde_class = class_new(gensym("fold~"), (t_newmethod)fold_tilde_new, 0,
			sizeof(t_fold_tilde), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(fold_tilde_class, (t_method)fold_tilde_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(fold_tilde_class, t_fold_tilde, x_in);
}
