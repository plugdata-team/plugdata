/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKME negative 'nbangs' value set during run-time */

#include "m_pd.h"
#include <common/api.h>

typedef struct _uzi
{
    t_object   x_obj;
    t_float    x_nbangs;
    t_float    x_offset;
    int        x_count;
    int        x_running;
    t_outlet  *x_out2;
    t_outlet  *x_out3;
} t_uzi;

static t_class *uzi_class;

#define UZI_RUNNING  1
#define UZI_PAUSED   2

static void uzi_dobang(t_uzi *x)
{
    /* CHECKME reentrancy */
    if (!x->x_running)
    {
    int count = (int)x->x_offset;
    int nbangs = (int)x->x_nbangs;
    int offset = (int)x->x_offset;
	x->x_running = UZI_RUNNING;
// If resuming/continuing, count restart the counter from where it stopped, even if the offset has changed
// something like count = last_count
	for (count = x->x_count; count < (offset + nbangs); count++)
	{
	    outlet_float(x->x_out3, count);
	    outlet_bang(((t_object *)x)->ob_outlet);
	    if (x->x_running == UZI_PAUSED)
	    {
		/* CHECKED: carry bang not sent, even if this is last bang */
		x->x_count = count + 1;
		return;
	    }
	}
	/* CHECKED: carry bang sent also when there are no left-outlet bangs */
	/* CHECKED: sent after left outlet, not before */
	outlet_bang(x->x_out2);
	x->x_count = offset;
	x->x_running = 0;
    }
}

static void uzi_bang(t_uzi *x)
{
    /* CHECKED: always restarts (when paused too) */
    x->x_count = (int)x->x_offset;
    x->x_running = 0;
    uzi_dobang(x);
}

static void uzi_float(t_uzi *x, t_float f)
{
    /* CHECKED: always sets a new value and restarts (when paused too) */
    x->x_nbangs = f;
    uzi_bang(x);
}

/* CHECKED: 'pause, resume' (but not just 'resume')
   sends a carry bang when not running (a bug?) */
static void uzi_pause(t_uzi *x)
{
    if (!x->x_running)
	x->x_count = (int)x->x_nbangs;  /* bug emulation? */
    x->x_running = UZI_PAUSED;
}

static void uzi_resume(t_uzi *x)
{
    if (x->x_running == UZI_PAUSED)
    {
	x->x_running = 0;
	uzi_dobang(x);
    }
}

static void uzi_offset(t_uzi *x, t_float f)
{
    x->x_offset = f;
}


static void *uzi_new(t_symbol *s, int ac, t_atom *av)
{
    t_uzi *x = (t_uzi *)pd_new(uzi_class);
    t_float f1 = 0, f2 = 1;
	switch(ac){
	default:
	case 2:
		f2=atom_getfloat(av+1);
	case 1:
		f1=atom_getfloat(av);
		break;
	case 0:
		break;
	}
    x->x_nbangs = (f1 >= 0. ? f1 : 0.);
    x->x_offset = f2;
    x->x_count = x->x_offset;
    x->x_running = 0;
    floatinlet_new((t_object *)x, &x->x_nbangs);
    outlet_new((t_object *)x, &s_bang);
    x->x_out2 = outlet_new((t_object *)x, &s_bang);
    x->x_out3 = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void uzi_setup(void)
{
    uzi_class = class_new(gensym("uzi"), (t_newmethod)uzi_new, 0,
        sizeof(t_uzi), 0, A_GIMME, 0);
    class_addbang(uzi_class, uzi_bang);
    class_addfloat(uzi_class, uzi_float);
    class_addmethod(uzi_class, (t_method)uzi_pause, gensym("pause"), 0);
    class_addmethod(uzi_class, (t_method)uzi_pause, gensym("break"), 0);
    class_addmethod(uzi_class, (t_method)uzi_resume, gensym("resume"), 0);
    class_addmethod(uzi_class, (t_method)uzi_resume, gensym("continue"), 0);
    class_addmethod(uzi_class, (t_method)uzi_offset, gensym("offset"), A_DEFFLOAT, 0);
}

CYCLONE_OBJ_API void Uzi_setup(void)
{
    uzi_class = class_new(gensym("Uzi"), (t_newmethod)uzi_new, 0,
        sizeof(t_uzi), 0, A_GIMME, 0);
    class_addbang(uzi_class, uzi_bang);
    class_addfloat(uzi_class, uzi_float);
    class_addmethod(uzi_class, (t_method)uzi_pause, gensym("pause"), 0);
    class_addmethod(uzi_class, (t_method)uzi_pause, gensym("break"), 0);
    class_addmethod(uzi_class, (t_method)uzi_resume, gensym("resume"), 0);
    class_addmethod(uzi_class, (t_method)uzi_resume, gensym("continue"), 0);
    class_addmethod(uzi_class, (t_method)uzi_offset, gensym("offset"), A_DEFFLOAT, 0);
    pd_error(uzi_class, "Cyclone: please use [uzi] instead of [Uzi] to suppress this error");
    class_sethelpsymbol(uzi_class, gensym("uzi"));
}
