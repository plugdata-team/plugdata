// Porres 2017

#include <math.h>
#include "m_pd.h"

#define TWO_PI (3.14159265358979323846 * 2)

static t_class *fbsine2_class;

typedef struct _fbsine2{
    t_object  x_obj;
    double    x_im;
    double    x_fb;
    double    x_a;
    double    x_c;
    double    x_xn;
    double    x_yn;
    double    x_phase;
    t_float   x_sr;
    t_float   x_freq;
    t_outlet  *x_outlet;
}t_fbsine2;


static void fbsine2_clear(t_fbsine2 *x){
    x->x_xn = x->x_yn = 0;
}

static void fbsine2_coeffs(t_fbsine2 *x, t_symbol *s, int argc, t_atom * argv){
    if(argc){
        if(argc > 4){
            pd_error(x, "fbsine2~: 'coeffs' needs a maximum of 4 floats as arguments");
            return;
        }
        int argnum = 0; // current argument
        while(argc){
            if(argv -> a_type != A_FLOAT){
                pd_error(x, "fbsine2~: 'coeffs' arguments needs to only contain floats");
                return;
            }
            else{
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum){
                    case 0:
                    x->x_im = curf;
                    break;
                    case 1:
                    x->x_fb = curf;
                    break;
                    case 2:
                    x->x_a = curf;
                    break;
                    case 3:
                    x->x_c = curf;
                    break;
                };
                argnum++;
            };
            argc--;
            argv++;
        };
    }
}

static void fbsine2_list(t_fbsine2 *x, t_symbol *s, int argc, t_atom * argv){
    if(argc){
        if(argc > 2){
            pd_error(x, "fbsine2~: list size needs to be <= 2");
            return;
        }
        int argnum = 0; // current argument
        while(argc){
            if(argv -> a_type != A_FLOAT){
                pd_error(x, "fbsine2~: list needs to only contain floats");
                return;
                }
            else{
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum){
                    case 0:
                    x->x_xn = curf;
                    break;
                    case 1:
                    x->x_yn = curf;
                    break;
                };
                argnum++;
            };
            argc--;
            argv++;
        };
    }
}

static t_int *fbsine2_perform(t_int *w){
    t_fbsine2 *x = (t_fbsine2 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    double xn = x->x_xn;
    double yn = x->x_yn;
    double im = x->x_im; // overall phase index multuplier amount
    double a = x->x_a;   // phase multiplier
    double fb = x->x_fb; // feedback amount
    double c = x->x_c;   // phase step
    double phase = x->x_phase;
    double sr = x->x_sr;
    while(nblock--){
        t_float hz = *in++;
        double phase_step = (double)hz / (double)sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        int trig;
        if(hz >= 0.){
            trig = phase >= 1.;
            if(trig)
                phase -= 1.;
        }
        else{
            trig = (phase <= 0.);
            if(trig)
                phase += 1.;
        }
        if(trig){ // update
            xn = sin(im*yn + fb*xn);
            yn = fmod(a*yn + c, TWO_PI);
        }
        *out++ = xn;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_xn = xn;
    x->x_yn = yn;
    return(w + 5);
}

static void fbsine2_dsp(t_fbsine2 *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(fbsine2_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *fbsine2_free(t_fbsine2 *x){
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *fbsine2_new(t_symbol *sel, int ac, t_atom *av){
    t_fbsine2 *x = (t_fbsine2 *)pd_new(fbsine2_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5;
    double im = 1.;
    double fb = 0.1;
    double a = 1.1;
    double c = 0.5;
    double xn = 0.1;
    double yn = 0.1;
    int argnum = 0; // argument number
    while(ac){
            if(av -> a_type != A_FLOAT)
                goto errstate;
            else{
                t_float curf = atom_getfloatarg(0, ac, av);
                switch(argnum){
                    case 0:
                        hz = curf;
                        break;
                    case 1:
                        im = curf;
                        break;
                    case 2:
                        fb = curf;
                        break;
                    case 3:
                        a = curf;
                        break;
                    case 4:
                        c = curf;
                        break;
                    case 5:
                        xn = curf;
                        break;
                    case 6:
                        yn = curf;
                        break;
                };
                argnum++;
            };
            ac--;
            av++;
        };
    if(hz >= 0)
        x->x_phase = 1;
    x->x_freq  = hz;
    x->x_im = im;
    x->x_fb = fb;
    x->x_a = a;
    x->x_c = c;
    x->x_xn = xn;
    x->x_yn = yn;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "fbsine2~: arguments needs to only contain floats");
        return NULL;
}

void fbsine2_tilde_setup(void){
    fbsine2_class = class_new(gensym("fbsine2~"),
        (t_newmethod)fbsine2_new, (t_method)fbsine2_free,
        sizeof(t_fbsine2), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(fbsine2_class, t_fbsine2, x_freq);
    class_addmethod(fbsine2_class, (t_method)fbsine2_dsp, gensym("dsp"), A_CANT, 0);
    class_addlist(fbsine2_class, fbsine2_list);
    class_addmethod(fbsine2_class, (t_method)fbsine2_coeffs, gensym("coeffs"), A_GIMME, 0);
    class_addmethod(fbsine2_class, (t_method)fbsine2_clear, gensym("clear"), 0);
}
