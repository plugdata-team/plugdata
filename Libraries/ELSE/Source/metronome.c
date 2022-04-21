// porres 2021-2022

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include "g_canvas.h"

#define DEFNTICKS   960
#define MAX_N       10
#define MAX_SIZE    40

typedef struct _metronome{
    t_object    x_obj;
    //    t_int       x_dir;
    t_clock    *x_clock;
    t_symbol   *x_sig;
    t_symbol   *x_s_name;
    char        x_sig_array[MAX_N][MAX_SIZE];
    t_int       x_n_sigs; // complex if > 1
    t_int       x_running;
    t_int       x_first_time;
    t_int       x_pause;
    t_int       x_complex;
    t_int       x_subflag;
    t_int       x_sigchange;
    t_int       x_group;
    t_int       x_n_ticks;
    t_int       x_tickcount;
    t_int       x_subdiv;
    t_int       x_n_subdiv;
    t_int       x_barcount;
    t_int       x_sub_barcount;
    t_int       x_tempocount;
    t_float     x_bpm;
    t_float     x_tempo;
    t_float     x_n_tempo;
    t_float     x_tempo_div;
    t_float     x_beat_length;
    t_outlet   *x_count_out;
    t_outlet   *x_phaseout;
    t_outlet   *x_actual_out;
}t_metronome;

static t_class *metronome_class;

static void metronome_stop(t_metronome *x);

static void output_actual_list(t_metronome *x){
    t_atom at[3];
    SETFLOAT(at, x->x_tempo_div);
    t_float bpm = x->x_bpm / x->x_tempo_div;
    SETFLOAT(at+1, bpm);
    SETFLOAT(at+2, 60000*x->x_n_tempo/bpm);
    outlet_list(x->x_actual_out, &s_list, 3, at);
}

static void output_count_list(t_metronome *x){
    t_atom at[4];
    SETFLOAT(at, x->x_barcount);
    SETFLOAT(at+1, x->x_sub_barcount);
    SETFLOAT(at+2, x->x_tempocount);
    SETFLOAT(at+3, x->x_subdiv);
    outlet_list(x->x_count_out, &s_list, 4, at);
}

static void string2atom(t_atom *ap, char* ch, int clen){
    char *buf = getbytes(sizeof(char)*(clen+1));
    char *endptr[1];
    strncpy(buf, ch, clen);
    buf[clen] = 0;
    t_float ftest = strtod(buf, endptr);
    if(buf+clen == *endptr) // it's a float
        SETFLOAT(ap, ftest);
    else
        SETSYMBOL(ap, gensym(buf));
    freebytes(buf, sizeof(char)*(clen+1));
}
    
static void metronome_div(t_metronome *x, t_atom *av){
    t_float f1, f2;
    if(av->a_type == A_SYMBOL){
        pd_error(x, "[metronome]: wrong time signature symbol");
        return;
    }
    else
        f1 = (int)atom_getfloat(av++);
    if(av->a_type == A_SYMBOL){
        char *s = (char *)atom_getsymbol(av)->s_name;
        int len = strlen(s);
        t_atom *d_av = getbytes(2*sizeof(t_atom));
        if(s[0] == '(' && s[len-1] == ')' && strstr(s, "/")){ // in parenthesis alright and a fraction
            s[len-1] = '\0'; // removing the last ')'
            s++;
            char *d = strstr(s, "/");
            string2atom(d_av, s, d-s);
            s = d+1;
            string2atom(d_av+1, s, strlen(s));
            if(d_av->a_type == A_FLOAT && (d_av+1)->a_type == A_FLOAT)
                f2 = atom_getfloat(d_av) / atom_getfloat(d_av+1);
            else{
                pd_error(x, "[metronome]: wrong time signature symbol");
                return;
            }
        }
        else{
            pd_error(x, "[metronome]: wrong time signature symbol");
            return;
        }
    }
    else
        f2 = atom_getfloat(av);
    if(f1 <= 0 || f2 <= 0){
        pd_error(x, "[metronome]: wrong time signature symbol");
        return;
    }
    t_float div = f1 / f2;
    if(!x->x_group){
        if(f1 == 6)
            x->x_group = 2;
        else if(f1 == 9)
            x->x_group = 3;
        else if(f1 == 12)
            x->x_group = 4;
        else
            x->x_group = (int)f1;
    }
    x->x_n_subdiv = (int)(f1/((t_float)x->x_group));
    x->x_n_tempo = x->x_group;
    x->x_tempo_div = (float)(div * x->x_beat_length/(t_float)x->x_group);
    if(x->x_running || !x->x_pause){
        output_actual_list(x);
        clock_setunit(x->x_clock, x->x_tempo * x->x_tempo_div / x->x_n_ticks, 0);
    }
}

