// porres 2018

#include <m_pd.h>
#include "math.h"

#define MAX_SIZE  1024

typedef struct _function{
    t_object    x_obj;
    float       x_f;
    float      *x_points;
    float      *x_durations;
    float       x_power;
    t_atom      x_at_exp[MAX_SIZE];
    int         x_point;
    int         x_last_point;
}t_function;

static t_class *function_class;

static t_sample function_interpolate(t_function* x, t_sample f){
    float point_m1 = x->x_points[x->x_point-1];
    float point = x->x_points[x->x_point];
    float dif = point - point_m1;
    float dur = x->x_durations[x->x_point];
    float dur_m1 = x->x_durations[x->x_point-1];
    float frac = (f-dur_m1)/(dur-dur_m1);
    float power = x->x_at_exp[x->x_point-1].a_w.w_float;
    if(fabs(power) == 1) // linear
        return(point_m1 + frac * dif);
    else{
        if(power >= 0){ // positive exponential
            if(point_m1 < point) // ascending
                return(point_m1 + pow(frac, power) * dif);
            else // descending` (invert)
                return(point_m1 + (1-pow(1-frac, power)) * dif);
        }
        else{
            if(point_m1 < point) // ascending
                return(point_m1 + (1-pow(1-frac, -power)) * dif);
            else // descending` (invert)
                return(point_m1 + pow(frac, -power) * dif);
        }
    }
}
    
static t_int *functionsig_perform(t_int *w){
	t_function *x = (t_function *)(w[1]);
    t_sample *in = (t_float *)(w[2]);
    t_sample *out = (t_float *)(w[3]);
    int n = (int)(w[4]);
    if(x->x_point > x->x_last_point)
        x->x_point = x->x_last_point;
    while(n--){
        t_sample f = *in++;
        t_sample val;
        while((x->x_point > 0) && (f < x->x_durations[x->x_point-1]))
            x->x_point--;
        while((x->x_point <  x->x_last_point) && (x->x_durations[x->x_point] < f))
            x->x_point++;
        if(x->x_point == 0 || f >= x->x_durations[x->x_last_point])
            val = x->x_points[x->x_point];
        else
            val = function_interpolate(x, f);
        *out++ = val;
    }
    return(w+5);
}

static void functionsig_dsp(t_function *x, t_signal **sp){
    dsp_add(functionsig_perform, 4, x,sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void function_init(t_function *x, int ac, t_atom* av){ // ???
    if(ac < 3){
        post("[function~] list needs at least 3 floats");
        return;
    }
    float *dur;
    float *val;
    float tdur = 0;
    x->x_durations[0] = 0;
    x->x_last_point = ac >> 1; // = (int)ac/2
    if(x->x_last_point > MAX_SIZE){
        post("[function]: too many lines, maximum is %d", MAX_SIZE);
        return;
    }
    dur = x->x_durations;
    val = x->x_points;
    *val = atom_getfloat(av++);
    *dur = 0.0;
    ac--;
    dur++;
    while(ac > 0){
        *dur++ = (tdur += atom_getfloat(av++));
        ac--;
        *++val = ac > 0 ? atom_getfloat(av++): 0;
        ac--;
    }
}

static void function_exp(t_function *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    s = NULL;
    for(int i = 0; i < ac; i++)
        SETFLOAT(x->x_at_exp+i, (av+i)->a_w.w_float);
}

static void function_norm_dur(t_function* x){ // normalize duration
    for(int i = 1; i <= x->x_last_point; i++)
        x->x_durations[i] /= x->x_durations[x->x_last_point];
}

static void function_list(t_function *x,t_symbol* s, int ac,t_atom* av){
    s = NULL;
    function_init(x, ac, av);
    function_norm_dur(x);
}

static void *function_new(t_symbol *s,int ac,t_atom* av){
    s = NULL; // avoid warning
    t_function *x = (t_function *)pd_new(function_class);
    int i;
    for(i = 0; i < MAX_SIZE; i++) // set exponential list to linear
        SETFLOAT(x->x_at_exp+i, 1);
    x->x_f = 0;
    x->x_points = getbytes(MAX_SIZE*sizeof(float));
    x->x_durations = getbytes(MAX_SIZE*sizeof(float));
    int symarg = 0, n = 0;
    while(ac > 0){
        if((av+n)->a_type == A_FLOAT && !symarg)
            n++, ac--;
        else if((av+n)->a_type == A_SYMBOL){
            if(!symarg){ // set envelope
                symarg = 1;
                if(n){
                    if(n < 3)
                        pd_error(x, "[function~]: needs at least 3 float arguments");
                    else
                        function_init(x, n, av);
                }
            }
            if(atom_getsymbolarg(0, ac, av+n) == gensym("-exp")){
                ac--, av++;
                if(ac){
                    i = 0;
                    while(ac){
                        if((av+n)->a_type != A_FLOAT)
                            goto errstate;
                        SETFLOAT(x->x_at_exp+i, (av+n)->a_w.w_float);
                        ac--, av++, i++;
                    }
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    }
    if(!symarg && n > 0){ // set envelope
        if(n < 3)
            pd_error(x, "[function~]: needs at least 3 float arguments");
        else
            function_init(x, n, av);
    }
    function_norm_dur(x);
    outlet_new(&x->x_obj, gensym("signal"));
    return(x);
errstate:
    pd_error(x, "[function~]: improper args");
    return(NULL);
}

void function_tilde_setup(void){
    function_class = class_new(gensym("function~"), (t_newmethod)function_new, 0,
    	sizeof(t_function), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(function_class, t_function, x_f);
    class_addmethod(function_class, (t_method)functionsig_dsp, gensym("dsp"), 0);
    class_addmethod(function_class, (t_method)function_exp, gensym("exp"), A_GIMME, 0);
    class_addlist(function_class, function_list);
}
