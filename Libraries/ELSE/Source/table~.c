// Porres 2018

#include "m_pd.h"
#include "buffer.h"
#include <math.h>

static t_class *table_class;

typedef struct _table{
    t_object  x_obj;
    t_buffer *x_buffer;
    t_outlet *x_outlet;
    t_float   x_ph;
    t_int     x_guard;
    t_int     x_index;
}t_table;

static void table_set(t_table *x, t_symbol *name){
    buffer_setarray(x->x_buffer, name);
}

static void table_index(t_table *x, t_float f){
    x->x_index = f!= 0;
}

static void table_guard(t_table *x, t_float f){
    x->x_guard = f!= 0;
}

static t_int *table_perform_sig(t_int *w){
    t_table *x = (t_table *)(w[1]);
    t_int n = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]); // phase
    t_float *out = (t_float *)(w[4]);
    t_word *vector = x->x_buffer->c_vectors[0];
    t_int nm1 = (t_int)(x->x_buffer->c_npts) - 1;
    while(n--){
        if(x->x_buffer->c_playable && vector){
            t_float phase = *in++;
            if(x->x_guard){ 
                if(x->x_index) // like [tabread4~]
                    phase = phase < 1 ? 1 : phase > nm1 - 1 ? nm1 - 1 : phase;
                else{ // like [tabosc4~]
                    phase = phase < 0 ? 0 : phase >= 1 ? 0 : phase;
                    phase = phase * ((float)nm1 - 2) + 1;
                }
            }
            else{ // read the whole damn table
                if(x->x_index)
                    phase = phase < 0 ? 0 : phase > nm1 ? nm1 : phase;
                else{
                    phase = phase < 0 ? 0 : phase > 1 ? 1 : phase;
                    phase *= (float)nm1;
                }
            }
            t_int ndx = (t_int)(phase);
            if((phase - ndx) == 0) // integer: no interpolation
                *out++ = vector[ndx].w_float;
            else{ // lagrange interpolation
                double frac = (double)phase - (double)ndx;
                t_int ndxm1 = ndx - 1, ndx1 = ndx + 1, ndx2 = ndx + 2;
                if(ndxm1 < 0)
                    ndxm1 = 0;
                if(ndx2 > nm1)
                    ndx2 = nm1;
                double a = (double)vector[ndxm1].w_float;
                double b = (double)vector[ndx].w_float;
                double c = (double)vector[ndx1].w_float;
                double d = (double)vector[ndx2].w_float;
                double cmb = c-b;
                *out++ = (t_float)(b+frac*(cmb-(1.-frac)/6. * ((d-a-3.0*cmb) * frac+d+2.0*a-3.0*b)));
            }
        }
        else
            *out++ = 0;
    }
    return(w+5);
}

static void table_dsp(t_table *x, t_signal **sp){
    buffer_checkdsp(x->x_buffer);
    dsp_add(table_perform_sig, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *table_free(t_table *x){
    buffer_free(x->x_buffer);
    outlet_free(x->x_outlet);
    return(void *)x;
}

static void *table_new(t_symbol *s, int ac, t_atom *av){
    t_table *x = (t_table *)pd_new(table_class);
    t_symbol *name = s;
    name = NULL;
    x->x_guard = 1;
    x->x_index = 0;
    t_int nameset = 0;
    t_int floatarg = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(!nameset){
                name = atom_getsymbolarg(0, ac, av);
                nameset = 1, ac--, av++;
            }
            else
                goto errstate;
        }
        else if(av->a_type == A_FLOAT){
            switch(floatarg){
                case 0:
                    x->x_guard = atom_getfloatarg(0, ac, av) != 0;
                    break;
                case 1:
                    x->x_index = atom_getfloatarg(0, ac, av) != 0;
                    break;
                default:
                    break;
            };
            floatarg++, ac--, av++;
        }
        else
            goto errstate;
    };
    x->x_buffer = buffer_init((t_class *)x, name, 1, 0);
    x->x_outlet = outlet_new(&x->x_obj, gensym("signal"));
    return(x);
    errstate:
        post("table~: improper args");
        return NULL;
}

void table_tilde_setup(void){
    table_class = class_new(gensym("table~"), (t_newmethod)table_new,
        (t_method)table_free, sizeof(t_table), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(table_class, t_table, x_ph);
    class_addmethod(table_class, (t_method)table_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(table_class, (t_method)table_set, gensym("set"), A_SYMBOL, 0);
    class_addmethod(table_class, (t_method)table_guard, gensym("guard"), A_FLOAT, 0);
    class_addmethod(table_class, (t_method)table_index, gensym("index"), A_FLOAT, 0);
}
