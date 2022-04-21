// Porres 2017-2020

#include "m_pd.h"
#include "g_canvas.h"
#include <string.h>

static t_class *nyquist_class;

typedef struct _nyquist{
    t_object    x_obj;
    t_clock    *x_clock;
    float       x_new_sr;
    float       x_sr;
    int         x_khz;
    int         x_period;
}t_nyquist;

static void nyquist_bang(t_nyquist *x){
    x->x_sr = x->x_new_sr = sys_getsr();
    float nyquist = x->x_sr * 0.5;
    if(x->x_khz)
        nyquist *= 0.001;
    if(x->x_period)
        nyquist = 1./nyquist;
    outlet_float(x->x_obj.ob_outlet, nyquist);
}

static void nyquist_hz(t_nyquist *x){
    x->x_khz = x->x_period = 0;
    nyquist_bang(x);
}

static void nyquist_khz(t_nyquist *x){
    x->x_khz = 1;
    x->x_period = 0;
    nyquist_bang(x);
}

static void nyquist_ms(t_nyquist *x){
    x->x_khz = x->x_period = 1;
    nyquist_bang(x);
}

static void nyquist_sec(t_nyquist *x){
    x->x_khz = 0;
    x->x_period = 1;
    nyquist_bang(x);
}

static void nyquist_loadbang(t_nyquist *x, t_floatarg action){
    if(action == LB_LOAD)
        nyquist_bang(x);
}

static void nyquist_tick(t_nyquist *x){
    if(x->x_new_sr != x->x_sr)
        nyquist_bang(x);
}

static t_int *nyquist_perform(t_int *w){
    t_nyquist *x = (t_nyquist *)(w[1]);
    clock_delay(x->x_clock, 0);
    return(w+2);
}

static void nyquist_dsp(t_nyquist *x, t_signal **sp){
    x->x_new_sr = (float)sp[0]->s_sr;
    dsp_add(nyquist_perform, 1, x);
}

static void nyquist_free(t_nyquist *x){
    clock_free(x->x_clock);
}

static void *nyquist_new(t_symbol *s, int ac, t_atom *av){
    t_nyquist *x = (t_nyquist *)pd_new(nyquist_class);
    s = NULL;
    x->x_khz = x->x_period = 0;
    if(ac && av->a_type == A_SYMBOL){
        t_symbol *curarg = atom_getsymbolarg(0, ac, av);
        if(!strcmp(curarg->s_name, "-khz"))
            x->x_khz = 1;
        else if(!strcmp(curarg->s_name, "-ms"))
            x->x_khz = x->x_period = 1;
        else if(!strcmp(curarg->s_name, "-sec"))
            x->x_period = 1;
        else
            goto errstate;
    }
    x->x_clock = clock_new(x, (t_method)nyquist_tick);
    outlet_new(&x->x_obj, &s_float);
    return(x);
errstate:
    pd_error(x, "[nyquist~]: improper args");
    return(NULL);
}

void nyquist_tilde_setup(void){
    nyquist_class = class_new(gensym("nyquist~"), (t_newmethod)nyquist_new,
        (t_method)nyquist_free, sizeof(t_nyquist), 0, A_GIMME, 0);
    class_addmethod(nyquist_class, nullfn, gensym("signal"), 0);
    class_addmethod(nyquist_class, (t_method)nyquist_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(nyquist_class, (t_method)nyquist_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    class_addmethod(nyquist_class, (t_method)nyquist_hz, gensym("hz"), 0);
    class_addmethod(nyquist_class, (t_method)nyquist_khz, gensym("khz"), 0);
    class_addmethod(nyquist_class, (t_method)nyquist_ms, gensym("ms"), 0);
    class_addmethod(nyquist_class, (t_method)nyquist_sec, gensym("sec"), 0);
    class_addbang(nyquist_class, (t_method)nyquist_bang);
}
