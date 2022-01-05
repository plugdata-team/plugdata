// Porres 2018

#include "m_pd.h"
#include <string.h>

static t_class *coin_class;

typedef struct _coin{
    t_object    x_obj;
    t_int       x_val;
    t_float     x_n;
    t_float     x_out_of;
    t_outlet  *x_out2;
} t_coin;

static void coin_float(t_coin *x, t_float f){
    if(f > 0)
        x->x_out_of = f;
}

static void coin_seed(t_coin *x, t_float f){
    x->x_val = (int)f * 1319;
}

static void coin_bang(t_coin *x){
    int val = x->x_val;
    t_float random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    random = 100 * (random + 1) / 2; // rescale
    x->x_val = val * 435898247 + 382842987;
    t_float prob = x->x_n * 100/x->x_out_of;
    if(prob < 0){
        prob = 0;
    }
    if(prob > 100){
        prob = 100;
    }
    if(prob > 0){
        if(prob == 100)
            outlet_bang(x->x_obj.ob_outlet);
        else if(random < prob)
            outlet_bang(x->x_obj.ob_outlet);
        else
            outlet_bang(x->x_out2);
    }
    else
        outlet_bang(x->x_out2);
}

static void *coin_new(t_symbol *s, int ac, t_atom *av){
    t_symbol *sym = s;
    sym = NULL;
    t_coin *x = (t_coin *) pd_new(coin_class);
    static int init_seed = 54569;
    x->x_n = 50;
    x->x_out_of = 100;
    /////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *cursym = atom_getsymbolarg(0, ac, av);
            if(!strcmp(cursym->s_name, "-seed")){
                if(ac == 2 && (av+1)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(1, ac, av);
                    init_seed = curfloat;
                    ac -= 2;
                    av += 2;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else{
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_n = curf;
                    break;
                case 1:
                    if(curf > 0)
                        x->x_out_of = curf;
                    break;
                default:
                    break;
            };
        };
        argnum++;
        ac--;
        av++;
    };
///////////////////////////////////////////////
    x->x_val = init_seed *= 1319; // load seed value
    floatinlet_new((t_object *)x, &x->x_n);
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    return (x);
errstate:
    pd_error(x, "[coin]: improper args");
    return NULL;
}

void coin_setup(void){
    coin_class = class_new(gensym("coin"), (t_newmethod)coin_new, 0, sizeof(t_coin), 0, A_GIMME, 0);
    class_addmethod(coin_class, (t_method)coin_seed, gensym("seed"), A_DEFFLOAT, 0);
    class_addbang(coin_class, (t_method)coin_bang);
    class_addfloat(coin_class, coin_float);
}
