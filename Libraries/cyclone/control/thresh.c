/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */
/*
  2017 Derek Kwan - moved thresh_anything contents to thresh_list, no longer accept anything/bang/symbol
  but kept legacy code just incase 
*/

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define THRESH_INISIZE    32  /* LATER rethink */
#define THRESH_MAXSIZE   256
#define THRESH_DEFTHRESH  10

typedef struct _thresh
{
    t_object   x_ob;
    t_float    x_thresh;
    t_outlet  *x_outlet;
    int        x_size;    /* as allocated */
    int        x_natoms;  /* as used */
    t_atom    *x_message;
    t_atom     x_messini[THRESH_INISIZE];
    t_clock   *x_clock;
} t_thresh;

static t_class *thresh_class;

static void thresh_tick(t_thresh *x)
{
    int ac = x->x_natoms;
    if (ac)
    {
	t_atom *av = x->x_message;
	if (ac > 1)
        {
	  //if(av->a_type == A_FLOAT)
	        outlet_list(x->x_outlet, &s_list, ac, av);

	  /*
	  else if(av->a_type == A_SYMBOL)
                outlet_anything(x->x_outlet, av->a_w.w_symbol, ac-1, \
                        &av[1]);
	  */
        }
	else
        {
	    if(av->a_type == A_FLOAT)
                outlet_float(x->x_outlet, av->a_w.w_float);
	    /*
	    else if(av->a_type == A_SYMBOL)
                outlet_anything(x->x_outlet, av->a_w.w_symbol, 0, 0);
	    */
        };
	
	x->x_natoms = 0;
    }
}

static void thresh_list(t_thresh *x, t_symbol *s, int ac, t_atom *av)
{
    
    int ntotal = x->x_natoms + ac;
    t_atom *buf;
    clock_unset(x->x_clock);
    /*
    if (s == &s_) s = 0;
    if (s)
    {
    	ntotal++;
    };
    */
    if (ntotal > x->x_size)
    {
	/* LATER if (ntotal > THRESH_MAXSIZE)... (cf prepend) */
	int nrequested = ntotal;
	x->x_message = grow_withdata(&nrequested, &x->x_natoms,
				     &x->x_size, x->x_message,
				     THRESH_INISIZE, x->x_messini,
				     sizeof(*x->x_message));
	if (nrequested != ntotal)
	{
	    x->x_natoms = 0;
	    if (ac >= x->x_size)
	      //ac = (s ? x->x_size - 1 : x->x_size);
	      ac = x->x_size;
	}
    }
    buf = x->x_message + x->x_natoms;
    /*
    if (s)
    {
	SETSYMBOL(buf, s);
	buf++;
	x->x_natoms++;
    }
    */
    if (ac)
    {
	memcpy(buf, av, ac * sizeof(*buf));
	x->x_natoms += ac;
    }
    clock_delay(x->x_clock, x->x_thresh);


}

static void thresh_symbol(t_thresh *x, t_symbol *s)
{
  pd_error(x, "[thresh]: no method for 'symbol'");
  //thresh_anything(x, s, 0, 0);
}


static void thresh_anything(t_thresh *x, t_symbol *s, int ac, t_atom *av)
{
  pd_error(x, "[thresh]: no method for 'anything'");
}

static void thresh_float(t_thresh *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    thresh_list(x, 0, 1, &at);
}



static void thresh_ft1(t_thresh *x, t_floatarg f)
{
    if (f < 0)
	f = 0;  /* CHECKED */
    x->x_thresh = f;
    /* CHECKED: no rearming */
}

static void thresh_free(t_thresh *x)
{
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
    if (x->x_clock)
	clock_free(x->x_clock);
}

static void thresh_bang(t_thresh *x)
{
  pd_error(x, "[thresh]: no method for 'bang'");
  //outlet_bang(x->x_outlet);
}

static void *thresh_new(t_floatarg f)
{
    t_thresh *x = (t_thresh *)pd_new(thresh_class);
    x->x_thresh = (f > 0 ? f : THRESH_DEFTHRESH);
    x->x_size = THRESH_INISIZE;
    x->x_natoms = 0;
    x->x_message = x->x_messini;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    x->x_outlet = outlet_new((t_object *)x, &s_anything);  /* LATER rethink: list or float */
    x->x_clock = clock_new(x, (t_method)thresh_tick);
    return (x);
}

CYCLONE_OBJ_API void thresh_setup(void)
{
    thresh_class = class_new(gensym("thresh"),
			     (t_newmethod)thresh_new,
			     (t_method)thresh_free,
			     sizeof(t_thresh), 0,
			     A_DEFFLOAT, 0);
    class_addfloat(thresh_class, thresh_float);
    class_addlist(thresh_class, thresh_list);
    class_addsymbol(thresh_class, thresh_symbol);
    class_addanything(thresh_class, thresh_anything);
    class_addbang(thresh_class, thresh_bang);
    class_addmethod(thresh_class, (t_method)thresh_ft1,
		    gensym("ft1"), A_FLOAT, 0);
    /* CHECKED: thresh: doesn't understand bang, symbol, anything */
}
