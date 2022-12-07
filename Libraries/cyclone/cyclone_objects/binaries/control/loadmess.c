// modified from [init]

/* Copyright (c) 2000-2006 Thomas Musil @ IEM KUG Graz Austria
   Copyright (c) 2016      Joel Matthys
   Copyright (c) 2016      Marco Matteo Markidis
   */

// may have issues with LB_LOAD defined in g_canvas.h, sub with 0 if necessary (but ill-advised) - DK

#include <string.h>

#include "m_pd.h"
#include <common/api.h>
#include "g_canvas.h"

#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)
#define IS_A_DOLLAR(atom,index) ((atom+index)->a_type == A_DOLLAR)
#define IS_A_DOLLSYM(atom,index) ((atom+index)->a_type == A_DOLLSYM)
#define IS_A_SEMI(atom,index) ((atom+index)->a_type == A_SEMI)
#define IS_A_COMMA(atom,index) ((atom+index)->a_type == A_COMMA)

#ifdef MSW
int sys_noloadbang;
#else
extern int sys_noloadbang;
#endif

static t_class *loadmess_class;

typedef struct _loadmess
{
  t_object     x_obj;
  t_int        x_n;
  t_int        x_ac;
  t_atom       *x_at;
  t_symbol     *x_sym;
  t_atomtype   x_type;
  t_canvas     *x_canvas;
  t_int        defer;
  t_clock      *x_clock;
} t_loadmess;

static void loadmess_bang(t_loadmess *x)
{
  if(x->x_type == A_FLOAT)
    outlet_float(x->x_obj.ob_outlet, atom_getfloat(x->x_at));
  else if(x->x_type == A_SYMBOL)
    outlet_symbol(x->x_obj.ob_outlet, atom_getsymbol(x->x_at));
  else if(x->x_type == A_NULL)
    outlet_bang(x->x_obj.ob_outlet);
  else if(x->x_type == A_COMMA)
    outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac, x->x_at);
  else if(x->x_type == A_GIMME)
    outlet_list(x->x_obj.ob_outlet, &s_list, x->x_ac, x->x_at);
  else if(x->x_type == A_POINTER)
    outlet_pointer(x->x_obj.ob_outlet, (t_gpointer *)x->x_at->a_w.w_gpointer);
}

static void loadmess_loadbang(t_loadmess *x, t_floatarg action)
{
  
  if(!sys_noloadbang && action == LB_LOAD) {
    if(x->defer==-1)
	{
      loadmess_bang(x);
    }
    else clock_delay(x->x_clock, x->defer);
  }
  else clock_unset(x->x_clock);
}

static void loadmess_defer(t_loadmess *x)
{
  loadmess_bang(x);
}

static void loadmess_set(t_loadmess *x, t_symbol *s, int ac, t_atom *av)
{
  t_int i;
  x->x_type = A_NULL;
  if(ac==0)
    {
      x->x_type = A_NULL;
      x->x_sym = &s_bang;
      x->x_n = 1;
      x->x_ac = 0;
      x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
    }
  else if(ac == 1)
    {
      if(IS_A_SYMBOL(av,0))
	{
	  x->x_type = A_COMMA;
	  x->x_sym = atom_getsymbol(av);
	  x->x_n = 1;
	  x->x_ac = 0;
	  x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
	}
      else
	{
	  if(IS_A_FLOAT(av,0))
	    {
	      x->x_type = A_FLOAT;
	      x->x_sym = &s_float;
	    }
	  else if(IS_A_POINTER(av,0))
	    {
	      x->x_type = A_POINTER;
	      x->x_sym = &s_pointer;
	    }
	  x->x_n = x->x_ac = 1;
	  x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
	  x->x_at[0] = *av;
	}
    }
  else
    {
      x->x_type = A_COMMA; // outlet_anything
      if(IS_A_SYMBOL(av,0)) {
      	x->x_sym = atom_getsymbol(av++);
      	ac--;
      }
      else
      	{
	  x->x_sym = &s_list;
	}
      x->x_n = x->x_ac = ac;
      x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
      for(i=0;i<ac;i++)
	x->x_at[i] = av[i];
    }

}

static void loadmess_click(t_loadmess *x, t_floatarg xpos, t_floatarg ypos,
			   t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
  loadmess_bang(x);
}

static void loadmess_free(t_loadmess *x)
{
  if(x->x_at)
    freebytes(x->x_at, x->x_n * sizeof(t_atom));
  clock_free(x->x_clock);
}

static void *loadmess_new(t_symbol *s, int ac, t_atom *av)
{
  t_loadmess *x = (t_loadmess *)pd_new(loadmess_class);
  int i;
  x->defer = -1;
  
  x->x_type = A_NULL;
  if(ac==0)
    {
      x->x_type = A_NULL;
      x->x_sym = &s_bang;
      x->x_n = 1;
      x->x_ac = 0;
      x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
    }
  else if(ac == 1)
    {
      if(IS_A_SYMBOL(av,0))
	{
	  x->x_type = A_COMMA;
	  x->x_sym = atom_getsymbol(av);
	  x->x_n = 1;
	  x->x_ac = 0;
	  x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
	}
      else
	{
	  if(IS_A_FLOAT(av,0))
	    {
	      x->x_type = A_FLOAT;
	      x->x_sym = &s_float;
	    }
	  else if(IS_A_POINTER(av,0))
	    {
	      x->x_type = A_POINTER;
	      x->x_sym = &s_pointer;
	    }
	  x->x_n = x->x_ac = 1;
	  x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
	  x->x_at[0] = *av;
	}
    }
  else
    {
      x->x_type = A_COMMA;/*outlet_anything*/
      if(IS_A_SYMBOL(av,0)) {
      	x->x_sym = atom_getsymbol(av++);
      	ac--;
      }
      else
      	{
	  x->x_sym = &s_list;
	}
      x->x_n = x->x_ac = ac;
      x->x_at = (t_atom *)getbytes(x->x_n * sizeof(t_atom));
      for(i=0;i<ac;i++) {
	t_symbol *firstarg = atom_getsymbolarg(i,ac,av);
	if(strcmp(firstarg->s_name,"@defer")!=0) {
	  x->x_at[i] = av[i];
	}
	else {
	  t_int defer = atom_getintarg(i+1,ac,av);
	  switch (defer) {
	  case 0:
	    break;
	  case 1:
	    x->defer = 0;
	    break;
	  default:
	    pd_error(x,"Defer attribute must be 0 or 1");
	    x->defer = -1;
	    break;
	  }
	  x->x_n = x->x_ac = ac-2;
	  x->x_at = (t_atom *)t_resizebytes(x->x_at,sizeof(t_atom)*(x->x_n+2),sizeof(t_atom)*x->x_n);
	  i = ac;
	}
      }
    }
  outlet_new(&x->x_obj, &s_list);
  x->x_canvas = canvas_getcurrent();
  x->x_clock = clock_new(x,(t_method)loadmess_defer);
  return (x);
}

CYCLONE_OBJ_API void loadmess_setup(void)
{
  loadmess_class = class_new(gensym("loadmess"), (t_newmethod)loadmess_new,
			     (t_method)loadmess_free, sizeof(t_loadmess), 0, A_GIMME, 0);
  class_addmethod(loadmess_class, (t_method)loadmess_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
  class_addmethod(loadmess_class, (t_method)loadmess_set, gensym("set"),A_GIMME,0);
  class_addbang(loadmess_class, (t_method)loadmess_bang);
  class_addmethod(loadmess_class, (t_method)loadmess_click, gensym("click"),
		  A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,0);
}