static void metronome_symbol(t_metronome *x, t_symbol *s){
    char *ch = (char*)s->s_name;
    if(strstr(ch, "/")){
        t_atom *av = getbytes(2*sizeof(t_atom));
        char *d = strstr(ch, "/");
        if(d == ch || !strcmp(d, "/"))
            goto error;
        string2atom(av+0, ch, d-ch);
        ch = d+1;
        string2atom(av+1, ch, strlen(ch));
        metronome_div(x, av);
    }
    else{
    error:
        pd_error(x, "[metronome]: wrong time signature symbol");
    }
}
static void metronome_set_beat(t_metronome *x, t_atom *av){
    t_float f1 = atom_getfloat(av++);
    t_float f2 = atom_getfloat(av);
    x->x_beat_length = f2/f1;
}

static void metronome_beat(t_metronome *x, t_symbol *s, int argc, t_atom *argv){
    argc = 0;
    if(argv->a_type == A_FLOAT){
        t_float beat = atom_getfloat(argv);
        if(beat > 0)
            x->x_beat_length = (1/beat);
        else
            pd_error(x, "[metronome]: beat needs to be > 1");
    }
    else{
        s = atom_getsymbol(argv);
        char *ch = (char*)s->s_name;
        if(strstr(ch, "/")){
            t_atom *av = getbytes(2*sizeof(t_atom));
            char *d = strstr(ch, "/");
            if(d == ch || !strcmp(d, "/"))
                goto error;
            string2atom(av+0, ch, d-ch);
            ch = d+1;
            string2atom(av+1, ch, strlen(ch));
            metronome_set_beat(x, av);
        }
        else{
        error:
            pd_error(x, "[metronome]: wrong beat format");
        }
    }
}

