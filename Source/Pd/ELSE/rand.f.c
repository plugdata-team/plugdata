// Porres 2017
 
#include "m_pd.h"

static t_class *randf_class;

typedef struct _randf{
    t_object    x_obj;
    t_int       x_val;
    t_float     x_min;
    t_float     x_max;
} t_randf;

static void randf_seed(t_randf *x, t_float f){
    x->x_val = (int)f * 1319;
}

static void randf_bang(t_randf *x){
    int val = x->x_val;
    t_float random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    random = x->x_min + (x->x_max - x->x_min) * (random + 1) / 2; // rescale
    x->x_val = val * 435898247 + 382842987;
    outlet_float(x->x_obj.ob_outlet, random);
}

static void *randf_new(t_symbol *s, int argc, t_atom *argv){
    t_randf *x = (t_randf *) pd_new(randf_class);
    static int init_seed = 54569;
/////////////////////////////////////////////
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
                    case 2: init_seed = atom_getfloatarg(0, argc, argv);
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
///////////////////////////////////////////////
    x->x_val = init_seed *= 1319; // load seed value
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    outlet_new((t_object *)x, &s_float);
    return (x);
errstate:
    pd_error(x, "randf: improper args");
    return NULL;
}

void setup_rand0x2ef(void){
  randf_class = class_new(gensym("rand.f"), (t_newmethod)randf_new, 0, sizeof(t_randf), 0, A_GIMME, 0);
  class_addmethod(randf_class, (t_method)randf_seed, gensym("seed"), A_DEFFLOAT, 0);
  class_addbang(randf_class, (t_method)randf_bang);
}
