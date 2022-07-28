// porres 2017

#include "m_pd.h"
#include "math.h"

#define OFF      0
#define RUNNING  1
#define PAUSED   2

typedef struct _loop{
    t_object    x_obj;
    float       x_counter_start;
    float       x_target;
    float       x_offset;
    double      x_count;
    float       x_step;
    t_int       x_iter;
    t_int       x_upwards;
    t_int       x_status;
    t_int       x_b;
}t_loop;

static t_class *loop_class;

static void loop_do_loop(t_loop *x){ // The Actual Loop
    x->x_status = RUNNING;
    if(x->x_iter){
        while(x->x_count <= ((double)x->x_target * (double)x->x_step)){
            if(x->x_b)
                outlet_bang(((t_object *)x)->ob_outlet);
            else
                outlet_float(((t_object *)x)->ob_outlet, x->x_count + x->x_offset);
            x->x_count += (double)x->x_step;
            if(x->x_status == PAUSED)
                return;
        }
    }
    else{
        t_float range = fabs((x->x_target - x->x_count) / x->x_step);
        int n = (int)range + 1;
        while(n--){
            if(x->x_b)
                outlet_bang(((t_object *)x)->ob_outlet);
            else{
                outlet_float(((t_object *)x)->ob_outlet, x->x_count + x->x_offset);
                if(x->x_upwards)
                    x->x_count += (double)x->x_step;
                else
                    x->x_count -= (double)x->x_step;
            }
            if(x->x_status == PAUSED)
                return;
        }
    }
    x->x_status = OFF;
}

static void loop_bang(t_loop *x){
    if(x->x_status != RUNNING){
        x->x_count = x->x_counter_start; // reset
        loop_do_loop(x);
    }
}

static void loop_range(t_loop *x, t_symbol *s, int ac, t_atom *av){
    if(ac == 2){
        s = NULL;
        x->x_counter_start = atom_getfloat(av);
        x->x_target = atom_getfloat(av+1);
    }
}

static void loop_set(t_loop *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    else if(ac == 1){
        t_float f = atom_getfloat(av);
        if(f < 1){
            pd_error(x, "[loop]: number of iterations need to be >= 1");
            return;
        }
        x->x_counter_start = 0;
        x->x_target = (int)f - 1;
        x->x_upwards = x->x_iter = 1;
        return;
    }
    else{
        s = NULL;
        x->x_counter_start = atom_getfloat(av);
        x->x_target = atom_getfloat(av+1);
        if(ac == 3){
            float step = atom_getfloat(av+2);
            if(step <= 0)
                pd_error(x, "[loop]: step needs to be > 0");
            else
                x->x_step = step;
        }
        x->x_upwards = x->x_counter_start < x->x_target;
        x->x_iter = 0;
    }
}

static void loop_float(t_loop *x, t_float f){
    if(f < 1){
        pd_error(x, "[loop]: number of iterations need to be >= 1");
        return;
    }
    x->x_counter_start = 0;
    x->x_target = (int)f - 1;
    x->x_upwards = x->x_iter = 1;
    loop_bang(x);
}

static void loop_list(t_loop *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac < 2){
        if(!ac)
            loop_bang(x);
        else if(ac == 1)
            loop_float(x, atom_getfloat(av));
        return;
    }
    x->x_counter_start = atom_getfloat(av);
    x->x_target = atom_getfloat(av+1);
    if(ac == 3){
        float step = atom_getfloat(av+2);
        if(step <= 0)
            pd_error(x, "[loop]: step needs to be > 0");
        else
            x->x_step = step;
    }
    x->x_upwards = x->x_counter_start < x->x_target;
    x->x_iter = 0;
    loop_bang(x);
}

static void loop_pause(t_loop *x){
    if(x->x_status == RUNNING)
        x->x_status = PAUSED;
}

static void loop_continue(t_loop *x){
    if(x->x_status == PAUSED)
        loop_do_loop(x);
}

static void loop_offset(t_loop *x, t_float f){
    x->x_offset = f;
}

static void loop_step(t_loop *x, t_float f){
    if(f <= 0)
        pd_error(x, "[loop]: step needs to be > 0");
    else
        x->x_step = f;
}

static void *loop_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_loop *x = (t_loop *)pd_new(loop_class);
    x->x_status = OFF;
    t_float f1 = 0, f2 = 0, offset = 0, step = 1;
    x->x_upwards = x->x_iter = 1;
    x->x_b = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ // if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    f1 = argval;
                    break;
                case 1:
                    f2 = argval;
                    x->x_iter = 0;
                    break;
                case 2:
                    step = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--, argv++;
        }
        else if(argv->a_type == A_SYMBOL && !argnum){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(curarg == gensym("-offset")){
                offset = atom_getfloatarg(0, argc, argv+1);
                argc-=2, argv+=2;
            }
            else if(curarg == gensym("-step")){
                step = atom_getfloatarg(0, argc, argv+1);
                argc-=2, argv+=2;
            }
            else if(curarg == gensym("-b")){
                x->x_b = 1;
                argc--, argv++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
    ///////////////////////////////////////////////////////////////////////////////////
    x->x_offset = (int)offset;
    if(step <= 0){
        step = 1;
        pd_error(x, "[loop]: step needs to be > 0 - set to default (1)");
    }
    x->x_step = step;
    if(x->x_iter){ // only one arg
        if(f1 < 1)
            f1 = 1;
        x->x_target = (int)f1 - 1;
    }
    else{
        x->x_counter_start = f1;
        x->x_target = f2;
        x->x_upwards = x->x_counter_start < x->x_target;
    }
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("set"));
    outlet_new((t_object *)x, 0);
    return(x);
errstate:
    pd_error(x, "[loop]: improper args");
    return(NULL);
}

void loop_setup(void){
    loop_class = class_new(gensym("loop"), (t_newmethod)loop_new, 0, sizeof(t_loop), 0, A_GIMME, 0);
    class_addlist(loop_class, loop_list);
    class_addmethod(loop_class, (t_method)loop_pause, gensym("pause"), 0);
    class_addmethod(loop_class, (t_method)loop_continue, gensym("continue"), 0);
    class_addmethod(loop_class, (t_method)loop_offset, gensym("offset"), A_DEFFLOAT, 0);
    class_addmethod(loop_class, (t_method)loop_step, gensym("step"), A_DEFFLOAT, 0);
    class_addmethod(loop_class, (t_method)loop_set, gensym("set"), A_GIMME, 0);
    class_addmethod(loop_class, (t_method)loop_range, gensym("range"), A_GIMME, 0);
}
