
#include <string.h>
#include "m_pd.h"

typedef struct _sig2float{
    t_object  x_obj;
    t_float   x_value;
    int       x_rqoffset;  /* requested */
    int       x_offset;    /* effective (truncated) */
    int       x_stopped;
    int       x_on;        /* !stopped && deltime > 0 */
    float     x_deltime;
    int       x_npoints;
    int       x_nleft;
    int       x_nblock;
    float     x_ksr;
    t_clock  *x_clock;
}t_sig2float;

static t_class *sig2float_class;

static void sig2float_tick(t_sig2float *x){
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void sig2float_bang(t_sig2float *x){
    x->x_nleft = x->x_offset;
}

static void sig2float_correct(t_sig2float *x){
    int wason = x->x_on;
    x->x_offset = (x->x_rqoffset < x->x_nblock ? x->x_rqoffset : x->x_nblock - 1);
    x->x_npoints = x->x_deltime * x->x_ksr - x->x_nblock + x->x_offset;
    if((x->x_on = !x->x_stopped)){
        if(!wason) x->x_nleft = x->x_offset;
    }
    else if(wason)
        clock_unset(x->x_clock);
}

static void sig2float_start(t_sig2float *x){
    x->x_stopped = 0;
    if(!x->x_on){
        x->x_nleft = x->x_offset;
        x->x_on = 1;
    }
}

static void sig2float_stop(t_sig2float *x){
    x->x_stopped = 1;
    if (x->x_on){
        clock_unset(x->x_clock);
        x->x_on = 0;
    }
}

static void sig2float_float(t_sig2float *x, t_float f){
    if(f != 0.)
        sig2float_start(x);
    else
        sig2float_stop(x);
}

static void sig2float_set(t_sig2float *x, t_floatarg f){
    x->x_deltime = (f > 0. ? f : 0.);
    sig2float_correct(x);
}

static void sig2float_ms(t_sig2float *x, t_floatarg f){
    x->x_deltime = (f > 0. ? f : 0.);
    sig2float_correct(x);
    x->x_nleft = x->x_offset;
}

static void sig2float_offset(t_sig2float *x, t_floatarg f){
    int i = (int)f;
    x->x_rqoffset = (i >= 0 ? i : 0);
    sig2float_correct(x);
}

static t_int *sig2float_perform(t_int *w){
    t_sig2float *x = (t_sig2float *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    x->x_value = in[x->x_offset];
    if(x->x_on){
        if(x->x_nleft < x->x_nblock){
            clock_delay(x->x_clock, 0);
            x->x_nleft = x->x_npoints;
        }
        else
            x->x_nleft -= x->x_nblock;
    }
    return(w + 3);
}

static void sig2float_dsp(t_sig2float *x, t_signal **sp){
    x->x_nblock = sp[0]->s_n;
    x->x_ksr = sp[0]->s_sr * 0.001;
    sig2float_correct(x);
    x->x_nleft = x->x_offset;
    dsp_add(sig2float_perform, 2, x, sp[0]->s_vec);
}

static void sig2float_free(t_sig2float *x){
    if (x->x_clock)
            clock_free(x->x_clock);
}

static void *sig2float_new(t_symbol *s, int argc, t_atom * argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_sig2float *x = (t_sig2float *)pd_new(sig2float_class);
    x->x_stopped = 0;
    x->x_on = 0;
    x->x_value = 0;
    x->x_nblock = 64;  // ????
    x->x_ksr = 44.1;  // ????
	t_float interval, offset, active;
	interval = 0.;
	offset = 0.;
	active = 1;
	int argnum = 0;
	while(argc > 0){
		if(argv -> a_type == A_FLOAT){
			t_float argval = atom_getfloatarg(0, argc, argv);
			switch(argnum){
				case 0:
					interval = argval;
					break;
				case 1:
					offset = argval;
					break;
				default:
					break;
			};
			argnum++;
			argc--;
			argv++;
		}
		else if(argv -> a_type == A_SYMBOL){
			t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(curarg->s_name, "-off")){
                active = 0;
                argc--;
                argv++;
			}
			else
				goto errstate;
		}
		else
			goto errstate;
	};
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("\f"));  // hack
    outlet_new(&x->x_obj, &s_float);
    x->x_clock = clock_new(x, (t_method)sig2float_tick);
    sig2float_offset(x, offset);
    sig2float_ms(x, interval);
	sig2float_float(x, active);
    return(x);
	errstate:
		pd_error(x, "sig2float~: improper args");
		return NULL;
}

void sig2float_tilde_setup(void){
    sig2float_class = class_new(gensym("sig2float~"),
        (t_newmethod)sig2float_new, (t_method)sig2float_free, sizeof(t_sig2float), 0, A_GIMME,0);
	class_domainsignalin(sig2float_class, -1);
	class_addfloat(sig2float_class, (t_method)sig2float_float);
    class_addmethod(sig2float_class, (t_method)sig2float_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(sig2float_class, (t_method)sig2float_bang);
    class_addmethod(sig2float_class, (t_method)sig2float_ms, gensym("\f"), A_FLOAT, 0);
    class_addmethod(sig2float_class, (t_method)sig2float_offset,
		    gensym("offset"), A_FLOAT, 0);
    class_addmethod(sig2float_class, (t_method)sig2float_set,
                    gensym("set"), A_FLOAT, 0);
    class_addmethod(sig2float_class, (t_method)sig2float_start,
		    gensym("start"), 0);
    class_addmethod(sig2float_class, (t_method)sig2float_stop,
		    gensym("stop"), 0);
}

