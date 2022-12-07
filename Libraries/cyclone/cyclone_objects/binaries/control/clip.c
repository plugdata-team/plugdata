/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define CLIP_INISIZE   32  /* LATER rethink */
#define CLIP_MAXSIZE  256

typedef struct _clip
{
    t_object   x_ob;
    float      x_f1;
    float      x_f2;
    int        x_size;  /* as allocated */
    t_atom    *x_message;
    t_atom     x_messini[CLIP_INISIZE];
    int        x_entered;
} t_clip;

static t_class *clip_class;

/* CHECKED case of f1 > f2:  x <= f2 => f1, x > f2 => f2
   (Pd implementation of clip has it the other way around) */
static void clip_float(t_clip *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet,
		 (f > x->x_f2 ? x->x_f2 : (f < x->x_f1 ? x->x_f1 : f)));
}

static void clip_list(t_clip *x, t_symbol *s, int ac, t_atom *av)
{
    if (ac)
    {
	int docopy = 0;
	int i;
	t_atom *ap;
	t_float f1 = x->x_f1;
	t_float f2 = x->x_f2;
	for (i = 0, ap = av; i < ac; i++, ap++)
	{
	    t_float f;
	    if (ap->a_type == A_FLOAT)
		f = ap->a_w.w_float;
	    else
	    {
		docopy = 1;
		/* CHECKED: symbols inside lists are converted to zeros */
		f = 0;
	    }
	    if (f < f1 || f > f2) docopy = 1;
	}
	if (docopy)
	{
	    t_atom *buf;
	    t_atom *bp;
	    int reentered = x->x_entered;
	    int prealloc = !reentered;
	    x->x_entered = 1;
	    if (prealloc && ac > x->x_size)
	    {
		if (ac > CLIP_MAXSIZE)
		    prealloc = 0;
		else
		    x->x_message = grow_nodata(&ac, &x->x_size, x->x_message,
					       CLIP_INISIZE, x->x_messini,
					       sizeof(*x->x_message));
	    }
	    if (prealloc) buf = x->x_message;
	    else
		/* LATER consider using the stack if ntotal <= MAXSTACK */
		buf = getbytes(ac * sizeof(*buf));
	    if (buf)
	    {
		for (i = 0, ap = av, bp = buf; i < ac; i++, ap++, bp++)
		{
		    t_float f = (ap->a_type == A_FLOAT ? ap->a_w.w_float : 0);
		    if (f < f1)
			f = f1;
		    else if (f > f2)
			f = f2;
		    SETFLOAT(bp, f);
		}
		outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, buf);
		if (buf != x->x_message)
		    freebytes(buf, ac * sizeof(*buf));
	    }
	    if (!reentered)
	    {
		x->x_entered = 0;
	    }
	}
	else outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
    }
}

static void clip_set(t_clip *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_f1 = 0;
    x->x_f2 = 0;
    if (ac)  /* CHECKED: 'set' without arguments sets both values to 0 */
    {
	if (av->a_type == A_FLOAT)  /* CHECKED: symbol is converted to 0 */
	    x->x_f1 = av->a_w.w_float;
	av++;
	if (--ac)
	{
	    if (av->a_type == A_FLOAT)
		x->x_f2 = av->a_w.w_float;
	}
	else x->x_f2 = x->x_f1;  /* CHECKED */
    }
}

static void clip_free(t_clip *x)
{
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
}

static void *clip_new(t_symbol *s, int ac, t_atom *av)
{
    t_clip *x = (t_clip *)pd_new(clip_class);
    x->x_f1 = 0;
    x->x_f2 = 0;
    x->x_size = CLIP_INISIZE;
    x->x_message = x->x_messini;
    x->x_entered = 0;
    floatinlet_new((t_object *)x, &x->x_f1);
    floatinlet_new((t_object *)x, &x->x_f2);
    outlet_new(&x->x_ob, &s_anything);
    clip_set(x, 0, ac, av);
    return (x);
}

CYCLONE_OBJ_API void clip_setup(void)
{
    clip_class = class_new(gensym("cyclone/clip"), (t_newmethod)clip_new,
        (t_method)clip_free, sizeof(t_clip), 0, A_GIMME, 0);
    class_addfloat(clip_class, clip_float);
    class_addlist(clip_class, clip_list);
    class_addmethod(clip_class, (t_method)clip_set, gensym("set"), A_GIMME, 0);
    class_sethelpsymbol(clip_class, gensym("clip"));
}

CYCLONE_OBJ_API void Clip_setup(void)
{
    clip_class = class_new(gensym("Clip"), (t_newmethod)clip_new,
        (t_method)clip_free, sizeof(t_clip), 0, A_GIMME, 0);
    class_addfloat(clip_class, clip_float);
    class_addlist(clip_class, clip_list);
    class_addmethod(clip_class, (t_method)clip_set, gensym("set"), A_GIMME, 0);
    class_sethelpsymbol(clip_class, gensym("clip"));
    pd_error(clip_class, "Cyclone: please use [cyclone/clip] instead of [Clip] to suppress this error");
}
