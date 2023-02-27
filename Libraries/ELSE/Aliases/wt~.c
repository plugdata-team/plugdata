// Porres 2018-2023

#include "m_pd.h"
#include "magic.h"
#include "buffer.h"

static t_class *wt_class;

typedef struct _wt{
    t_object  x_obj;
    t_buffer *x_buffer;
    double    x_phase;
    double    x_last_phase_offset;
    t_float   x_freq;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_int midi;
    t_int soft;
    t_float   x_sr;
    t_float   x_offset;
    t_float   x_size;
    t_int     x_interp;
// MAGIC:
    t_glist  *x_glist;              // object list
    t_float  *x_signalscalar;       // right inlet's float field
    int       x_hasfeeders;         // right inlet connection flag
    t_float   x_phase_sync_float;   // float from magic
}t_wt;

#define INDEX_2PT() \
    double xpos = phase*(double)size; \
    int ndx = (int)xpos; \
    double frac = xpos - ndx; \
    if(ndx == size) ndx = 0; \
    int ndx1 = ndx + 1; \
    if(ndx1 == size) ndx1 = 0; \
    double b = (double)vector[ndx + offset].w_float; \
    double c = (double)vector[ndx1 + offset].w_float;

#define INDEX_4PT() \
    double xpos = phase*(double)size; \
    int ndx = (int)xpos; \
    double frac = xpos - ndx; \
    if(ndx == size) ndx = 0; \
    int ndxm1 = ndx - 1; \
    if(ndxm1 < 0) ndxm1 = size - 1; \
    int ndx1 = ndx + 1; \
    if(ndx1 == size) ndx1 = 0; \
    int ndx2 = ndx1 + 1; \
    if(ndx2 == size) ndx2 = 0; \
    double a = (double)vector[ndxm1 + offset].w_float; \
    double b = (double)vector[ndx + offset].w_float; \
    double c = (double)vector[ndx1 + offset].w_float; \
    double d = (double)vector[ndx2 + offset].w_float;

static void wt_offset(t_wt *x, t_float f){
    x->x_offset = f;
}

static void wt_size(t_wt *x, t_float f){
    x->x_size = f;
}

static void wt_set(t_wt *x, t_symbol *s){
    buffer_setarray(x->x_buffer, s);
}

static void wt_none(t_wt *x){
    x->x_interp = 0;
}

static void wt_lin(t_wt *x){
    x->x_interp = 1;
}

static void wt_cos(t_wt *x){
    x->x_interp = 2;
}

static void wt_lagrange(t_wt *x){
    x->x_interp = 3;
}

static void wt_spline(t_wt *x){
    x->x_interp = 4;
}

static void wt_midi(t_wt *x, t_floatarg f){
    x->midi = (int)(f != 0);
}

static void wt_soft(t_wt *x, t_floatarg f){
    x->soft = (int)(f != 0);
}

