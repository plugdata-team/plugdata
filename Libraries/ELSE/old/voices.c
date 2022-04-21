// porres 2018-2019

#include "m_pd.h"
#include <string.h>

static t_class *voices_class;

typedef struct voice{
    t_clock        *v_clock;
    t_float         v_pitch;
    int             v_used;
    int             v_released;
    unsigned long   v_count;
}t_voice;

typedef struct voices{
    t_object        x_obj;
    t_voice        *x_vec;
    t_outlet      **x_outs;
    t_outlet       *x_extra;
    t_clock        *x_clock; // clock to check voices
    unsigned long   x_count;
    int             x_n;
    int             x_retrig;
    int             x_steal;
    int             x_list_mode;
    float           x_release;
    float           x_offset;
    float           x_vel;
}t_voices;

static void voices_tick(t_voices *x){
    if(x->x_count != 0){ // check if voices are unused, if so: reset counter
        int used = 0;
        int i;
        t_voice *v;
        for(v = x->x_vec, i = 0; i < x->x_n; v++, i++){ // search used voices
            if(v->v_used){
                used = 1;
                break;
            }
        }
        if(!used){ // all are unused, reset counter
            for(v = x->x_vec, i = x->x_n; i--; v++)
                v->v_count = 0;
            x->x_count = 0;
        }
    }
}

static void voice_tick(t_voice *v_n){ //    post("free voice");
    v_n->v_used = v_n->v_released = v_n->v_pitch = 0.;
}

static void voices_noteon(t_voices *x, t_float f){
    t_voice *v, *first_used = 0, *first_unused = 0;
    unsigned int used_idx = 0, unused_idx = 0;
    int i;
    unsigned int count_used = 0xffffffff, count_unused = 0xffffffff;
// find first_used (on) / first_unused (off)
    for(v = x->x_vec, i = 0; i < x->x_n; v++, i++){
        if(v->v_used && v->v_count < count_used) // find first used (on) voice
            first_used = v, count_used = (unsigned int)v->v_count, used_idx = i;
        else if(!v->v_used && v->v_count < count_unused) // find first unused (off) voice
            first_unused = v, count_unused = (unsigned int)v->v_count, unused_idx = i;
    }
    if(first_unused){ // if there's an unused voice, use it
        first_unused->v_used = 1; // mark as used
        first_unused->v_pitch = f; // set pitch
        first_unused->v_count = x->x_count++; // increase counter
        if(x->x_list_mode){
            t_atom at[3];
            SETFLOAT(at, unused_idx + x->x_offset);             // voice number
            SETFLOAT(at+1, first_unused->v_pitch);              // pitch
            SETFLOAT(at+2, x->x_vel);                           // Note-On velocity
            outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
        }
        else{
            t_atom at[2];
            SETFLOAT(at, first_unused->v_pitch);                // pitch
            SETFLOAT(at+1, x->x_vel);                           // Note-On velocity
            outlet_list(x->x_outs[unused_idx], &s_list, 2, at);
        }
    }
    else{ // there's no unused voice / all voices are being used
        if(x->x_steal){ // if "steal", steal first used voice
            // "free" first used (send note off)
            // at1 / at2 ?
            if(x->x_list_mode){
                 t_atom at1[3];
                 SETFLOAT(at1, used_idx + x->x_offset);           // voice number
                 SETFLOAT(at1+1, first_used->v_pitch);            // pitch
                 SETFLOAT(at1+2, 0);                              // Note-Off
                 outlet_list(x->x_obj.ob_outlet, &s_list, 3, at1);
                 // note on
                 t_atom at2[3];
                 SETFLOAT(at2, used_idx + x->x_offset);           // voice number
                 SETFLOAT(at2+1, f);                              // pitch
                 SETFLOAT(at2+2, x->x_vel);                       // Note-On velocity
                 outlet_list(x->x_obj.ob_outlet, &s_list, 3, at2);
            }
            else{
                t_atom at1[2];
                SETFLOAT(at1, first_used->v_pitch);               // pitch
                SETFLOAT(at1+1, 0);                               // Note-Off
                outlet_list(x->x_outs[used_idx], &s_list, 2, at1);
                // note on
                t_atom at2[2];
                SETFLOAT(at2, f);                                 // pitch
                SETFLOAT(at2+1, x->x_vel);                        // Note-On velocity
                outlet_list(x->x_outs[used_idx], &s_list, 2, at2);
                
            }
            first_used->v_pitch = f; // set new pitch
            first_used->v_count = x->x_count++; // increase counter
        }
        else{ // don't steal, output in extra outlet
            t_atom at[2];
            SETFLOAT(at, f);                                      // pitch
            SETFLOAT(at+1, x->x_vel);                             // Note-On velocity
            outlet_list(x->x_extra, &s_list, 2, at);
        }
    }
}

