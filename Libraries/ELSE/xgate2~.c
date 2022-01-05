
#include "m_pd.h"
#include <math.h>

#define HALF_PI (3.14159265358979323846 * 0.5)
#define MAXOUTPUT 500

typedef struct _xgate2{
    t_object    x_obj;
    t_float     *x_ch_select; // main signal (channel selector)
    int         x_n_outlets; // outlets excluding main signal
    int         x_indexed; // outlets excluding main signal
    t_float     **x_ovecs; // copying from matrix
    t_float     *x_ivec; // single pointer since (an array rather than an array of arrays)
}t_xgate2;

static t_class *xgate2_class;

static void xgate2_index(t_xgate2 *x, t_floatarg f)
{
    x->x_indexed = f != 0;
}

static t_int *xgate2_perform(t_int *w){
    t_xgate2 *x = (t_xgate2 *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *channel = x->x_ch_select;
    t_float **ovecs = x->x_ovecs;
    t_float *ivec = x->x_ivec;
    int n_outlets = x->x_n_outlets;
    int max_sel = x->x_n_outlets - 1;
    int indexed = x->x_indexed;
    int i, j;
    for(i = 0; i < nblock; i++){
        t_float input = ivec[i];
        t_float sel = channel[i];
        if(!indexed) // default is indexed
            sel = channel[i] * max_sel;
        if(sel <= 0){
            ovecs[0][i] = input;
            for(j = 1; j < n_outlets; j++)
                ovecs[j][i] = 0.0;
        }
        else if(sel >= max_sel){
            ovecs[max_sel][i] = input;
            for(j = 0; j < max_sel; j++)
                ovecs[j][i] = 0.0;
        }
        else{
            int ch = (int)sel;                          // selected output
            float fade = (sel - ch) * HALF_PI;          // fade point
            float fadeL = fabs(sin(fade - HALF_PI));    // cos fade value
            float fadeR = sin(fade);                    // sin fade value
            ovecs[ch][i] = input * fadeL; // cos
            ovecs[ch + 1][i] = input * fadeR; // sin
            for(j = 0; j < n_outlets; j++){
                if(j != ch && j != ch + 1)
                    ovecs[j][i] = 0.0;
            }
        }
    };
    return(w + 3);
}

static void xgate2_dsp(t_xgate2 *x, t_signal **sp){
    int i, nblock = sp[0]->s_n;
    t_signal **sigp = sp;
    x->x_ch_select = (*sigp++)->s_vec; // the idx inlet 
    x->x_ivec = (*sigp++)->s_vec; // the input inlet
    for(i = 0; i < x->x_n_outlets; i++) // the n_outlets
        *(x->x_ovecs+i) = (*sigp++)->s_vec;
    dsp_add(xgate2_perform, 2, x, nblock);
}

static void *xgate2_new(t_symbol *s, int argc, t_atom *argv){
    t_xgate2 *x = (t_xgate2 *)pd_new(xgate2_class);
    t_float n_outlets = 2; //inlets not counting xgate2 input
    x->x_indexed = 1;
    int i;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT) { //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    n_outlets = argval;
                case 1:
                    x->x_indexed = argval != 0;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else
            goto errstate;
    };
    if(n_outlets < 2)
        n_outlets = 2;
    else if(n_outlets > (t_float)MAXOUTPUT)
        n_outlets = MAXOUTPUT;
    x->x_n_outlets = (int)n_outlets;
    x->x_ovecs = getbytes(n_outlets * sizeof(*x->x_ovecs));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    for (i = 0; i < n_outlets; i++)
        outlet_new((t_object *)x, &s_signal);
    return(x);
    errstate:
        pd_error(x, "[xgate2~]: improper args");
        return NULL;
}

void * xgate2_free(t_xgate2 *x){
    freebytes(x->x_ovecs, x->x_n_outlets * sizeof(*x->x_ovecs));
    return (void *) x;
}

void xgate2_tilde_setup(void){
    xgate2_class = class_new(gensym("xgate2~"), (t_newmethod)xgate2_new,
                (t_method)xgate2_free, sizeof(t_xgate2), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(xgate2_class, nullfn, gensym("signal"), 0);
    class_addmethod(xgate2_class, (t_method)xgate2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(xgate2_class, (t_method)xgate2_index, gensym("index"), A_FLOAT, 0);
}
