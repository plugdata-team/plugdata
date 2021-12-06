// Porres 2017-2020

#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <string.h>

static t_class *sr_class;

typedef struct _settings{
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, callback, sr;
}t_settings;

typedef struct _sr{
    t_object    x_obj;
    t_clock    *x_clock;
    float       x_sr;
    float       x_new_sr;
    int         x_khz;
    int         x_period;
    t_settings  x_settings;
}t_sr;

///////////////////////////////////////////////////////////////////////////////////////////

static void audio_settings(int *pnaudioindev, int *paudioindev, int *pchindev, int *pnaudiooutdev,
    int *paudiooutdev, int *pchoutdev, int *prate, int *padvance, int *pcallback, int *psr){
        sys_get_audio_params(pnaudioindev , paudioindev , pchindev, pnaudiooutdev,
            paudiooutdev, pchoutdev, prate, padvance, pcallback, psr);
}

static void sr_apply(t_sr *x){
    t_atom av [2*MAXAUDIOINDEV + 2*MAXAUDIOOUTDEV + 3];
    int ac = 2*MAXAUDIOINDEV + 2*MAXAUDIOOUTDEV + 3;
    int i = 0;
    for(i = 0; i < MAXAUDIOINDEV; i++){
        SETFLOAT(av+i + 0*MAXAUDIOINDEV, (float)(x->x_settings.audioindev[i]));
        SETFLOAT(av+i + 1*MAXAUDIOINDEV, (float)(x->x_settings.chindev[i]));
    }
    for(i = 0; i < MAXAUDIOOUTDEV; i++){
        SETFLOAT(av+i + 2*MAXAUDIOINDEV + 0*MAXAUDIOOUTDEV, (float)(x->x_settings.audiooutdev[i]));
        SETFLOAT(av+i + 2*MAXAUDIOINDEV + 1*MAXAUDIOOUTDEV, (float)(x->x_settings.choutdev[i]));
    }
    SETFLOAT(av+2 * MAXAUDIOINDEV + 2*MAXAUDIOOUTDEV + 0, (float)(x->x_settings.rate));
    SETFLOAT(av+2 * MAXAUDIOINDEV + 2*MAXAUDIOOUTDEV + 1, (float)(x->x_settings.advance));
    SETFLOAT(av+2 * MAXAUDIOINDEV + 2*MAXAUDIOOUTDEV + 2, (float)(x->x_settings.callback));
    if(gensym("pd")->s_thing)
        typedmess(gensym("pd")->s_thing, gensym("audio-dialog"), ac, av);
}

static void get_settings(t_settings *setts){
    int i = 0;
    memset(setts, 0, sizeof(t_settings));
    setts->callback = -1;
    audio_settings(&setts->naudioindev,  setts->audioindev,  setts->chindev,
               &setts->naudiooutdev, setts->audiooutdev, setts->choutdev, &setts->rate,
               &setts->advance, &setts->callback,    &setts->sr);
    for(i = setts->naudioindev; i < MAXAUDIOINDEV; i++){
        setts->audioindev[i] = 0;
        setts->chindev[i] = 0;
    }
    for(i = setts->naudiooutdev; i < MAXAUDIOOUTDEV; i++){
        setts->audiooutdev[i] = 0;
        setts->choutdev[i] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

static void sr_set(t_sr *x, t_floatarg f){
    int rate = (int)f;
    if(rate > 0){
        x->x_settings.rate = rate;
        sr_apply(x);
    }
}

static void sr_bang(t_sr *x){
    float sr = sys_getsr();
    x->x_sr = x->x_new_sr = sr;
    if(x->x_khz)
        sr *= 0.001;
    if(x->x_period)
        sr = 1./sr;
    outlet_float(x->x_obj.ob_outlet, sr);
}

static void sr_hz(t_sr *x){
    x->x_khz = x->x_period = 0;
    sr_bang(x);
}

static void sr_khz(t_sr *x){
    x->x_khz = 1;
    x->x_period = 0;
    sr_bang(x);
}

static void sr_ms(t_sr *x){
    x->x_khz = x->x_period = 1;
    sr_bang(x);
}

static void sr_sec(t_sr *x){
    x->x_khz = 0;
    x->x_period = 1;
    sr_bang(x);
}

static void sr_loadbang(t_sr *x, t_floatarg action){
    if(action == LB_LOAD)
        sr_bang(x);
}

static void sr_tick(t_sr *x){
    if(x->x_new_sr != x->x_sr)
        sr_bang(x);
}

static t_int *sr_perform(t_int *w){
    t_sr *x = (t_sr *)(w[1]);
    clock_delay(x->x_clock, 0);
    return(w+2);
}

static void sr_dsp(t_sr *x, t_signal **sp){
    x->x_new_sr = (float)sp[0]->s_sr;
    dsp_add(sr_perform, 1, x);
}

static void sr_free(t_sr *x){
    clock_free(x->x_clock);
}

static void *sr_new(t_symbol *s, int ac, t_atom *av){
    t_sr *x = (t_sr *)pd_new(sr_class);
    get_settings(&x->x_settings);
    x->x_khz = x->x_period = 0;
    if(ac <= 2){
        while(ac){
            if(av->a_type == A_SYMBOL){
                t_symbol *sym = s; // get rid of warning
                sym = atom_getsymbolarg(0, ac, av);
                if(sym == gensym("-khz"))
                    x->x_khz = 1;
                else if(sym == gensym("-ms"))
                    x->x_khz = x->x_period = 1;
                else if(sym == gensym("-sec"))
                    x->x_period = 1;
                else goto errstate;
                ac--, av++;
            }
            else{
                sr_set(x, atom_getfloatarg(0, ac, av));
                ac--, av++;
            }
        }
    }
    else goto errstate;
    x->x_clock = clock_new(x, (t_method)sr_tick);
    outlet_new(&x->x_obj, &s_float);
    return(x);
    errstate:
        pd_error(x, "[sr~]: improper args");
        return(NULL);
}

void sr_tilde_setup(void){
    sr_class = class_new(gensym("sr~"), (t_newmethod)sr_new,
        (t_method)sr_free, sizeof(t_sr), 0, A_GIMME, 0);
    class_addmethod(sr_class, nullfn, gensym("signal"), 0);
    class_addmethod(sr_class, (t_method)sr_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(sr_class, (t_method)sr_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    class_addmethod(sr_class, (t_method)sr_hz, gensym("hz"), 0);
    class_addmethod(sr_class, (t_method)sr_khz, gensym("khz"), 0);
    class_addmethod(sr_class, (t_method)sr_ms, gensym("ms"), 0);
    class_addmethod(sr_class, (t_method)sr_sec, gensym("sec"), 0);
    class_addmethod(sr_class, (t_method)sr_set, gensym("set"), A_DEFFLOAT, 0);
    class_addbang(sr_class, (t_method)sr_bang);
}
