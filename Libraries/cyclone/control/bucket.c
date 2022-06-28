/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a modified version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _bucket
{
    t_object    x_ob;
    int         x_numbucks;
    t_float    *x_bucks;   /* CHECKED: no limit */
    t_outlet  **x_outs;
    short int   x_frozen;  /* 0 for thawed, 1 for frozen */
    short int   x_dir;     /* 0 for L2R, 1 for R2L */
    short int   x_max5mode;  /* 0 for classic Max 4.6 mode, 
                              1 for Max 5 mode (with 2nd !0 argument) */
} t_bucket;

static t_class *bucket_class;

static void bucket_bang(t_bucket *x)
{
    int i = x->x_numbucks;
    /* CHECKED: outlets output in right-to-left order */
    while (i--) outlet_float(x->x_outs[i], x->x_bucks[i]);
}

static void bucket_float(t_bucket *x, t_float val)
{
    int i;

    if (!x->x_frozen)
	bucket_bang(x);
    if (!x->x_dir)
    {
	for (i = x->x_numbucks - 1; i > 0; i--)
	    x->x_bucks[i] = x->x_bucks[i - 1];
	x->x_bucks[0] = val;
    }
    else
    {
	for (i = 0; i < x->x_numbucks - 1; i++)
	    x->x_bucks[i] = x->x_bucks[i + 1];
	x->x_bucks[x->x_numbucks - 1] = val;
    }
    if (x->x_max5mode && !x->x_frozen)
	bucket_bang(x);
}

static void bucket_freeze(t_bucket *x)
{
    x->x_frozen = 1;
}

static void bucket_thaw(t_bucket *x)
{
    x->x_frozen = 0;
}

static void bucket_roll(t_bucket *x)
{
    if (x->x_dir)
	bucket_float(x, x->x_bucks[0]);
    else
	bucket_float(x, x->x_bucks[x->x_numbucks - 1]);
}

static void bucket_rtol(t_bucket *x)
{
    x->x_dir = 1;
}

static void bucket_ltor(t_bucket *x)
{
    x->x_dir = 0;
}

static void bucket_set(t_bucket *x, t_floatarg f)
{
    int i = x->x_numbucks;
    while (i--) x->x_bucks[i] = f;
    if (!x->x_frozen)  /* CHECKED */
	bucket_bang(x);
}

static void bucket_free(t_bucket *x)
{
    if (x->x_bucks)
	freebytes(x->x_bucks, x->x_numbucks * sizeof(*x->x_bucks));
    if (x->x_outs)
	freebytes(x->x_outs, x->x_numbucks * sizeof(*x->x_outs));
}

static void *bucket_new(t_floatarg val, t_floatarg max5mode)
{
    t_bucket *x;
    int nbucks = (int)val;
    t_float *bucks;
    t_outlet **outs;
    if (nbucks < 1)
	nbucks = 1;
    if (!(bucks = (t_float *)getbytes(nbucks * sizeof(*bucks))))
	return (0);
    if (!(outs = (t_outlet **)getbytes(nbucks * sizeof(*outs))))
    {
	freebytes(bucks, nbucks * sizeof(*bucks));
	return (0);
    }
    x = (t_bucket *)pd_new(bucket_class);
    x->x_numbucks = nbucks;
    x->x_bucks = bucks;
    x->x_outs = outs;
    x->x_frozen = 0;
    x->x_dir = 0;
    x->x_max5mode = ((int)max5mode != 0);
    while (nbucks--) *outs++ = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void bucket_setup(void)
{
    bucket_class = class_new(gensym("bucket"),
			     (t_newmethod)bucket_new,
			     (t_method)bucket_free,
			     sizeof(t_bucket), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(bucket_class, bucket_bang);
    class_addfloat(bucket_class, bucket_float);
    class_addmethod(bucket_class, (t_method)bucket_freeze, gensym("freeze"), 0);
    class_addmethod(bucket_class, (t_method)bucket_thaw, gensym("thaw"), 0);
    class_addmethod(bucket_class, (t_method)bucket_ltor, gensym("L2R"), 0);
    class_addmethod(bucket_class, (t_method)bucket_rtol, gensym("R2L"), 0);
    /* CHECKED (refman error) roll has no argument */
    class_addmethod(bucket_class, (t_method)bucket_roll, gensym("roll"), 0);
    /* 3.5 additions */
    class_addmethod(bucket_class, (t_method)bucket_set,
		    gensym("set"), A_FLOAT, 0);
    class_addmethod(bucket_class, (t_method)bucket_ltor, gensym("l2r"), 0);
    class_addmethod(bucket_class, (t_method)bucket_rtol, gensym("r2l"), 0);
}

CYCLONE_OBJ_API void Bucket_setup(void)
{
    bucket_class = class_new(gensym("Bucket"), (t_newmethod)bucket_new, (t_method)bucket_free,
        sizeof(t_bucket), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(bucket_class, bucket_bang);
    class_addfloat(bucket_class, bucket_float);
    class_addmethod(bucket_class, (t_method)bucket_freeze, gensym("freeze"), 0);
    class_addmethod(bucket_class, (t_method)bucket_thaw, gensym("thaw"), 0);
    class_addmethod(bucket_class, (t_method)bucket_ltor, gensym("L2R"), 0);
    class_addmethod(bucket_class, (t_method)bucket_rtol, gensym("R2L"), 0);
    class_addmethod(bucket_class, (t_method)bucket_roll, gensym("roll"), 0);
    class_addmethod(bucket_class, (t_method)bucket_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(bucket_class, (t_method)bucket_ltor, gensym("l2r"), 0);
    class_addmethod(bucket_class, (t_method)bucket_rtol, gensym("r2l"), 0);
    pd_error(bucket_class, "Cyclone: please use [bucket] instead of [Bucket] to suppress this error");
    class_sethelpsymbol(bucket_class, gensym("bucket"));
}
