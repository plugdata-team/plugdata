/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//updating argument parsing for object creation, adding interval and active attributes - Derek Kwan 2016
#include <string.h>
#include "m_pd.h"
#include <common/api.h>

/* CHECKME for a fixed minimum deltime, if any (5ms for c74's metro) */

#define PDCYSSINTV 0.
#define PDCYSSOFFSET 0
#define PDCYSSACTIVE 1

typedef struct _snapshot
{
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
} t_snapshot;

static t_class *snapshot_class;

static void snapshot_tick(t_snapshot *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void snapshot_bang(t_snapshot *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void snapshot_correct(t_snapshot *x)
{
    int wason = x->x_on;
    x->x_offset =
	(x->x_rqoffset < x->x_nblock ? x->x_rqoffset : x->x_nblock - 1);
    x->x_npoints = x->x_deltime * x->x_ksr - x->x_nblock + x->x_offset;
    if((x->x_on = (!x->x_stopped && x->x_deltime > 0.))){
        if(!wason)
            x->x_nleft = x->x_offset;  /* CHECKME */
    }
    else if (wason) clock_unset(x->x_clock);
}

static void snapshot_start(t_snapshot *x)
{
    x->x_stopped = 0;
    if (!x->x_on && x->x_deltime > 0.)  /* CHECKED no default */
    {
	x->x_nleft = x->x_offset;  /* CHECKME */
	x->x_on = 1;
    }
}

static void snapshot_stop(t_snapshot *x)
{
    x->x_stopped = 1;
    if (x->x_on)
    {
	clock_unset(x->x_clock);
	x->x_on = 0;
    }
}

static void snapshot_float(t_snapshot *x, t_float f)
{
    /* CHECKED nonzero/zero, CHECKED incompatible: int only (float ignored) */
    if (f != 0.)
	snapshot_start(x);
    else
	snapshot_stop(x);
}

static void snapshot_ft1(t_snapshot *x, t_floatarg f)
{
    x->x_deltime = (f > 0. ? f : 0.);  /* CHECKED */
    /* CHECKED setting deltime to a positive value starts the clock
       only if it was stopped by setting deltime to zero */
    snapshot_correct(x);
}

static void snapshot_offset(t_snapshot *x, t_floatarg f)
{
    int i = (int)f;  /* CHECKME */
    x->x_rqoffset = (i >= 0 ? i : 0);  /* CHECKME */
    /* CHECKME if the change has an effect prior to next dsp_add call */
    snapshot_correct(x);
}

static void snapshot_sampleinterval(t_snapshot *x, t_floatarg f)
{
    int samples = f > 0. ? (int)f : 0.;
    x->x_deltime = samples / x->x_ksr;
    snapshot_correct(x);
}

static t_int *snapshot_perform(t_int *w)
{
    t_snapshot *x = (t_snapshot *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    x->x_value = in[x->x_offset];
    if (x->x_on)
    {
	/* CHECKME nleft vs offset */
	if (x->x_nleft < x->x_nblock)
	{
	    clock_delay(x->x_clock, 0);
	    x->x_nleft = x->x_npoints;
	}
	else x->x_nleft -= x->x_nblock;
    }
    return (w + 3);
}

static void snapshot_dsp(t_snapshot *x, t_signal **sp)
{
    x->x_nblock = sp[0]->s_n;
    x->x_ksr = sp[0]->s_sr * 0.001;
    snapshot_correct(x);
    x->x_nleft = x->x_offset;  /* CHECKME */
    dsp_add(snapshot_perform, 2, x, sp[0]->s_vec);
}

static void snapshot_free(t_snapshot *x)
{
    if (x->x_clock) clock_free(x->x_clock);
}

static void *snapshot_new(t_symbol *s, int argc, t_atom * argv){
    s = NULL;
    t_snapshot *x = (t_snapshot *)pd_new(snapshot_class);
    x->x_stopped = 0;  /* CHECKED */
    x->x_on = 0;
    x->x_value = 0;
    x->x_nblock = 64;  /* redundant */
    x->x_ksr = 44.1;  /* redundant */
	t_float interval, offset, active;
	interval = PDCYSSINTV;
	offset = PDCYSSOFFSET;
	active = PDCYSSACTIVE;
	int argnum = 0;
	while(argc > 0){
		if(argv->a_type == A_FLOAT){
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
		else if(argv->a_type == A_SYMBOL){
			t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
			if(strcmp(curarg->s_name, "@interval")==0){
				if(argc >= 2){
					t_float argval = atom_getfloatarg(1, argc, argv);
					interval = argval;
					argc-=2;
					argv+=2;
				}
				else{
					goto errstate;
				};
			}
			else if(strcmp(curarg->s_name, "@active")==0){
				if(argc >= 2){
					t_float argval = atom_getfloatarg(1, argc, argv);
					active = argval;
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
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,&s_float, gensym("ft1"));
    outlet_new(&x->x_obj, &s_float);
    x->x_clock = clock_new(x, (t_method)snapshot_tick);
    snapshot_offset(x, offset);  /* CHECKME (this is fixed at nblock-1 in Pd) */
    snapshot_ft1(x, interval);
	snapshot_float(x, active);
    return (x);
	errstate:
		pd_error(x, "snapshot~: improper args");
		return NULL;
}

CYCLONE_OBJ_API void snapshot_tilde_setup(void){
    snapshot_class = class_new(gensym("cyclone/snapshot~"),
        (t_newmethod)snapshot_new, (t_method)snapshot_free, sizeof(t_snapshot), 0, A_GIMME,0);
	class_domainsignalin(snapshot_class, -1);
	class_addfloat(snapshot_class, (t_method)snapshot_float);
    class_addmethod(snapshot_class, (t_method)snapshot_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(snapshot_class, (t_method)snapshot_bang);
    class_addmethod(snapshot_class, (t_method)snapshot_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(snapshot_class, (t_method)snapshot_offset, gensym("offset"), A_FLOAT, 0);
    class_addmethod(snapshot_class, (t_method)snapshot_start, gensym("start"), 0);
    class_addmethod(snapshot_class, (t_method)snapshot_stop, gensym("stop"), 0);
    class_addmethod(snapshot_class, (t_method)snapshot_sampleinterval, gensym("sampleinterval"), A_FLOAT, 0);
}

CYCLONE_OBJ_API void Snapshot_tilde_setup(void){
    snapshot_class = class_new(gensym("Snapshot~"),
        (t_newmethod)snapshot_new, (t_method)snapshot_free, sizeof(t_snapshot), 0, A_GIMME,0);
    class_domainsignalin(snapshot_class, -1);
    class_addfloat(snapshot_class, (t_method)snapshot_float);
    class_addmethod(snapshot_class, (t_method)snapshot_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(snapshot_class, (t_method)snapshot_bang);
    class_addmethod(snapshot_class, (t_method)snapshot_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(snapshot_class, (t_method)snapshot_offset, gensym("offset"), A_FLOAT, 0);
    class_addmethod(snapshot_class, (t_method)snapshot_start, gensym("start"), 0);
    class_addmethod(snapshot_class, (t_method)snapshot_stop, gensym("stop"), 0);
    class_addmethod(snapshot_class, (t_method)snapshot_sampleinterval, gensym("sampleinterval"), A_FLOAT, 0);
    class_sethelpsymbol(snapshot_class, gensym("snapshot~"));
    pd_error(snapshot_class, "Cyclone: please use [cyclone/snapshot~] instead of [Snapshot~] to supress this error");
}
