// porres 2019

#include <m_pd.h>
#include <math.h>

typedef struct{
	t_object    x_obj;
    float       x_v;
}t_factor;

static t_class *factor_class;

void factor_float(t_factor *x, t_floatarg f){
    int n = (int)f;
    if(n <= 0 || n > 16777216){
        pd_error(x, "[factor]: number %d out of range (1 - 16777216)", n);
        return;
    }
    if(n == 1){
        outlet_float(x->x_obj.te_outlet, n);
        return;
    }
    int ac = 0;
    t_atom av[24];
    int p = 2;
    while(n > 1){
        if(n % p == 0){
            SETFLOAT(av+ac, p);
            ac++;
            n /= p;
        }
        else
            p++;
        if(ac >= 24)
            break; // safeguard
    }
    outlet_list(x->x_obj.te_outlet, gensym("list"), ac, av);
}

void *factor_new(void){
	t_factor *x = (t_factor *)pd_new(factor_class);
	outlet_new((t_object *)x, 0);
	return(x);
}

void factor_setup(void){
	factor_class = class_new(gensym("factor"), (t_newmethod)factor_new,
                    0, sizeof(t_factor), 0, 0);
	class_addfloat(factor_class, factor_float);
}
