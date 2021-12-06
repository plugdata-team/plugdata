// porres 2018-2019

#include "m_pd.h"
#include <math.h>

#define MAX_SEGS  1024 // maximum line segments

static t_class *envgen_proxy;

typedef struct _envgen_proxy{
    t_pd             p_pd;
    struct _envgen  *p_owner;
}t_proxy;

typedef struct _envgenseg{
    float  target;  // line target
    float  ms;      // line time in ms
}t_envgen_line;

typedef struct _envgen{
    t_object        x_obj;
    t_proxy         x_proxy;
    int             x_ac;
    int             x_ac_rel;
    int             x_nleft;
    int             x_n_lines;
    int             x_line_n;
    int             x_pause;
    int             x_status;
    int             x_legato;
    int             x_release;
    int             x_suspoint;
    int             x_n;
    float           x_power;
    float           x_value;
    float           x_target;
    float           x_last_target;
    float           x_inc;
    float           x_delta;
    float           x_lastin;
    float           x_maxsustain;
    float           x_retrigger;
    float           x_gain;
    t_atom         *x_av;
    t_atom         *x_av_rel;
    t_atom          x_at_exp[MAX_SEGS];
    t_envgen_line  *x_curseg;
    t_envgen_line  *x_lines;
    t_envgen_line   x_n_lineini[MAX_SEGS];
    t_clock        *x_clock;
    t_clock        *x_sus_clock;
    t_outlet       *x_out2;
}t_envgen;

static t_class *envgen_class;

static void copy_atoms(t_atom *src, t_atom *dst, int n){
    while(n--)
        *dst++ = *src++;
}

static void envgen_proxy_init(t_proxy * p, t_envgen *x){
    p->p_pd = envgen_proxy;
    p->p_owner = x;
}

static float envgen_get_step(t_envgen *x){
    float step = (float)(x->x_n-x->x_nleft)/(float)x->x_n;
    if(fabs(x->x_power) != 1){ // EXPONENTIAL
        if(x->x_power >= 0){ // positive exponential
            if((x->x_delta > 0) == (x->x_gain > 0))
                step = pow(step, x->x_power);
            else
                step = 1-pow(1-step, x->x_power);
        }
        else{ // negative exponential
            if((x->x_delta > 0) == (x->x_gain > 0))
                step = 1-pow(1-step, -x->x_power);
            else
                step = pow(step, -x->x_power);
        }
    }
    return(step);
}

static void envgen_proxy_list(t_proxy *p, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    s = NULL;
    t_envgen *x = p->p_owner;
    for(int n = 0; n < ac; n++)
        if((av+n)->a_type != A_FLOAT){
            pd_error(x, "[envgen~]: needs to only contain floats");
            return;
        }
    x->x_ac = ac;
    x->x_av = getbytes(x->x_ac*sizeof(*(x->x_av)));
    copy_atoms(av, x->x_av, x->x_ac);
}

static void envgen_retarget(t_envgen *x, int skip){
    x->x_target = x->x_curseg->target;
    if(skip)
        x->x_power = 1;
    else{
        x->x_power = x->x_at_exp[x->x_line_n].a_w.w_float;
        x->x_line_n++;
    }
    x->x_nleft = (int)(x->x_curseg->ms * sys_getsr()*0.001 + 0.5);
    x->x_n_lines--, x->x_curseg++;
    if(x->x_nleft == 0){ // stupid line's gonna be ignored
        x->x_value = x->x_last_target = x->x_target;
        x->x_delta = x->x_inc = 0;
        x->x_power = x->x_at_exp[x->x_line_n].a_w.w_float;
        while(x->x_n_lines && // others to be ignored
              !(int)(x->x_curseg->ms * sys_getsr()*0.001 + 0.5)){
            x->x_value = x->x_target = x->x_curseg->target;
            x->x_n_lines--, x->x_curseg++, x->x_line_n++;
            x->x_power = x->x_at_exp[x->x_line_n].a_w.w_float;
        }
    }
    else{
        x->x_delta = (x->x_target - (x->x_last_target = x->x_value));
        x->x_n = x->x_nleft;
        x->x_nleft--; // update it already
        x->x_inc = x->x_delta != 0 ? envgen_get_step(x) * x->x_delta : 0;
    }
}

