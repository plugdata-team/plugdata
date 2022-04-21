// Porres 2017

#include "m_pd.h"
#include "g_canvas.h"

typedef struct _loadbanger{
    t_object    x_ob;
    int         x_nouts;
    int         x_init;
    t_outlet  **x_outs;
    t_outlet   *x_outbuf[1];
}t_loadbanger;

static t_class *loadbanger_class;

static void loadbanger_loadbang(t_loadbanger *x, t_float f){
    if(x->x_init){
        if((int)f == LB_INIT){ // == LB_INIT (1) and "-init"
            int i = x->x_nouts;
            while(i--)
                outlet_bang(x->x_outs[i]);
        }
    }
    else if((int)f == LB_LOAD){ // == LB_LOAD (0) and hasn't banged yet (next)
        int j = x->x_nouts;
        while(j--)
            outlet_bang(x->x_outs[j]);
    }
}

static void loadbanger_click(t_loadbanger *x, t_floatarg xpos,
t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    xpos = ypos = shift = ctrl = alt = 0;
    int i = x->x_nouts;
    while(i--)
        outlet_bang(x->x_outs[i]);
}

static void loadbanger_bang(t_loadbanger *x){
    int i = x->x_nouts;
    while(i--)
        outlet_bang(x->x_outs[i]);
}

static void loadbanger_anything(t_loadbanger *x){
    int i = x->x_nouts;
    while(i--)
        outlet_bang(x->x_outs[i]);
}

static void loadbanger_free(t_loadbanger *x){
    if(x->x_outs != x->x_outbuf)
        freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void *loadbanger_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_loadbanger *x = (t_loadbanger *)pd_new(loadbanger_class);
    int i, nouts = 1;
    t_outlet **outs;
    x->x_init = 0;
    t_float float_flag = 0;
/////////////////////////////////////////////////////////////////////////////////////
    if(argc <= 2){
        while(argc > 0){
            if(argv->a_type == A_FLOAT && !float_flag){
                nouts = atom_getfloatarg(0, argc, argv);
                argc--, argv++;
                float_flag = 1;
            }
            else
                if(argv->a_type == A_SYMBOL && !float_flag){
                    t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
                    if(curarg == gensym("-init")){
                        x->x_init = 1;
                        argc--, argv++;
                    }
                    else
                        goto errstate;
                }
                else
                    goto errstate;
        };
    }
    else
        goto errstate;
/////////////////////////////////////////////////////////////////////////////////////
    if(nouts < 1)
        nouts = 1;
    if(nouts > 64)
        nouts = 64;
    if(nouts > 1){
        if(!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
            return (0);
    }
    else
        outs = 0;
    x->x_nouts = nouts;
    x->x_outs = (outs ? outs : x->x_outbuf);
    for(i = 0; i < nouts; i++)
        x->x_outs[i] = outlet_new((t_object *)x, &s_bang);
    return(x);
errstate:
    pd_error(x, "[loadbanger]: improper args");
    return(NULL);
}

void loadbanger_setup(void){
    loadbanger_class = class_new(gensym("loadbanger"), (t_newmethod)loadbanger_new,
        (t_method)loadbanger_free, sizeof(t_loadbanger), 0, A_GIMME, 0);
    class_addbang(loadbanger_class, loadbanger_bang);
    class_addanything(loadbanger_class, loadbanger_anything);
    class_addmethod(loadbanger_class, (t_method)loadbanger_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    class_addmethod(loadbanger_class, (t_method)loadbanger_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT,0);
}
