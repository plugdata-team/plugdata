// Porres 2017
 
#include "m_pd.h"

static t_class *randi_class;

typedef struct _randi{
    t_object        x_obj;
    unsigned int    x_val;
    t_float         x_min;
    t_float         x_max;
} t_randi;

static void randi_seed(t_randi *x, t_float f){
    x->x_val = f;
}

static void randi_bang(t_randi *x){
    unsigned int val = x->x_val;
    t_int random;
    t_int out_low = (int)x->x_min; // Output LOW
    t_int out_high = (int)x->x_max + 1; // Output HIGH
    if(out_low > out_high){
        int temp = out_low;
        out_low = out_high;
        out_high = temp;
    }
    t_int range = out_high - out_low; // range
    if(range == 0)
        random = out_low;
    else{
        random = ((double)range) * ((double)val) * (1./4294967296.);
        if(random > range)
            random = range;
        random += out_low;
        x->x_val = val * 472940017 + 832416023;
    }
    outlet_float(x->x_obj.ob_outlet, random);
}

static void *randi_new(t_symbol *s, int argc, t_atom *argv){
    t_randi *x = (t_randi *) pd_new(randi_class);
    static unsigned int random_seed = 1489853723;
//////////////////////////////////////////////////////////
    x->x_min = 0.;
    x->x_max = 1.;
    if(argc == 1){
        if(argv -> a_type == A_FLOAT){
            x->x_min = 0;
            x->x_max = atom_getfloat(argv);
        }
    }
    else if(argc > 1 && argc <= 3){
        int numargs = 0;
        while(argc > 0){
            if(argv -> a_type == A_FLOAT){
                switch(numargs){
                    case 0: x->x_min = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_max = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 2: random_seed = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    default:
                        argc--;
                        argv++;
                        break;
                };
            }
            else
                goto errstate;
        };
    }
    else if(argc > 3)
        goto errstate;
////////////////////////////////////////////////////////////
    random_seed = random_seed * 435898247 + 938284287;
    x->x_val = random_seed & 0x7fffffff; // load seed value
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    outlet_new((t_object *)x, &s_float);
    return (x);
errstate:
    pd_error(x, "randi: improper args");
    return NULL;
}

void setup_rand0x2ei(void){
  randi_class = class_new(gensym("rand.i"), (t_newmethod)randi_new, 0, sizeof(t_randi), 0, A_GIMME, 0);
  class_addmethod(randi_class, (t_method)randi_seed, gensym("seed"), A_DEFFLOAT, 0);
  class_addbang(randi_class, (t_method)randi_bang);
}
