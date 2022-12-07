/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* The first version of this code was written by Olaf Matthes.
   It was entirely reimplemented in the hope of adapting it to
   cyclone's guidelines. */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define BORAX_MAXVOICES  128  /* CHECKME */

typedef struct _Borax_voice
{
    int     v_index;  /* free iff zero */
    double  v_onset;
    int     v_nonsets;
} t_Borax_voice;

typedef struct _Borax
{
    t_object        x_ob;
    int             x_vel;  /* CHECKME t_float controlled with floatinlet
			       (CHECKME the same in flush) */
    double          x_onset;
    int             x_nonsets;
    int             x_ndurs;
    int             x_ndtimes;
    int             x_minindex;
    int             x_indices[BORAX_MAXVOICES];  /* 0 (free) or 1 (used) */
    int             x_nvoices;
    t_Borax_voice   x_voices[BORAX_MAXVOICES];
    t_outlet  *x_voiceout;
    t_outlet  *x_nvoicesout;
    t_outlet  *x_pitchout;
    t_outlet  *x_velout;
    t_outlet  *x_ndursout;
    t_outlet  *x_durout;
    t_outlet  *x_ndtimesout;
    t_outlet  *x_dtimeout;
} t_Borax;

static t_class *Borax_class;

static void Borax_delta(t_Borax *x)
{
    /* CHECKME first note */
    float dtime = clock_gettimesince(x->x_onset);  /* CHECKME */
    outlet_float(x->x_dtimeout, dtime);
    outlet_float(x->x_ndtimesout, ++x->x_ndtimes);  /* CHECKME */
}

static void Borax_durout(t_Borax *x, int pitch)
{
    float dur = clock_gettimesince(x->x_voices[pitch].v_onset);  /* CHECKME */
    outlet_float(x->x_durout, dur);
    outlet_float(x->x_ndursout, ++x->x_ndurs);  /* CHECKME */
}

static void Borax_float(t_Borax *x, t_float f){
    int pitch = (int)f;
    if((f - (t_float)pitch) == 0.){
        int index;
        if (pitch < 0 || pitch >= BORAX_MAXVOICES){
            /* CHECKME pitch range, complaints */
            return;
        }
        index = x->x_voices[pitch].v_index;
        if (x->x_vel){
            if (index)
                return;  /* CHECKME */
            x->x_indices[index = x->x_minindex] = 1;
            while (x->x_indices[++x->x_minindex]);
            index++;  /* CHECKME one-based? */
            Borax_delta(x);
            x->x_onset = clock_getlogicaltime();  /* CHECKME (in delta?) */
            x->x_voices[pitch].v_index = index;
            x->x_voices[pitch].v_onset = x->x_onset;
            x->x_voices[pitch].v_nonsets = ++x->x_nonsets;
            x->x_nvoices++;
        }
        else{
            if (!index)
                return;  /* CHECKME */
            index--;
            x->x_indices[index] = 0;
            if (index < x->x_minindex) x->x_minindex = index;
            index++;
            Borax_durout(x, pitch);
            x->x_voices[pitch].v_index = 0;
            x->x_nvoices--;
        }
        outlet_float(x->x_velout, x->x_vel);
        outlet_float(x->x_pitchout, pitch);
        outlet_float(x->x_nvoicesout, x->x_nvoices);
        outlet_float(x->x_voiceout, index);
        outlet_float(((t_object *)x)->ob_outlet, x->x_voices[pitch].v_nonsets);
    }
}

static void Borax_ft1(t_Borax *x, t_floatarg f){
    x->x_vel = (int)f;  /* CHECKME */
}

static void Borax_reset(t_Borax *x){
    x->x_vel = 0;
    x->x_onset = clock_getlogicaltime();
    x->x_nonsets = x->x_ndurs = x->x_ndtimes = 0;
    x->x_minindex = 0;
    memset(x->x_indices, 0, sizeof(x->x_indices));
    x->x_nvoices = 0;
    memset(x->x_voices, 0, sizeof(x->x_voices));
}

static void Borax_bang2(t_Borax *x){
    int pitch;
    for(pitch = 0; pitch < BORAX_MAXVOICES; pitch++){
        if (x->x_voices[pitch].v_index){
            /* CHECKME counters, etc. */
            Borax_durout(x, pitch);
            outlet_float(x->x_velout, 0);
            outlet_float(x->x_pitchout, pitch);
            outlet_float(x->x_nvoicesout, --x->x_nvoices);
            outlet_float(x->x_voiceout, x->x_voices[pitch].v_index);
            outlet_float(((t_object *)x)->ob_outlet, x->x_voices[pitch].v_nonsets);
        }
    }
    Borax_reset(x);
}

/* CHECKME flush in a destructor */

static void *Borax_new(void){
    t_Borax *x = (t_Borax *)pd_new(Borax_class);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_bang, gensym("bang2"));
    outlet_new((t_object *)x, &s_float);
    x->x_voiceout = outlet_new((t_object *)x, &s_float);
    x->x_nvoicesout = outlet_new((t_object *)x, &s_float);
    x->x_pitchout = outlet_new((t_object *)x, &s_float);
    x->x_velout = outlet_new((t_object *)x, &s_float);
    x->x_ndursout = outlet_new((t_object *)x, &s_float);
    x->x_durout = outlet_new((t_object *)x, &s_float);
    x->x_ndtimesout = outlet_new((t_object *)x, &s_float);
    x->x_dtimeout = outlet_new((t_object *)x, &s_float);
    Borax_reset(x);
    return(x);
}

CYCLONE_OBJ_API void borax_setup(void){
    Borax_class = class_new(gensym("borax"), (t_newmethod)Borax_new, 0, sizeof(t_Borax), 0, 0);
    class_addfloat(Borax_class, Borax_float);
    /* CHECKME list unfolding */
    class_addmethod(Borax_class, (t_method)Borax_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(Borax_class, (t_method)Borax_bang2, gensym("bang2"), 0);
    class_addmethod(Borax_class, (t_method)Borax_delta, gensym("delta"), 0);
}

CYCLONE_OBJ_API void Borax_setup(void){
    Borax_class = class_new(gensym("Borax"), (t_newmethod)Borax_new, 0, sizeof(t_Borax), 0, 0);
    class_addfloat(Borax_class, Borax_float);
    /* CHECKME list unfolding */
    class_addmethod(Borax_class, (t_method)Borax_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(Borax_class, (t_method)Borax_bang2, gensym("bang2"), 0);
    class_addmethod(Borax_class, (t_method)Borax_delta, gensym("delta"), 0);
    pd_error(Borax_class, "Cyclone: please use [borax] instead of [Borax] to suppress this error");
    class_sethelpsymbol(Borax_class, gensym("borax"));
}