static void voices_float(t_voices *x, t_float f){
    int i;
    t_voice *v;
    if(x->x_vel > 0){ // Note-on
        if(x->x_retrig == 2){ // retrigger mode 2: different output
            voices_noteon(x, f); // add new note, nothing different
        }
        else{ // retrigger mode 0 & 1
            t_voice *prev = 0;
            unsigned int prev_idx = 0;
            for(v = x->x_vec, i = 0; i < x->x_n; v++, i++){
                if(v->v_used && v->v_pitch == f){ // find previous pitch
                    prev = v, prev_idx = i;
                    break;
                }
            }
            if(prev){ // note already in voice allocation
                if(x->x_retrig == 1){ // retrigger
                    if(x->x_list_mode){
                        t_atom at[3];
                        SETFLOAT(at, prev_idx + x->x_offset);       // voice number
                        SETFLOAT(at+1, f);                          // pitch
                        SETFLOAT(at+2, x->x_vel);                   // Note-On velocity
                        outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
                    }
                    else{
                        t_atom at[2];
                        SETFLOAT(at, f);                           // pitch
                        SETFLOAT(at+1, x->x_vel);                  // Note-On velocity
                        outlet_list(x->x_outs[prev_idx], &s_list, 2, at);
                    }
                }
                else if(x->x_retrig == 0){ // extra
                    t_atom at[2];
                    SETFLOAT(at, f);                                  // pitch
                    SETFLOAT(at+1, x->x_vel);                         // Note-On
                    outlet_list(x->x_extra, &s_list, 2, at);
                }
            }
            else // new note (not in voice allocation)
                voices_noteon(x, f);
        }
    }
    else{ // Note off (x->x_vel = 0)
        t_voice *used_pitch = 0;
        unsigned int used_idx = 0, count = 0xffffffff;
        for(v = x->x_vec, i = 0; i < x->x_n; v++, i++){ // search pitch in oldest entry
            if(v->v_used && !v->v_released && v->v_pitch == f && v->v_count < count)
                used_pitch = v, count = (unsigned int)v->v_count, used_idx = i;
        }
        if(used_pitch){ // pitch was found in a used and unreleased voice
            // send note-off
            if(x->x_list_mode){
                t_atom at[3];
                SETFLOAT(at, used_idx + x->x_offset);             // voice number
                SETFLOAT(at+1, used_pitch->v_pitch);              // pitch
                SETFLOAT(at+2, 0);                                // Note-Off
                outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
            }
            else{
                t_atom at[2];
                SETFLOAT(at, used_pitch->v_pitch);                // pitch
                SETFLOAT(at+1, 0);                                // Note-Off
                outlet_list(x->x_outs[used_idx], &s_list, 2, at);
            }
            // free voice
            if(x->x_release > 0){
                clock_delay(used_pitch->v_clock, x->x_release);
                clock_delay(x->x_clock, x->x_release);
                used_pitch->v_released = 1;
            }
            else{
                used_pitch->v_used = used_pitch->v_pitch = 0;
                if(x->x_count != 0){ // check if all are unused, if so: reset counter
                    int used = 0;
                    for(v = x->x_vec, i = 0; i < x->x_n; v++, i++){ // search used voices
                        if(v->v_used){
                            used = 1;
                            break;
                        }
                    }
                    if(!used){ // all are unused, reset counter
                        for(v = x->x_vec, i = x->x_n; i--; v++)
                            v->v_count = 0;
                        x->x_count = 0;
                    }
                }
            }
        }
        else{ // pitch not found, send note-off in extra outlet
            t_atom at[2];
            SETFLOAT(at, f);                                  // pitch
            SETFLOAT(at+1, 0);                                // Note-Off
            outlet_list(x->x_extra, &s_list, 2, at);
        }
    }
}

static void voices_offset(t_voices *x, t_float f){
    if(x->x_list_mode)
        x->x_offset = (int)f;
    else
        post("[voices]: 'offset' is not pertinent when not in list mode");
}

static void voices_steal(t_voices *x, t_float f){
    x->x_steal = (f != 0);
}

static void voices_release(t_voices *x, t_float f){
    x->x_release = f;
    if(x->x_release < 0)
        x->x_release = 0;
}

static void voices_retrig(t_voices *x, t_float f){
    x->x_retrig = (int)f;
    if(x->x_retrig < 0)
        x->x_retrig = 0;
    if(x->x_retrig > 2)
        x->x_retrig = 2;
}

static void voices_flush(t_voices *x){
    t_voice *v;
    int i;
    for(i = 0, v = x->x_vec; i < x->x_n; i++, v++){
        if(v->v_used){
            if(x->x_list_mode){
                 t_atom at[3];
                 SETFLOAT(at, i + x->x_offset);                  // voice number
                 SETFLOAT(at+1, v->v_pitch);                     // pitch
                 SETFLOAT(at+2, 0);                              // Note-Off
                 outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
            }
            else{
                t_atom at[2];
                SETFLOAT(at, v->v_pitch);                       // pitch
                SETFLOAT(at+1, 0);                              // Note-Off
                outlet_list(x->x_outs[i], &s_list, 2, at);
            }
            if(x->x_release > 0){
                clock_delay(v->v_clock, x->x_release);
                clock_delay(x->x_clock, x->x_release);
                v->v_released = 1;
            }
            else
                v->v_used = v->v_count = v->v_pitch = 0;
        }
    }
    x->x_count = 0;
}