static void metronome_tick(t_metronome *x){
    outlet_float(x->x_phaseout, (float)x->x_tickcount / (float)x->x_n_ticks);
    if(x->x_tickcount == 0){
        x->x_subdiv = 1;
//        if(x->x_dir)
            x->x_tempocount++;
//        else
//            x->x_tempocount--;
        if(x->x_n_sigs == 1){ // not complex
            x->x_sub_barcount = 1;
            if(x->x_tempocount > x->x_n_tempo){
                x->x_barcount++;
                x->x_tempocount = 1;
            }
        }
        else{ // complex
            if(x->x_tempocount > x->x_n_tempo){
                if(x->x_first_time)
                    x->x_first_time = 0;
                else
                    x->x_sub_barcount++;
                if(x->x_sub_barcount > x->x_n_sigs){
                    x->x_sub_barcount = 1;
                    x->x_barcount++;
                }
                char buf[MAX_SIZE];
                sprintf(buf,"%s", x->x_sig_array[x->x_sub_barcount-1]);
                x->x_sig = gensym(buf);
                x->x_sigchange = 1;
                x->x_tempocount = 1;
                x->x_group = 0;
            }
        }
/*        else if(x->x_tempocount < 0){
            x->x_tempocount = (x->x_n_tempo - 1);
            x->x_barcount--;
        }*/
        output_count_list(x);
        outlet_bang(x->x_obj.ob_outlet);
//        if(x->x_sigchange && x->x_tempocount == 1){ // change time signature
        if(x->x_sigchange && x->x_sub_barcount && x->x_tempocount == 1){ // change time signature
            x->x_first_time = 0;
            metronome_symbol(x, x->x_sig);
            x->x_sigchange = 0;
        }
        if(x->x_s_name->s_thing){
            t_atom at[2];
            SETFLOAT(at, x->x_tempo_div);
            typedmess(x->x_s_name->s_thing, gensym("beat"), 1, at);
            SETFLOAT(at, x->x_bpm);
            SETSYMBOL(at+1, gensym("permin"));
            typedmess(x->x_s_name->s_thing, gensym("tempo"), 2, at);
            pd_bang(x->x_s_name->s_thing);
        }
    }
    else{
        t_int div = (int)(x->x_tickcount / (x->x_n_ticks / x->x_n_subdiv)) + 1;
        if(div != x->x_subdiv){
            x->x_subdiv = div;
            output_count_list(x);
            if(x->x_subflag)
                outlet_bang(x->x_obj.ob_outlet);
        }
    }
//    if(x->x_dir){
        x->x_tickcount++;
        if(x->x_tickcount == x->x_n_ticks)
            x->x_tickcount = 0;
//    }
/*    else{
        x->x_tickcount--;
        if(x->x_tickcount < 0)
            x->x_tickcount = x->x_n_ticks - 1;
    }*/
    if(x->x_running && !x->x_pause)
        clock_delay(x->x_clock, 1);
}

static void metronome_timesig(t_metronome *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    x->x_n_sigs = 1;
    if(ac > 0){
        x->x_group = 0;
        if(ac >= 3){
            int i = 0;
            if(av->a_type == A_FLOAT){
                pd_error(x, "[metronome]: timesig: invalid syntax");
                return;
            }
            else{
                strcpy(x->x_sig_array[i], (atom_getsymbol(av))->s_name);
                av++;
                ac--;
            }
            while(ac){
                i++;
                if(atom_getsymbol(av) == gensym("+")){
                    av++;
                    ac--;
                }
                else{
                    pd_error(x, "[metronome]: wrong time signature syntax");
                    return;
                }
                if(av->a_type == A_FLOAT){
                    pd_error(x, "[metronome]: wrong time signature syntax");
                    return;
                }
                else{
                    strcpy(x->x_sig_array[i], (atom_getsymbol(av))->s_name);
                    av++;
                    ac--;
                }
            }
            x->x_n_sigs = i + 1;
            x->x_first_time = 1;
            x->x_sub_barcount = 1;
            char buf[MAX_SIZE];
            sprintf(buf,"%s", x->x_sig_array[x->x_sub_barcount-1]);
            x->x_sig = gensym(buf);
            if(x->x_tickcount == 0 && x->x_tempocount == 1){
                metronome_symbol(x, x->x_sig);
                x->x_sigchange = 0;
                x->x_first_time = 0;
            }
            else
                x->x_sigchange = 1;
        }
        else if(ac == 2){
            if((av+1)->a_type == A_SYMBOL){
                pd_error(x, "[metronome]: wrong time signature syntax");
                return;
            }
            else{
                t_int arg = (t_int)(atom_getfloat(av+1));
                if(arg > 0)
                    x->x_group = arg;
                else{
                    pd_error(x, "[metronome]: invalid group number [%d]", (int)arg);
                    return;
                }
            }
            goto firstarg;
        }
        else if(ac == 1){
        firstarg:
            if(av->a_type == A_SYMBOL){
                x->x_sig = atom_getsymbol(av);
                if(x->x_tickcount == 0 && x->x_tempocount == 1){
                    metronome_symbol(x, x->x_sig);
                    x->x_sigchange = 0;
                }
                else
                    x->x_sigchange = 1;
            }
            else{
                pd_error(x, "[metronome]: wrong time signature syntax");
                return;
            }
        }
    }
    else
        pd_error(x, "[metronome]: wrong time signature syntax");
}

