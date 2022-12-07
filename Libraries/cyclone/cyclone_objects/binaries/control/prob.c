/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdio.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/file.h"
#include "control/rand.h"

/* CHECKED: no preallocation.  It looks like if new state-entries
   were added to the list's head, and new transition-entries
   were added to the sublist's head.  No sorting of any kind. */

typedef struct _probtrans{
    int  tr_value;  /* state (if a header trans), or suffix value (otherwise) */
    int  tr_count;  /* (a total in case of a header trans) */
    struct _probtrans  *tr_suffix;     /* header trans of a suffix state */
    struct _probtrans  *tr_nexttrans;  /* next trans of this state */
    struct _probtrans  *tr_nextstate;  /* header trans of a next state */
}t_probtrans;

typedef struct _prob{
    t_object       x_ob;
    t_probtrans   *x_translist;
    t_probtrans   *x_state;
    t_probtrans   *x_default;
    int            x_embedmode;
    unsigned int   x_seed;
    t_outlet      *x_bangout;
    t_file  *x_filehandle;
}t_prob;

static t_class *prob_class;

static t_probtrans *prob_findstate(t_prob *x, int value, int complain){
    t_probtrans *state;
    for(state = x->x_translist; state; state = state->tr_nextstate)
        if(state->tr_value == value)
            break;
    if(!state && complain)
        pd_error(x, "[prob]: no state %d", value);  /* CHECKED */
    return(state);
}

static void prob_update(t_prob *x){
    t_probtrans *state;
    char buf[64];
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
    for(state = x->x_translist; state; state = state->tr_nextstate){
        t_probtrans *trans;
        for(trans = state->tr_nexttrans; trans; trans = trans->tr_nexttrans){
            sprintf(buf, "%d %d %d\n", state->tr_value, trans->tr_value, trans->tr_count);
            editor_append(x->x_filehandle, buf);
        }
    }
}

static void prob_click(t_prob *x, t_floatarg xpos, t_floatarg ypos,
t_floatarg shift, t_floatarg ctrl, t_floatarg alt){ // CHECKED not available, LATER full editing
    xpos = ypos = shift = ctrl = alt = 0;
    t_probtrans *state;
    char buf[64];
    editor_open(x->x_filehandle, 0, 0);
    for(state = x->x_translist; state; state = state->tr_nextstate){
        t_probtrans *trans;
        for(trans = state->tr_nexttrans; trans; trans = trans->tr_nexttrans){
            sprintf(buf, "%d %d %d\n", state->tr_value, trans->tr_value, trans->tr_count);
            editor_append(x->x_filehandle, buf);
        }
    }
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  wm deiconify .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  raise .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  focus .%lx.text\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
}

static void prob_embedhook(t_pd *z, t_binbuf *bb, t_symbol *bindsym){
    t_prob *x = (t_prob *)z;
    if(x->x_embedmode){
        t_probtrans *pfx, *sfx;
        for(pfx = x->x_translist; pfx; pfx = pfx->tr_nextstate)
            for(sfx = pfx->tr_nexttrans; sfx; sfx = sfx->tr_nexttrans)
                binbuf_addv(bb, "siii;", bindsym, pfx->tr_value, sfx->tr_value, sfx->tr_count);
        binbuf_addv(bb, "ssi;", bindsym, gensym("embed"), 1);
        if(x->x_default)
            binbuf_addv(bb, "ssi;", bindsym, gensym("reset"),
        x->x_default->tr_value);
    };
    obj_saveformat((t_object *)x, bb);
}

static void prob_embed(t_prob *x, t_floatarg f){
    x->x_embedmode = ((int)f != 0);
}

static void prob_reset(t_prob *x, t_floatarg f){
    int value = (int)f;  /* CHECKED: float converted to int */
    t_probtrans *state = prob_findstate(x, value, 1);
    if(state){ /* CHECKED */
        x->x_default = state;
        /* CHECKED (sort of): */
        if(!x->x_state->tr_nexttrans)
            x->x_state = state;
    }
}

static void prob_clear(t_prob *x){
    t_probtrans *state, *nextstate;
    for(state = x->x_translist; state; state = nextstate){
        t_probtrans *trans, *nexttrans;
        for(trans = state->tr_nexttrans; trans; trans = nexttrans){
            nexttrans = trans->tr_nexttrans;
            freebytes(trans, sizeof(*trans));
        }
        nextstate = state->tr_nextstate;
        freebytes(state, sizeof(*state));
    }
    x->x_translist = 0;
    x->x_state = 0;
    x->x_default = 0;  // CHECKED: default number is not kept
    prob_update(x);
}