static void envgen_attack(t_envgen *x, int ac, t_atom *av){
    x->x_line_n = 0;
    int odd = ac % 2;
    int n_lines = ac/2;
    int skip1st = 0;
    if(odd){ // non legato mode, restart from first value
        if(x->x_legato) // actually legato
            av++, ac--; // skip 1st atom
        else{ // ok, non legato really
            n_lines++; // add extra segment
            skip1st = 1; // for release
        }
    }
    if(n_lines > MAX_SEGS)
        n_lines = MAX_SEGS, odd = 0;
    if(x->x_suspoint){ // find sustain point
        if((n_lines - skip1st) >= x->x_suspoint){ // we have a release!
            x->x_release = 1;
            n_lines = x->x_suspoint; // limit n_lines to suspoint
            int n = 2*n_lines + skip1st;
            x->x_ac_rel = (ac -= n); // release n
            x->x_av_rel = getbytes(x->x_ac_rel*sizeof(*(x->x_av_rel)));
            copy_atoms(av+n, x->x_av_rel, x->x_ac_rel); // copy release ramp
            if(x->x_maxsustain > 0)
                clock_delay(x->x_sus_clock, x->x_maxsustain);
        }
        else // no release
            x->x_release = 0;
    }
    else // no release
        x->x_release = 0;
// attack
    x->x_n_lines = n_lines; // define number of line segments
    t_envgen_line *line_point = x->x_lines;
    if(odd && !x->x_legato){ // initialize 1st segment
        line_point->ms = x->x_status ? x->x_retrigger : 0;
        line_point->target = (av++)->a_w.w_float * x->x_gain;
        line_point++;
        n_lines--;
    }
    while(n_lines--){
        line_point->ms = (av++)->a_w.w_float;
        if(line_point->ms < 0)
            line_point->ms = 0;
        line_point->target = (av++)->a_w.w_float * x->x_gain;
        line_point++;
    }
    x->x_curseg = x->x_lines;
    envgen_retarget(x, skip1st);
    if(x->x_pause)
        x->x_pause = 0;
    if(!x->x_status)
        outlet_float(x->x_out2, x->x_status = 1); // turn on status
}

static void envgen_release(t_envgen *x, int ac, t_atom *av){
    if(ac < 2)
        return;
    int n_lines = ac/2;
    x->x_n_lines = n_lines; // define number of line segments
    t_envgen_line *line_point = x->x_lines;
    while(n_lines--){
        line_point->ms = av++->a_w.w_float;
        if(line_point->ms < 0)
            line_point->ms = 0;
        line_point->target = av++->a_w.w_float * x->x_gain;
        line_point++;
    }
    x->x_target = x->x_lines->target;
    x->x_curseg = x->x_lines;
    x->x_release = 0;
    envgen_retarget(x, 0);
    if(x->x_pause)
        x->x_pause = 0;
}

static void envgen_tick(t_envgen *x){
    if(x->x_status && !x->x_release)
        outlet_float(x->x_out2, x->x_status = 0);
}

static void envgen_sus_tick(t_envgen *x){
    if(x->x_release)
        envgen_release(x, x->x_ac_rel, x->x_av_rel);
}

static void envgen_exp(t_envgen *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    s = NULL;
    for(int i = 0; i < ac; i++)
        SETFLOAT(x->x_at_exp+i, (av+i)->a_w.w_float);
}

static void envgen_set(t_envgen *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    s = NULL;
    for(int n = 0; n < ac; n++)
        if((av+n)->a_type != A_FLOAT){
            pd_error(x, "[envgen~]: set needs to only contain floats");
            return;
        }
    x->x_av = getbytes(x->x_ac * sizeof(*(x->x_av)));
    copy_atoms(av, x->x_av, x->x_ac = ac);
}

static void envgen_list(t_envgen *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    for(int n = 0; n < ac; n++)
        if((av+n)->a_type != A_FLOAT){
            pd_error(x, "[envgen~]: list needs to only contain floats");
            return;
        }
    x->x_ac = ac;
    x->x_av = getbytes(x->x_ac * sizeof(*(x->x_av)));
    copy_atoms(av, x->x_av, x->x_ac);
    envgen_attack(x, x->x_ac, x->x_av);
}

