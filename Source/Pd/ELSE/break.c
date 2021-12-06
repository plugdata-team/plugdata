// porres 2017

#include "m_pd.h"
#include <string.h>

static t_class *break_class;

typedef struct _break{
  t_object     	x_obj;
  int 			x_break;
  char			x_separator;
}t_break;

static void break_float(t_break *x, t_float f){
    outlet_float(x->x_obj.ob_outlet, f);
}

static void break_symbol(t_break *x, t_symbol *s){
    if(x->x_break){
        if(x->x_separator == s->s_name[0])
            outlet_anything(x->x_obj.ob_outlet, s, 0, s);
        else
            outlet_symbol(x->x_obj.ob_outlet, s);
    }
    else
        outlet_symbol(x->x_obj.ob_outlet, s);
}

static void break_anything(t_break *x, t_symbol *s, int ac, t_atom *av){
    if (x->x_break){
        if(!ac)
            outlet_anything(x->x_obj.ob_outlet, s, ac, av);
        else{
            int i = -1, first = 1, n;
            while(i < ac){
// i is starting point & j is broken item
                int j = i + 1;
// j starts as next item from previous iteration (and as 0 in the first iteration)
                for (j; j < ac; j++){
                    if ((av+j)->a_type == A_SYMBOL && x->x_separator == (atom_getsymbol(av+j))->s_name[0]){
                        break;
                    }
                }
// n is number of extra elements in the broken message (that's why we have - 1)
                n = j - i - 1;
                if(first){
                    if(n == 0) // it's a selector
                        if(!strcmp(s->s_name, "list")){ // if selector is list, do nothing
                        }
                        else
                            outlet_anything(x->x_obj.ob_outlet, s, n, av - 1); // output selector
                    else
                        outlet_anything(x->x_obj.ob_outlet, s, n, av);
                    first = 0;
                    }
                else
                    outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(av + i), n, av + i + 1);
                i = j;
            }
        }
    }
    else
        outlet_anything(x->x_obj.ob_outlet, s, ac, av);
}

static void *break_new(t_symbol *selector, int ac, t_atom* av){
  t_break *x = (t_break *)pd_new(break_class);
  if(ac && av->a_type == A_SYMBOL){
      x->x_separator = atom_getsymbol(av)->s_name[0];
      x->x_break = 1;
  }
  outlet_new(&x->x_obj, &s_anything);
  return(x);
}

void break_setup(void){
  break_class = class_new(gensym("break"), (t_newmethod)break_new, 0, sizeof(t_break), 0, A_GIMME, 0);
    class_addanything(break_class, break_anything);
    class_addsymbol(break_class, break_symbol);
    class_addfloat(break_class, break_float);
}