static void metronome_tempo(t_metronome *x, t_floatarg tempo){
    if(tempo < 0) // avoid negative tempo for now
        tempo = 0;
    if(tempo != 0){
        x->x_bpm = tempo;
        tempo = 60000./tempo;
        if(tempo < 0)
            tempo *= -1;
        x->x_tempo = tempo;
/*      int dir = (tempo > 0);
        if(dir != x->x_dir){
            x->x_dir = dir;
            if(x->x_dir){
                x->x_tempocount+=2;
                if(x->x_tempocount >= x->x_n_tempo){
                    x->x_tempocount-= x->x_n_tempo;
                    x->x_barcount++;
                }
            }
            else{
                x->x_tempocount-=2;
                if(x->x_tempocount < 0){
                    x->x_tempocount += x->x_n_tempo;
                    x->x_barcount--;
                }
            }
        }*/
        clock_setunit(x->x_clock, x->x_tempo * x->x_tempo_div / x->x_n_ticks, 0);
        if(x->x_running && !x->x_pause){
            metronome_tick(x);
            output_actual_list(x);
            if(x->x_s_name->s_thing){
                t_atom at[2];
                SETFLOAT(at, x->x_bpm);
                SETSYMBOL(at+1, gensym("permin"));
                typedmess(x->x_s_name->s_thing, gensym("tempo"), 2, at);
            }
        }
    }
    else
        clock_unset(x->x_clock);
}

static void metronome_float(t_metronome *x, t_float f){
    f = (f != 0);
    if(x->x_s_name->s_thing)
        pd_float(x->x_s_name->s_thing, f);
    if(f){
        if(x->x_n_sigs > 1){
            char buf[MAX_SIZE];
            sprintf(buf,"%s", x->x_sig_array[0]);
            x->x_group = 0;
            metronome_symbol(x, x->x_sig = gensym(buf));
        }
        x->x_barcount = x->x_sub_barcount = x->x_tempocount = x->x_subdiv = x->x_tickcount = 0;
        output_count_list(x);
        x->x_barcount++;
        x->x_sub_barcount++;
        x->x_running = 1;
        x->x_pause = 0;
        metronome_tick(x);
    }
    else{
        x->x_running = 0;
        x->x_barcount = x->x_sub_barcount = x->x_tempocount = x->x_subdiv = x->x_tickcount = 0;
        outlet_float(x->x_phaseout, 0);
        output_count_list(x);
        clock_unset(x->x_clock);
    }
}

static void metronome_start(t_metronome *x){
    if(x->x_s_name->s_thing){
        typedmess(x->x_s_name->s_thing, gensym("resync"), 0, NULL);
    }
    metronome_float(x, 1);
}

static void metronome_stop(t_metronome *x){
    metronome_float(x, 0);
}

static void metronome_pause(t_metronome *x){
    x->x_pause = 1;
    metronome_tempo(x, 0);
}

static void metronome_continue(t_metronome *x){
    x->x_pause = 0;
    metronome_tempo(x, x->x_bpm);
}

static void metronome_sub(t_metronome *x, t_floatarg f){
    x->x_subflag = (f != 0);
}

static void metronome_free(t_metronome *x){
    clock_free(x->x_clock);
}

