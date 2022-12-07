/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//updating argument parsing on object creation and adding autoreset attribute - Derek Kwan 2016
#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define COUNTMAXINT 0x7fffffff

#define PDCYCMINV 0.
#define PDCYCMAXV 0.
#define PDCYCONFL 0
#define PDCYCAUTO 0 

typedef struct _count
{
    t_object x_obj;
    t_float  x_lastin;
    int    x_min;
    int    x_max;
    int    x_limit;
    int    x_on;
    int    x_sig;
    int    x_autoreset;
    int    x_count;  /* MAYBE use 64 bits (if 13.5 hours is not enough...) */
} t_count;

static t_class *count_class;

static void count_bang(t_count *x)
{
    x->x_count = x->x_min;
    x->x_on = 1;
}

static void count_min(t_count *x, t_floatarg f)
{
    x->x_min = (int)f;
}

static void count_max(t_count *x, t_floatarg f)
{
    x->x_max = (int)f;
    /* MAYBE use 64 bits */
    x->x_limit = (x->x_max == 0 ? COUNTMAXINT
		  : x->x_max - 1);  /* CHECKED */
}

static void count_autoreset(t_count *x, t_floatarg f)
{
    x->x_autoreset = (f != 0);
}

static void count_float(t_count *x, t_floatarg f)
{
    x->x_count = x->x_min = (int)f;
    x->x_on = 1;
}

static void count_list(t_count *x, t_symbol *s, int ac, t_atom *av)
{
    int i;
    if (ac > 4) ac = 4;
    for (i = 0; i < ac; i++)
	if (av[i].a_type != A_FLOAT) break;
    switch (i)
    {
    case 4:
	count_autoreset(x, av[3].a_w.w_float);
    case 3:
	x->x_on = (av[2].a_w.w_float != 0);
    case 2:
	count_max(x, av[1].a_w.w_float);
    case 1:
	count_min(x, av[0].a_w.w_float);
    default:
	x->x_count = x->x_min;
    }
}

/* changes minimum (and optional the maximum) without changing current value */
static void count_set(t_count *x, t_symbol *s, int ac, t_atom *av)
{
    if (ac > 0)
    {
	if (av[0].a_type == A_FLOAT) {
	    x->x_min = (int)av[0].a_w.w_float;
        {
            x->x_max = (int)0;
            x->x_limit = (x->x_max == 0 ? COUNTMAXINT : x->x_max - 1);
        }
	}
    }
    if (ac > 1)
    {
	if (av[1].a_type == A_FLOAT) 
	{
	    x->x_max = (int)av[1].a_w.w_float;
	    x->x_limit = (x->x_max == 0 ? COUNTMAXINT
		: x->x_max - 1);
	}
    } 
}

static void count_stop(t_count *x)
{
    if (x->x_sig) {
        x->x_count = x->x_min;
//        x->x_on = 1;
    }
    else {
    x->x_count = x->x_min;
    x->x_on = 0;
    }
}

static t_int *count_perform(t_int *w)
{
    t_count *x = (t_count *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *input = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float lastin = x->x_lastin;
    int count = x->x_count;
    int limit = x->x_limit;
    while (nblock--)
    {
    float in = *input++;
// Updating to Max 5 functionality of signal input
// (sample accurate) triggering
    if (in != 0 && lastin == 0) // turn it on
        {
         count = x->x_min;
         x->x_on = 1;
         x->x_sig = 1;
         }
    else if (in == 0 && lastin != 0) // turn it off
        {
        count = x->x_min;
        x->x_on = 0;
        x->x_sig = 0;
        }
    lastin = in; // audio triggering is working!
        if (x->x_on)
        {
            {
            if (count > limit) count = x->x_min;
            *out++ = (t_float)count++;
            }
        }
        else *out++ = count;
    }
    x->x_lastin = lastin;
    x->x_count = count;
    return (w + 5);
}

static void count_dsp(t_count *x, t_signal **sp)
{
    if (x->x_autoreset) count_bang(x);
    dsp_add(count_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *count_new(t_symbol *s, int argc, t_atom *argv)
{
    t_count *x = (t_count *)pd_new(count_class);
    x->x_lastin = 0;
	t_float minval, maxval, onflag, autoflag;
	minval = PDCYCMINV;
	maxval = PDCYCMAXV;
	onflag = PDCYCONFL;
	autoflag = PDCYCAUTO;
	int argnum = 0;
	while(argc > 0){
		if(argv -> a_type == A_FLOAT){
			t_float argval = atom_getfloatarg(0, argc, argv);
			switch(argnum){
				case 0:
					minval = argval;
					break;
				case 1:
					maxval = argval;
					break;
				case 2:
					onflag = argval;
					break;
				case 3:
					autoflag = argval;
					break;
				default:
					break;
				};
			argnum++;
			argc--;
			argv++;
			}
		else if(argv -> a_type == A_SYMBOL){
			t_symbol * curarg = atom_getsymbolarg(0, argc, argv);
			if(strcmp(curarg->s_name, "@autoreset") == 0){
				if(argc >= 2){
					t_float argval = atom_getfloatarg(1, argc, argv);
					autoflag = argval;
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
    count_min(x, minval);
    count_max(x, maxval);
    x->x_on = (onflag != 0);
    count_autoreset(x, autoflag);
    x->x_count = x->x_min;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,&s_float, gensym("ft1"));
    outlet_new(&x->x_obj, &s_signal);
    return (x);
    
    errstate:
    pd_error(x, "count~: improper args");
    return NULL;
}

CYCLONE_OBJ_API void count_tilde_setup(void)
{
    count_class = class_new(gensym("count~"), (t_newmethod)count_new,
        0, sizeof(t_count), 0, A_GIMME, 0);
	class_domainsignalin(count_class, -1);
	class_addfloat(count_class, (t_method)count_float);
    class_addlist(count_class, (t_method)count_list);
    class_addmethod(count_class, (t_method)count_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(count_class, (t_method)count_bang);
    class_addmethod(count_class, (t_method)count_max, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(count_class, (t_method)count_autoreset, gensym("autoreset"), A_FLOAT, 0);
    class_addmethod(count_class, (t_method)count_min, gensym("min"), A_FLOAT, 0);
    class_addmethod(count_class, (t_method)count_set, gensym("set"), A_GIMME, 0);
    class_addmethod(count_class, (t_method)count_stop, gensym("stop"), 0);
}