static void prob_list(t_prob *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int prefval, suffval, count;
    if (ac == 3 && av->a_type == A_FLOAT
    && av[1].a_type == A_FLOAT && av[2].a_type == A_FLOAT
    && (prefval = (int)av->a_w.w_float) == av->a_w.w_float
    && (suffval = (int)av[1].a_w.w_float) == av[1].a_w.w_float
    && (count = (int)av[2].a_w.w_float) == av[2].a_w.w_float){
        t_probtrans *prefix = prob_findstate(x, prefval, 0);
        t_probtrans *suffix = prob_findstate(x, suffval, 0);
        t_probtrans *trans;
        if(prefix && suffix){
            for(trans = prefix->tr_nexttrans; trans;
                 trans = trans->tr_nexttrans)
            if(trans->tr_suffix == suffix)
                break;
            if(trans){ // transition already exists
                prefix->tr_count += (count - trans->tr_count);
                trans->tr_count = count;
                return;
            }
        }
        if(!prefix){
            if(!(prefix = getbytes(sizeof(*prefix))))
                return;
            prefix->tr_value = prefval;
            prefix->tr_count = 0;
            prefix->tr_suffix = 0;
            prefix->tr_nexttrans = 0;
            prefix->tr_nextstate = x->x_translist;
            x->x_translist = prefix;
            if(suffval == prefval)
                suffix = prefix;
        }
        if(!suffix){
            if(!(suffix = getbytes(sizeof(*suffix))))
                return;
            suffix->tr_value = suffval;
            suffix->tr_count = 0;
            suffix->tr_suffix = 0;
            suffix->tr_nexttrans = 0;
            suffix->tr_nextstate = x->x_translist;
            x->x_translist = suffix;
        }
        if((trans = getbytes(sizeof(*trans)))){
            trans->tr_value = suffval;
            trans->tr_count = count;
            trans->tr_suffix = suffix;
            trans->tr_nexttrans = prefix->tr_nexttrans;
            trans->tr_nextstate = prefix->tr_nextstate;
            prefix->tr_count += count;
            prefix->tr_nexttrans = trans;
        }
        if(!x->x_state)  /* CHECKED */
            x->x_state = prefix;  /* CHECKED */
        prob_update(x);
    }
    else
        pd_error(x, "[prob]: bad list message format");  /* CHECKED */
}

static void prob_dump(t_prob *x){ // CHECKED
    t_probtrans *state;
    post("transition probabilities:");
    for(state = x->x_translist; state; state = state->tr_nextstate){
        t_probtrans *trans;
        for(trans = state->tr_nexttrans; trans; trans = trans->tr_nexttrans)
            post(" from %3d to %3d: %d", state->tr_value, trans->tr_value, trans->tr_count);
        post("total weights for state %d: %d", state->tr_value, state->tr_count);
    }
}

static void prob_bang(t_prob *x){
    if(x->x_state){  // no output after clear
        int rnd = rand_int(&x->x_seed, x->x_state->tr_count);
        t_probtrans *trans = x->x_state->tr_nexttrans;
        if(trans){
            for(trans = x->x_state->tr_nexttrans; trans; trans = trans->tr_nexttrans)
                if((rnd -= trans->tr_count) < 0)
                    break;
            if(trans){
                t_probtrans *nextstate = trans->tr_suffix;
                if(nextstate){
                    outlet_float(((t_object *)x)->ob_outlet, nextstate->tr_value);
                    x->x_state = nextstate;
                }
                else
                    pd_error(x, "[prob] bug; prob_bang: void suffix");
            }
            else
                pd_error(x, "[prob] bug; prob_bang: search overflow");
        }
        else{
            outlet_bang(x->x_bangout);
            if(x->x_default)  // CHECKED: stays at dead-end if no default
                x->x_state = x->x_default;
        }
    }
}

static void prob_float(t_prob *x, t_float f){
    int value = (int)f;
    if(f == value){ // CHECKED
        t_probtrans *state = prob_findstate(x, value, 1);
        if(state)  // CHECKED
            x->x_state = state;
    }
    else
        pd_error(x, "[prob]: doesn't understand \"noninteger float\"");
}

static void prob_free(t_prob *x){
    prob_clear(x);
    file_free(x->x_filehandle);
}

static void *prob_new(void){
    t_prob *x = (t_prob *)pd_new(prob_class);
    x->x_translist = 0;
    x->x_state = 0;
    x->x_default = 0;
    x->x_embedmode = 0;  // CHECKED
    rand_seed(&x->x_seed, 0);
    outlet_new((t_object *)x, &s_float);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    x->x_filehandle = file_new((t_pd *)x, prob_embedhook, 0, 0, 0);
    return (x);
}

CYCLONE_OBJ_API void prob_setup(void){
    prob_class = class_new(gensym("prob"), (t_newmethod)prob_new,
        (t_method)prob_free, sizeof(t_prob), 0, 0);
    class_addbang(prob_class, prob_bang);
    class_addfloat(prob_class, prob_float);
    class_addlist(prob_class, prob_list);
    class_addmethod(prob_class, (t_method)prob_embed, gensym("embed"), A_FLOAT, 0);
    class_addmethod(prob_class, (t_method)prob_reset, gensym("reset"), A_FLOAT, 0);
    class_addmethod(prob_class, (t_method)prob_clear, gensym("clear"), 0);
    class_addmethod(prob_class, (t_method)prob_dump, gensym("dump"), 0);
    class_addmethod(prob_class, (t_method)prob_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    file_setup(prob_class, 1);
}