static void voices_voices(t_voices *x, t_float f){
    if(x->x_list_mode){
        t_voice *v;
        int n = (int)f < 1 ? 1 : (int)f;
        if(n == x->x_n)
            return;
        voices_flush(x);
        if(x->x_vec)
            freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
        x->x_vec = (t_voice *)getbytes(n * sizeof(*x->x_vec));
        x->x_n = n;
        int i;
        for(v = x->x_vec, i = n; i--; v++){ // initialize voices
            v->v_pitch = v->v_used = v->v_count = 0;
            v->v_clock = clock_new(v, (t_method)voice_tick);
        }
    }
    else
        post("[voices]: 'voices' is not pertinent when not in list mode");
}

static void voices_clear(t_voices *x){
    t_voice *v;
    int i;
    for(v = x->x_vec, i = x->x_n; i--; v++)
        v->v_pitch = v->v_used = v->v_released = v->v_count = 0; // zero voices
    x->x_count = 0;
}

static void voices_free(t_voices *x){
    t_voice *v;
    int i;
    for(v = x->x_vec, i = x->x_n; i--; v++){
        if(v->v_clock) // needed ???
            clock_free(v->v_clock);
    }
    clock_free(x->x_clock);
    freebytes(x->x_vec, x->x_n * sizeof (*x->x_vec));
    if(x->x_outs)
        freebytes(x->x_outs, x->x_n * sizeof(*x->x_outs));
}

static void *voices_new(t_symbol *s, int argc, t_atom *argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_voices *x = (t_voices *)pd_new(voices_class);
    t_voice *v;
// default
    x->x_offset = 0;
    x->x_list_mode = 0;
    int retrig = 0;
    int n = 1;
    x->x_steal = 0;
    float release = 0;
/////////////////////////////////////////////////////////////////////////////////
    int symarg = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(cursym->s_name, "-retrig")){
                if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(1, argc, argv);
                    retrig = (int)curfloat;
                    argc -= 2;
                    argv += 2;
                    argnum++;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-rel")){
                if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(1, argc, argv);
                    release = curfloat;
                    argc -= 2;
                    argv += 2;
                    argnum++;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-list")){
                x->x_list_mode = 1;
                argc--;
                argv++;
                argnum++;
                if(argc >= 1 && (argv)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(0, argc, argv);
                    x->x_offset = (int)curfloat;
                    argc--;
                    argv++;
                    argnum++;
                }
            }
            else
                goto errstate;
        }
        else if(argv->a_type == A_FLOAT && !symarg){
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    n = (int)argval;
                    break;
                case 1:
                    x->x_steal = argval != 0;
                    break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        }
        else
            goto errstate;
    };
/////////////////////////////////////////////////////////////////////////////////
    if(release < 0)
        release = 0;
    x->x_release = release;
    if(retrig < 0)
        retrig = 0;
    if(retrig > 2)
        retrig = 2;
    x->x_retrig = retrig;
    if(n < 1)
        n = 1;
    x->x_n = n;
    x->x_vec = (t_voice *)getbytes(n * sizeof(*x->x_vec));
    int i;
    for(v = x->x_vec, i = n; i--; v++){ // initialize voices
        v->v_pitch = v->v_used = v->v_count = 0;
        v->v_clock = clock_new(v, (t_method)voice_tick);
    }
    x->x_clock = clock_new(x, (t_method)voices_tick);
    x->x_vel = x->x_count = 0;
    floatinlet_new(&x->x_obj, &x->x_vel);
    floatinlet_new(&x->x_obj, &x->x_release);
    if(x->x_list_mode)
        outlet_new(&x->x_obj, &s_list);
    else{
        t_outlet **outs;
        if(!(outs = (t_outlet **)getbytes(x->x_n * sizeof(*outs))))
            return(0);
        x->x_outs = outs;
        for(i = 0; i < x->x_n; i++)
            x->x_outs[i] = outlet_new((t_object *)x, &s_list);
    }
    x->x_extra = outlet_new((t_object *)x, &s_list);
    return(x);
errstate:
    pd_error(x, "[voices]: improper args");
    return NULL;
}

void voices_setup(void){
    voices_class = class_new(gensym("voices"), (t_newmethod)voices_new,
            (t_method)voices_free, sizeof(t_voices), 0, A_GIMME, 0);
    class_addfloat(voices_class, voices_float);
    class_addmethod(voices_class, (t_method)voices_offset, gensym("offset"), A_FLOAT, 0);
    class_addmethod(voices_class, (t_method)voices_steal, gensym("steal"), A_FLOAT, 0);
    class_addmethod(voices_class, (t_method)voices_retrig, gensym("retrig"), A_FLOAT, 0);
    class_addmethod(voices_class, (t_method)voices_release, gensym("rel"), A_FLOAT, 0);
    class_addmethod(voices_class, (t_method)voices_voices, gensym("voices"), A_FLOAT, 0);
    class_addmethod(voices_class, (t_method)voices_flush, gensym("flush"), 0);
    class_addmethod(voices_class, (t_method)voices_clear, gensym("clear"), 0);
}
