
#include <string.h>
#include "m_pd.h"

typedef struct _s2f{
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
}t_s2f;

static t_class *s2f_class;

static void s2f_tick(t_s2f *x){
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void s2f_bang(t_s2f *x){
    x->x_nleft = x->x_offset;
}

static void s2f_correct(t_s2f *x){
    int wason = x->x_on;
    x->x_offset = (x->x_rqoffset < x->x_nblock ? x->x_rqoffset : x->x_nblock - 1);
    x->x_npoints = x->x_deltime * x->x_ksr - x->x_nblock + x->x_offset;
    if((x->x_on = !x->x_stopped)){
        if(!wason) x->x_nleft = x->x_offset;
    }
    else if(wason)
        clock_unset(x->x_clock);
}

static void s2f_start(t_s2f *x){
    x->x_stopped = 0;
    if(!x->x_on){
        x->x_nleft = x->x_offset;
        x->x_on = 1;
    }
}

static void s2f_stop(t_s2f *x){
    x->x_stopped = 1;
    if (x->x_on){
        clock_unset(x->x_clock);
        x->x_on = 0;
    }
}

static void s2f_float(t_s2f *x, t_float f){
    if(f != 0.)
        s2f_start(x);
    else
        s2f_stop(x);
}

static void s2f_set(t_s2f *x, t_floatarg f){
    x->x_deltime = (f > 0. ? f : 0.);
    s2f_correct(x);
}

static void s2f_ms(t_s2f *x, t_floatarg f){
    x->x_deltime = (f > 0. ? f : 0.);
    s2f_correct(x);
    x->x_nleft = x->x_offset;
}

static void s2f_offset(t_s2f *x, t_floatarg f){
    int i = (int)f;
    x->x_rqoffset = (i >= 0 ? i : 0);
    s2f_correct(x);
}

static t_int *s2f_perform(t_int *w){
    t_s2f *x = (t_s2f *)(w[1]);
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

static void s2f_dsp(t_s2f *x, t_signal **sp){
    x->x_nblock = sp[0]->s_n;
    x->x_ksr = sp[0]->s_sr * 0.001;
    s2f_correct(x);
    x->x_nleft = x->x_offset;
    dsp_add(s2f_perform, 2, x, sp[0]->s_vec);
}

static void s2f_free(t_s2f *x){
    if (x->x_clock)
            clock_free(x->x_clock);
}

static void *s2f_new(t_symbol *s, int argc, t_atom * argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_s2f *x = (t_s2f *)pd_new(s2f_class);
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
    x->x_clock = clock_new(x, (t_method)s2f_tick);
    s2f_offset(x, offset);
    s2f_ms(x, interval);
	s2f_float(x, active);
    return(x);
	errstate:
		pd_error(x, "s2f~: improper args");
		return NULL;
}

void s2f_tilde_setup(void){
    s2f_class = class_new(gensym("s2f~"),
        (t_newmethod)s2f_new, (t_method)s2f_free, sizeof(t_s2f), 0, A_GIMME,0);
	class_domainsignalin(s2f_class, -1);
	class_addfloat(s2f_class, (t_method)s2f_float);
    class_addmethod(s2f_class, (t_method)s2f_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(s2f_class, (t_method)s2f_bang);
    class_addmethod(s2f_class, (t_method)s2f_ms, gensym("\f"), A_FLOAT, 0);
    class_addmethod(s2f_class, (t_method)s2f_offset,
		    gensym("offset"), A_FLOAT, 0);
    class_addmethod(s2f_class, (t_method)s2f_set,
                    gensym("set"), A_FLOAT, 0);
    class_addmethod(s2f_class, (t_method)s2f_start,
		    gensym("start"), 0);
    class_addmethod(s2f_class, (t_method)s2f_stop,
                    gensym("stop"), 0);
    class_sethelpsymbol(s2f_class, gensym("sig2float~"));
}

