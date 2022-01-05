#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

#define HALF_PI (3.14159265358979323846 * 0.5)

static t_class *rotate_class;

typedef struct _rotate{
    t_object    x_obj;
    t_int       x_ch;
    t_float    *x_inbuf;
    t_float   **x_ins;      // array of input signal vectors
    t_float   **x_outs;     // array of output signal vectors
    t_inlet    *x_inlet_pos;
}t_rotate;

static t_int *rotate_perform(t_int *w){
    t_rotate *x = (t_rotate*) w[1];
    t_float *invec;
	t_int x_ch = x->x_ch;
    t_float *inarr = x->x_inbuf;
    t_float **x_ins = x->x_ins;
    t_float **x_outs = x->x_outs;
    double amp1, amp2;
    double pos;
    double idx;
	int ch, i, j;
    int offset;
    int n = (int) w[(x_ch * 2) + 3];
    for(i = 0; i < x_ch + 1; i++){ // copy input vectors
        invec = (t_float *) w[2+i];
        for(j = 0; j < n; j++)
            x_ins[i][j] = invec[j];
    }
    for(i = 0; i < x_ch; i++) // assign output vector pointers
        x_outs[i] = (t_float *) w[3 + x_ch + i];
	for(j = 0; j < n; j++){
        for(ch = 0; ch < x_ch; ch++){
            inarr[ch] = x_ins[ch][j];
            x_outs[ch][j] = 0;
        }
        idx = x_ins[x_ch][j] * (double)x_ch; // pos inlet
        if(idx <= -x_ch || idx >= x_ch)
            idx = 0;
        while(idx < 0)
            idx += x_ch;
        offset = (int)floor(idx) % x_ch;
        pos = (idx - offset) * HALF_PI;
        amp1 = cos(pos);
        amp2 = sin(pos);
        for(ch = 0; ch < x_ch; ch++){
            x_outs[(ch+offset) % x_ch][j] += amp1 * inarr[ch];
            x_outs[(ch+offset+1) % x_ch][j] += amp2 * inarr[ch];
        }
	}
    return(w+(x_ch*2)+4);
}

static void rotate_dsp(t_rotate *x, t_signal **sp){
    t_int i;
    t_int **sigvec;
    int pointer_count = (x->x_ch * 2) + 3; // I/O chs + obj + panner + blocksize
    sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));
    for(i = 0; i < pointer_count; i++)
        sigvec[i] = (t_int *) calloc(sizeof(t_int), 1);
    sigvec[0] = (t_int *)x; // object
    sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // block size
    for(i = 1; i < pointer_count - 1; i++) // inlet and outlets
        sigvec[i] = (t_int *)sp[i-1]->s_vec;
    dsp_addv(rotate_perform, pointer_count, (t_int *)sigvec);
    free(sigvec);
}

static void rotate_free(t_rotate *x){
    for(int i = 0; i < x->x_ch + 1; i++)
        free(x->x_ins[i]);
    free(x->x_ins);
    free(x->x_outs);
    free(x->x_inbuf);
}

static void *rotate_new(t_floatarg f1, t_floatarg f2){
    t_rotate *x = (t_rotate *)pd_new(rotate_class);
    t_int ch = (t_int)f1;
    if(ch < 2)
        ch = 2;
    x->x_ch = ch;
    float pos = f2;
// allocate in chs plus 1 for controlling the pan
    int i;
    for(i = 1; i < x->x_ch; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
    x->x_inlet_pos = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_pos, pos);
    for(i = 0; i < x->x_ch; i++)
        outlet_new(&x->x_obj, gensym("signal"));
    x->x_inbuf = (t_float *) malloc((x->x_ch + 1) * sizeof(t_float));
    x->x_ins = (t_float **) malloc((x->x_ch + 1) * sizeof(t_float *));
    x->x_outs = (t_float **) malloc(x->x_ch * sizeof(t_float *));
    for(i = 0; i < x->x_ch + 1; i++)
        x->x_ins[i] = (t_float *) malloc(8192 * sizeof(t_float));
    return(x);
}

void rotate_tilde_setup(void){
    rotate_class = class_new(gensym("rotate~"), (t_newmethod)rotate_new,
            (t_method)rotate_free, sizeof(t_rotate), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(rotate_class, nullfn, gensym("signal"), 0);
    class_addmethod(rotate_class, (t_method)rotate_dsp, gensym("dsp"), A_CANT, 0);
}
