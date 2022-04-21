// porres 2017

#include "m_pd.h"
#include <string.h>

static t_class *break_class;

typedef struct _break{
    t_object    x_obj;
    t_symbol   *x_arg;
    int     	x_break;
    int         x_n;
}t_break;

static void break_float(t_break *x, t_float f){
    outlet_float(x->x_obj.ob_outlet, f);
}

static void break_symbol(t_break *x, t_symbol *s){
    if(x->x_break){
        if(strncmp(s->s_name, x->x_arg->s_name, x->x_n) == 0)
            outlet_anything(x->x_obj.ob_outlet, s, 0, 0);
        else
            outlet_symbol(x->x_obj.ob_outlet, s);
    }
    else
        outlet_symbol(x->x_obj.ob_outlet, s);
}

static void break_anything(t_break *x, t_symbol *s, int ac, t_atom *av){
    if(x->x_break){
        if(!ac)
            outlet_anything(x->x_obj.ob_outlet, s, ac, av);
        else{
            int i = -1, first = 1, n;
            while(i < ac){
// i is starting point & j is broken item
                int j = i + 1;
// j starts as next item from previous iteration (and as 0 in the first iteration)
                int z;
                for(z = j; z < ac; z++){
                    if((av+z)->a_type == A_SYMBOL)
                        if(!strncmp((atom_getsymbol(av+z))->s_name, x->x_arg->s_name, x->x_n))
                            break;
                }
                j = z;
// n is number of extra elements in the broken message (that's why we have - 1)
                n = j - i - 1;
                if(first){
                    if(n == 0) // it's a selector
                        if(s == gensym("list")){ // if selector is list, do nothing
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

static void *break_new(t_symbol *s){
    t_break *x = (t_break *)pd_new(break_class);
    if(s !=  &s_){
        x->x_arg = s;
        x->x_n = strlen(s->s_name);
        x->x_break = 1;
    }
    else
        x->x_break = 0;
    outlet_new(&x->x_obj, &s_anything);
    return(x);
}

void break_setup(void){
    break_class = class_new(gensym("break"), (t_newmethod)break_new, 0,
        sizeof(t_break), 0, A_DEFSYMBOL, 0);
    class_addanything(break_class, break_anything);
    class_addsymbol(break_class, break_symbol);
    class_addfloat(break_class, break_float);
}