/*static void metronome_set(t_metronome *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){
    x->x_barcount = atom_getfloatarg(0, ac, av);
        ac--, av++;
    }
    if(ac){
    x->x_sub_barcount = atom_getfloatarg(0, ac, av);
        ac--, av++;
    }
    if(ac){
    x->x_tempocount = atom_getfloatarg(0, ac, av) - 1;
        ac--, av++;
    }
//    if(ac){
//        t_float f = atom_getfloatarg(0, ac, av);
//        x->x_tickcount = (int)((f > 1 ? 1 : f < 0 ? 0 : f) * x->x_n_ticks);
//        ac--, av++;
//    }
}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
static void *metronome_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_metronome *x = (t_metronome *)pd_new(metronome_class);
    x->x_clock = clock_new(x, (t_method)metronome_tick);
    t_canvas *canvas = canvas_getrootfor(canvas_getcurrent());
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING, "$0-clock-.x%lx.c", (long unsigned int)canvas);
    x->x_s_name = canvas_realizedollar(canvas, gensym(buf));
    x->x_sig = gensym("4/4");
//    x->x_dir = 1;
    x->x_n_ticks = DEFNTICKS;
    x->x_tickcount = 0;
    x->x_tempo_div = 1;
    x->x_beat_length = 4;
    x->x_group = 0;
    x->x_pause = 0;
    x->x_complex = 0;
    x->x_sigchange = 0;
    x->x_subflag = 0;
    x->x_n_tempo = 4;
    x->x_n_subdiv = 1;
    x->x_n_sigs = 1;
    t_float tempo = 120;
    outlet_new(&x->x_obj, gensym("bang"));
    x->x_count_out = outlet_new((t_object *)x, gensym("list"));
    x->x_phaseout = outlet_new((t_object *)x, gensym("float"));
    x->x_actual_out = outlet_new((t_object *)x, gensym("list"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("tempo"));
    int argn = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-name")){
                if(argn)
                    goto errstate;
                ac--, av++;
                if(av->a_type == A_SYMBOL){
                    sym = atom_getsymbolarg(0, ac, av);
                    x->x_s_name = canvas_realizedollar(canvas, sym);
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(sym == gensym("-beat")){
                if(argn)
                    goto errstate;
                ac--, av++;
                metronome_beat(x, NULL, 1, av);
                ac--, av++;
            }
            else if(sym == gensym("-sub")){
                if(argn)
                    goto errstate;
                ac--, av++;
                x->x_subflag = 1;
            }
            else{
                if(ac == 1){
                    t_atom at[1];
                    if(av->a_type == A_SYMBOL){
                        SETSYMBOL(at, atom_getsymbolarg(0, ac, av));
                        ac--, av++;
                        metronome_timesig(x, gensym("timesig"), 1, at);
                    }
                    else
                        goto errstate;
                }
                else{
                    metronome_timesig(x, gensym("timesig"), ac, av);
                    ac = 0;
                }
            }
        }
        else if(av->a_type == A_FLOAT){
            argn = 1;
            tempo = atom_getfloatarg(0, ac, av);
            ac--, av++;
        }
    }
    metronome_tempo(x, tempo);
    metronome_stop(x);
    return(x);
    errstate:
        pd_error(x, "[metronome]: improper args");
        return(NULL);
}

void metronome_setup(void){
    metronome_class = class_new(gensym("metronome"), (t_newmethod)metronome_new,
        (t_method)metronome_free, sizeof(t_metronome), 0, A_GIMME, 0);
    class_addfloat(metronome_class, (t_method)metronome_float);
    class_addbang(metronome_class, (t_method)metronome_start);
    class_addmethod(metronome_class, (t_method)metronome_timesig, gensym("timesig"), A_GIMME, 0);
    class_addmethod(metronome_class, (t_method)metronome_tempo, gensym("tempo"), A_FLOAT, 0);
    class_addmethod(metronome_class, (t_method)metronome_beat, gensym("beat"), A_GIMME, 0);
    class_addmethod(metronome_class, (t_method)metronome_start, gensym("start"), 0);
    class_addmethod(metronome_class, (t_method)metronome_stop, gensym("stop"), 0);
    class_addmethod(metronome_class, (t_method)metronome_pause, gensym("pause"), 0);
    class_addmethod(metronome_class, (t_method)metronome_continue, gensym("continue"), 0);
    class_addmethod(metronome_class, (t_method)metronome_sub, gensym("sub"), A_FLOAT, 0);
//    class_addmethod(metronome_class, (t_method)metronome_set, gensym("set"), A_GIMME, 0);
}