static void envgen_bang(t_envgen *x){
    envgen_attack(x, x->x_ac, x->x_av);
}

static void envgen_rel(t_envgen *x){
    if(x->x_release)
        envgen_release(x, x->x_ac_rel, x->x_av_rel);
}

static void envgen_float(t_envgen *x, t_floatarg f){
    if(f != 0){
        x->x_gain = f;
        envgen_attack(x, x->x_ac, x->x_av);
    }
    else{
        if(x->x_release)
            envgen_release(x, x->x_ac_rel, x->x_av_rel);
    }
}

static void envgen_setgain(t_envgen *x, t_floatarg f){
    if(f != 0)
        x->x_gain = f;
}

static void envgen_retrigger(t_envgen *x, t_floatarg f){
    x->x_retrigger = f < 0 ? 0 : f;
}

static void envgen_legato(t_envgen *x, t_floatarg f){
    x->x_legato = f != 0;
}

static void envgen_maxsustain(t_envgen *x, t_floatarg f){
    x->x_maxsustain = f;
}

static void envgen_suspoint(t_envgen *x, t_floatarg f){
    x->x_suspoint = f < 0 ? 0 : (int)f;
}

static t_int *envgen_perform(t_int *w){
    t_envgen *x = (t_envgen *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    float lastin = x->x_lastin;
    while(n--){
        t_float f = *in++;
        if(f != 0 && lastin == 0){ // set attack ramp
            x->x_gain = f;
            envgen_attack(x, x->x_ac, x->x_av);
        }
        else if(x->x_release && f == 0 && lastin != 0) // set release ramp
            envgen_release(x, x->x_ac_rel, x->x_av_rel);
        if(PD_BIGORSMALL(x->x_value)) // ??????????????
            x->x_value = 0;
        *out++ = x->x_value = x->x_last_target + x->x_inc;
        if(!x->x_pause && x->x_status){ // not paused and 'on' => let's update
            if(x->x_nleft > 0){ // increase
                x->x_nleft--;
                x->x_inc = x->x_delta != 0 ? envgen_get_step(x) * x->x_delta : 0;
            }
            else if(x->x_nleft == 0){ // reached target, update!
                x->x_last_target = x->x_target;
                x->x_inc = 0;
                if(x->x_n_lines > 0) // there's more, retarget to the next
                    envgen_retarget(x, 0);
                else if(!x->x_release) // there's no release, we're done.
                    clock_delay(x->x_clock, 0);
            }
        }
        lastin = f;
    }
    x->x_lastin = lastin;
    return(w+5);
}

static void envgen_dsp(t_envgen *x, t_signal **sp){
    dsp_add(envgen_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void envgen_free(t_envgen *x){
    if(x->x_lines != x->x_n_lineini)
        freebytes(x->x_lines, MAX_SEGS*sizeof(*x->x_lines));
    if(x->x_clock)
        clock_free(x->x_clock);
    if(x->x_sus_clock)
        clock_free(x->x_sus_clock);
}

static void envgen_pause(t_envgen *x){
    x->x_pause = 1;
}

static void envgen_resume(t_envgen *x){
    x->x_pause = 0;
}

static void *envgen_new(t_symbol *s, int ac, t_atom *av){
    t_symbol *cursym = s; // avoid warning
    t_envgen *x = (t_envgen *)pd_new(envgen_class);
    x->x_gain = x->x_power = 1;
    x->x_lastin = x->x_maxsustain = x->x_retrigger = x->x_value = x->x_last_target = 0;
    x->x_nleft = x->x_n_lines = x->x_line_n = x->x_pause = x->x_release = 0;
    x->x_suspoint = x->x_legato = 0;
    x->x_lines = x->x_n_lineini;
    x->x_curseg = 0;
    t_atom at[2];
    int i;
    for(i = 0; i < MAX_SEGS; i++) // set exponential list to linear
        SETFLOAT(x->x_at_exp+i, 1);
    SETFLOAT(at, 0);
    SETFLOAT(at+1, 0);
    x->x_av = getbytes(2*sizeof(*(x->x_av)));
    copy_atoms(at, x->x_av, x->x_ac = 2);
    int symarg = 0, n = 0;
    while(ac > 0){
        if((av+n)->a_type == A_FLOAT && !symarg)
            n++, ac--;
        else if((av+n)->a_type == A_SYMBOL){
            if(!symarg){
                symarg = 1;
                if(n == 1 && av->a_type == A_FLOAT)
                    x->x_value = atom_getfloatarg(0, ac, av);
                else if(n > 1){
                    x->x_ac = n;
                    x->x_av = getbytes(n * sizeof(*(x->x_av)));
                    copy_atoms(av, x->x_av, x->x_ac);
                }
                av += n;
                n = 0;
            }
            cursym = atom_getsymbolarg(0, ac, av);
            if(cursym == gensym("-init")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_value = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-exp")){
                if(ac >= 2){
                    ac--, av++;
                    i = 0;
                    while(ac || av->a_type == A_FLOAT){
                        SETFLOAT(x->x_at_exp+i, av->a_w.w_float);
                        ac--, av++, i++;
                    }
                }
                else goto errstate;
            }
            else if(cursym == gensym("-retrigger")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_retrigger = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-suspoint")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    int suspoint = (int)atom_getfloatarg(1, ac, av);
                    x->x_suspoint = suspoint < 0 ? 0 : suspoint;
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-maxsustain")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_maxsustain = (int)atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-legato"))
                x->x_legato = 1, ac--, av++;
            else goto errstate;
        }
        else goto errstate;
    }
    if(!symarg && n > 0){
        if(n == 1)
            x->x_value = atom_getfloatarg(0, n, av);
        else{
            x->x_av = getbytes(n*sizeof(*(x->x_av)));
            copy_atoms(av, x->x_av, x->x_ac = n);
        }
    }
    x->x_last_target = x->x_value;
    envgen_proxy_init(&x->x_proxy, x);
    inlet_new(&x->x_obj, &x->x_proxy.p_pd, 0, 0);
    outlet_new((t_object *)x, &s_signal);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    x->x_clock = clock_new(x, (t_method)envgen_tick);
    x->x_sus_clock = clock_new(x, (t_method)envgen_sus_tick);
    return(x);
errstate:
    pd_error(x, "[envgen~]: improper args");
    return NULL;
}

void envgen_tilde_setup(void){
    envgen_class = class_new(gensym("envgen~"), (t_newmethod)envgen_new,
            (t_method)envgen_free, sizeof(t_envgen), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(envgen_class, nullfn, gensym("signal"), 0);
    class_addmethod(envgen_class, (t_method)envgen_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(envgen_class, envgen_float);
    class_addlist(envgen_class, envgen_list);
    class_addbang(envgen_class, envgen_bang);
    class_addmethod(envgen_class, (t_method)envgen_setgain, gensym("setgain"), A_FLOAT, 0);
    class_addmethod(envgen_class, (t_method)envgen_set, gensym("set"), A_GIMME, 0);
    class_addmethod(envgen_class, (t_method)envgen_exp, gensym("exp"), A_GIMME, 0);
    class_addmethod(envgen_class, (t_method)envgen_bang, gensym("attack"), 0);
    class_addmethod(envgen_class, (t_method)envgen_rel, gensym("release"), 0);
    class_addmethod(envgen_class, (t_method)envgen_pause, gensym("pause"), 0);
    class_addmethod(envgen_class, (t_method)envgen_resume, gensym("resume"), 0);
    class_addmethod(envgen_class, (t_method)envgen_legato, gensym("legato"), A_FLOAT, 0);
    class_addmethod(envgen_class, (t_method)envgen_suspoint, gensym("suspoint"), A_FLOAT, 0);
    class_addmethod(envgen_class, (t_method)envgen_maxsustain, gensym("maxsustain"), A_FLOAT, 0);
    class_addmethod(envgen_class, (t_method)envgen_retrigger, gensym("retrigger"), A_FLOAT, 0);
    envgen_proxy = (t_class *)class_new(gensym("envgen proxy"), 0, 0, sizeof(t_proxy), 0, 0);
    class_addanything(envgen_proxy, envgen_proxy_list);
}
