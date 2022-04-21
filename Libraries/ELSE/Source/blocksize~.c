// porres 2018-2020

#include "m_pd.h"

static t_class *blocksize_class;

typedef struct _blocksize{
    t_object    x_obj;
    t_clock    *x_clock;
    float       x_size;
    float       x_n;
    float       x_sr;
    float       x_new_sr;
    int         x_mode;
}t_blocksize;

static void blocksize_bang(t_blocksize *x){
    if(x->x_size == 0)
        return;
    float size = x->x_size;
    if(x->x_mode == 1)
        size *= (1000./x->x_sr);
    else if(x->x_mode == 2)
        size = x->x_sr/size;
    outlet_float(x->x_obj.ob_outlet, size);
}

static void blocksize_samps(t_blocksize *x){
    if(x->x_mode != 0){
        x->x_mode = 0;
        blocksize_bang(x);
    }
}

static void blocksize_ms(t_blocksize *x){
    if(x->x_mode != 1){
        x->x_mode = 1;
        blocksize_bang(x);
    }
}

static void blocksize_hz(t_blocksize *x){
    if(x->x_mode != 2){
        x->x_mode = 2;
        blocksize_bang(x);
    }
}

static void blocksize_tick(t_blocksize *x){
    if(x->x_n != x->x_size){
        float size = x->x_size = x->x_n;
        if(x->x_mode){
            if(x->x_mode == 1)
                size *= (1000./x->x_sr);
            else if(x->x_mode == 2)
                size = x->x_sr/size;
        }
        outlet_float(x->x_obj.ob_outlet, size);
    }
    else if(x->x_new_sr != x->x_sr && x->x_mode){
        x->x_sr = x->x_new_sr;
        float size = x->x_size;
        if(x->x_mode == 1)
            size *= (1000./x->x_sr);
        else if(x->x_mode == 2)
            size = x->x_sr/size;
        outlet_float(x->x_obj.ob_outlet, size);
    }
}

static t_int *blocksize_perform(t_int *w){
    t_blocksize *x = (t_blocksize *)(w[1]);
    clock_delay(x->x_clock, 0);
    return(w+2);
}

static void blocksize_dsp(t_blocksize *x, t_signal **sp){
    x->x_n = (float)sp[0]->s_n;
    x->x_new_sr = (float)sp[0]->s_sr;
    dsp_add(blocksize_perform, 1, x);
}

static void blocksize_free(t_blocksize *x){
    clock_free(x->x_clock);
}

static void *blocksize_new(t_symbol *s, int ac, t_atom *av){
    t_blocksize *x = (t_blocksize *)pd_new(blocksize_class);
    s = NULL;
    x->x_size = x->x_mode = 0;
    x->x_sr = x->x_new_sr = (float)sys_getsr();
    if(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-ms"))
                x->x_mode = 1;
            else if(sym == gensym("-hz"))
                x->x_mode = 2;
            else goto errstate;
        }
        else goto errstate;
    }
    x->x_clock = clock_new(x, (t_method)blocksize_tick);
    outlet_new(&x->x_obj, &s_float);
    return(x);
errstate:
    pd_error(x, "[blocksize~]: improper args");
    return(NULL);
}

void blocksize_tilde_setup(void){
    blocksize_class = class_new(gensym("blocksize~"), (t_newmethod)blocksize_new,
        (t_method)blocksize_free, sizeof(t_blocksize), 0, A_GIMME, 0);
    class_addmethod(blocksize_class, nullfn, gensym("signal"), 0);
    class_addmethod(blocksize_class, (t_method)blocksize_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(blocksize_class, (t_method)blocksize_ms, gensym("ms"), 0);
    class_addmethod(blocksize_class, (t_method)blocksize_hz, gensym("hz"), 0);
    class_addmethod(blocksize_class, (t_method)blocksize_samps, gensym("samps"), 0);
    class_addbang(blocksize_class, (t_method)blocksize_bang);
}