static t_int *wt_perform(t_int *w){
    t_wt *x = (t_wt *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // sync
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *out = (t_float *)(w[6]);
    t_word *vector = x->x_buffer->c_vectors[0];
    if(!x->x_hasfeeders){ // Magic
        t_float *scalar = x->x_signalscalar;
        if(!else_magic_isnan(*x->x_signalscalar)){
            t_float input_phase = fmod(*scalar, 1);
            if(input_phase < 0)
                input_phase += 1;
            x->x_phase = input_phase;
            else_magic_setnan(x->x_signalscalar);
        }
    }
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    while(n--){
        if(x->x_buffer->c_playable){
            double hz = *in1++;
            if(x->midi)
                hz = pow(2, (hz - 69)/12) * 440;
            double phase_offset = (double)*in3++;
            double phase_step = hz / sr; // phase_step
            phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clip nyq
            if(x->soft)
                phase_step *= (x->soft);
            double phase_dev = phase_offset - last_phase_offset;
            if(phase_dev >= 1 || phase_dev <= -1)
                phase_dev = fmod(phase_dev, 1); // wrap
            if(x->x_hasfeeders){ // signal connected, no magic
                t_float trig = *in2++;
                if(trig > 0 && trig <= 1){
                    if(x->soft)
                        x->soft = x->soft == 1 ? -1 : 1;
                    else
                        phase = trig;
                }
            }
            phase = phase + phase_dev;
            if(phase <= 0)
                phase += 1.; // wrap deviated phase
            if(phase >= 1)
                phase -= 1.; // wrap deviated phase
            if(vector){
                int npts = (t_int)(x->x_buffer->c_npts);
                if(npts < 4) // minimum table size is 4 points.
                    *out++ = 0;
                else{
                    int size = x->x_size > npts ? npts : x->x_size < 0 ? npts : x->x_size;
                    if(size < 4)
                        size = 4;
                    int offset =  x->x_offset;
                    if((offset + size) > npts)
                        offset = npts - size;
                    if(x->x_interp == 0){
                        int ndx = (int)(phase*(double)size);
                        *out++ = (double)vector[ndx].w_float;
                    }
                    else if(x->x_interp >= 3){
                        INDEX_4PT()
                        if(x->x_interp == 3)
                            *out++ = interp_lagrange(frac, a, b, c, d);
                        else
                            *out++ = interp_spline(frac, a, b, c, d);
                    }
                    else{
                        INDEX_2PT()
                        *out++ = x->x_interp == 2 ? interp_cos(frac, b, c) : interp_lin(frac, b, c);
                    }
                }
            }
            else // ??? maybe we dont need "playable"?
                *out++ = 0;
            phase += phase_step; // next phase
            last_phase_offset = phase_offset; // last phase offset
        }
        else
            *out++ = 0;
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return(w+7);
}

static void wt_dsp(t_wt *x, t_signal **sp){
    buffer_checkdsp(x->x_buffer);
    if(x->x_buffer->c_playable && x->x_buffer->c_npts < 4)
        pd_error(x, "[wt~]: table too small, minimum size is 4");
    x->x_hasfeeders = else_magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal);
    x->x_sr = sp[0]->s_sr;
    dsp_add(wt_perform, 6, x, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *wt_free(t_wt *x){
    buffer_free(x->x_buffer);
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    outlet_free(x->x_outlet);
    return(void *)x;
}

static void *wt_new(t_symbol *s, int ac, t_atom *av){
    t_wt *x = (t_wt *)pd_new(wt_class);
    s = NULL;
    t_symbol *name = NULL;
    int nameset = 0, floatarg = 0;
    x->x_freq = x->x_phase = x->x_last_phase_offset = 0.;
    t_float phaseoff = 0;
    x->x_interp = 4;
    x->x_size = -1;
    x->midi = x->soft = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbol(av);
            if(curarg == gensym("-none")){
                if(nameset)
                    goto errstate;
                wt_none(x), ac--, av++;
            }
            else if(curarg == gensym("-lin")){
                if(nameset)
                    goto errstate;
                wt_lin(x), ac--, av++;
            }
            else if(curarg == gensym("-cos")){
                if(nameset)
                    goto errstate;
                wt_cos(x), ac--, av++;
            }
            else if(curarg == gensym("-lagrange")){
                if(nameset)
                    goto errstate;
                wt_lagrange(x), ac--, av++;
            }
            else if(curarg == gensym("-offset")){
                ac--, av++;
                if(nameset)
                    goto errstate;
                x->x_offset = atom_getfloatarg(0, ac, av);
                ac--, av++;
            }
            else if(curarg == gensym("-size")){
                ac--, av++;
                if(nameset)
                    goto errstate;
                x->x_size = atom_getfloatarg(0, ac, av);
                ac--, av++;
            }
            else if(curarg == gensym("-midi")){
                ac--, av++;
                if(nameset)
                    goto errstate;
                x->midi = 1;
            }
            else if(curarg == gensym("-soft")){
                ac--, av++;
                if(nameset)
                    goto errstate;
                x->soft = 1;
            }
            else{
                if(nameset || floatarg)
                    goto errstate;
                name = curarg;
                nameset = 1, ac--, av++;
            }
        }
        else{ //else float
            switch(floatarg){
                case 0:
                    x->x_freq = atom_getfloatarg(0, ac, av);
                    break;
                case 1:
                    phaseoff = atom_getfloatarg(0, ac, av);
                    break;
                default:
                    break;
            };
            floatarg++, ac--, av++;
        };
    };
    x->x_inlet_sync = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_inlet_phase = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, phaseoff < 0 || phaseoff > 1 ? 0 : phaseoff);
    x->x_outlet = outlet_new(&x->x_obj, gensym("signal"));
    // Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    // Magic End
    x->x_buffer = buffer_init((t_class *)x, name, 1, 0);
    return(x);
    errstate:
        post("wt~: improper args");
        return NULL;
}

void wt_tilde_setup(void){
    wt_class = class_new(gensym("wt~"), (t_newmethod)wt_new,
        (t_method)wt_free, sizeof(t_wt), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(wt_class, t_wt, x_freq);
    class_addmethod(wt_class, (t_method)wt_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(wt_class, (t_method)wt_none, gensym("none"), 0);
    class_addmethod(wt_class, (t_method)wt_lin, gensym("lin"), 0);
    class_addmethod(wt_class, (t_method)wt_cos, gensym("cos"), 0);
    class_addmethod(wt_class, (t_method)wt_lagrange, gensym("lagrange"), 0);
    class_addmethod(wt_class, (t_method)wt_spline, gensym("spline"), 0);
    class_addmethod(wt_class, (t_method)wt_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(wt_class, (t_method)wt_offset, gensym("offset"), A_FLOAT, 0);
    class_addmethod(wt_class, (t_method)wt_soft, gensym("soft"), A_DEFFLOAT, 0);
    class_addmethod(wt_class, (t_method)wt_midi, gensym("midi"), A_DEFFLOAT, 0);
    class_addmethod(wt_class, (t_method)wt_set, gensym("set"), A_SYMBOL, 0);
    class_sethelpsymbol(wt_class, gensym("wavetable~"));
}


