
#include "m_pd.h"
#include <math.h>

#define HALF_PI (3.14159265358979323846 * 0.5)
#define MAXINTPUT 500

typedef struct _xselect2
{
    t_object   x_obj;
    t_float *x_ch_select; // main signal (channel selector)
    int 	  x_n_inlets; // inlets excluding main signal
    int 	  x_indexed; // inlets excluding main signal
    t_float  **x_ivecs; // copying from matrix
    t_float  *x_ovec; // single pointer since we're dealing with an array rather than an array of arrays
} t_xselect2;

static t_class *xselect2_class;

static void xselect2_index(t_xselect2 *x, t_floatarg f)
{
    x->x_indexed = f != 0;
}

static t_int *xselect2_perform(t_int *w)
{
    t_xselect2 *x = (t_xselect2 *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *channel = x->x_ch_select;
    t_float **ivecs = x->x_ivecs;
    t_float *ovec = x->x_ovec;
    t_float output;
    int max_sel = x->x_n_inlets - 1;
    int indexed = x->x_indexed;
    int i;
    for(i = 0; i < nblock; i++){
        float sel = channel[i];
        if(!indexed)
            sel = channel[i] * max_sel;
        if(sel <= 0)
            output = ivecs[0][i];
        else if(sel >= max_sel)
            output = ivecs[max_sel][i];
        else{
            int ch = (int)sel;
            float fade = (sel - ch) * HALF_PI;
            float fadeL = fabs(sin(fade - HALF_PI)); // cos
            float fadeR = sin(fade); // sin
            float left = ivecs[ch][i] * fadeL; // cos
            float right = ivecs[ch + 1][i] * fadeR; // sin
            output = left + right;
        }
        ovec[i] = output;
    };
    return (w + 3);
}

static void xselect2_dsp(t_xselect2 *x, t_signal **sp)
{
    int i, nblock = sp[0]->s_n;
    t_signal **sigp = sp;
    x->x_ch_select = (*sigp++)->s_vec; //the first sig in is the xselect2 idx
    for (i = 0; i < x->x_n_inlets; i++){ //now for the n_inlets
        *(x->x_ivecs+i) = (*sigp++)->s_vec;
    };
    x->x_ovec = (*sigp++)->s_vec; //now for the outlet
    dsp_add(xselect2_perform, 2, x, nblock);
}

static void *xselect2_new(t_symbol *s, int argc, t_atom *argv)
{
    t_xselect2 *x = (t_xselect2 *)pd_new(xselect2_class);
    t_float n_inlets = 2; //inlets not counting xselect2 input
    x->x_indexed = 1;
    int i;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT) { //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    n_inlets = argval;
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
    if(n_inlets < 2)
        n_inlets = 2;
    else if(n_inlets > (t_float)MAXINTPUT)
        n_inlets = MAXINTPUT;
    x->x_n_inlets = (int)n_inlets;
    x->x_ivecs = getbytes(n_inlets * sizeof(*x->x_ivecs));
    for (i = 0; i < n_inlets; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
    errstate:
        pd_error(x, "xselect2~: improper args");
        return NULL;
}

void * xselect2_free(t_xselect2 *x){
    freebytes(x->x_ivecs, x->x_n_inlets * sizeof(*x->x_ivecs));
    return (void *) x;
}

void xselect2_tilde_setup(void)
{
    xselect2_class = class_new(gensym("xselect2~"), (t_newmethod)xselect2_new,
                    (t_method)xselect2_free, sizeof(t_xselect2), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(xselect2_class, (t_method)xselect2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(xselect2_class, nullfn, gensym("signal"), 0);
    class_addmethod(xselect2_class, (t_method)xselect2_index, gensym("index"), A_FLOAT, 0);
}
