// based/inspired on MAX's borax

#include <string.h>
#include "m_pd.h"

#define noteinfo_MAXVOICES  128

typedef struct _noteinfo_voice{
    int     v_index;  /* free if zero */
    double  v_onset;
    int     v_nonsets;
}t_noteinfo_voice;

typedef struct _noteinfo{
    t_object            x_obj;
    float               x_vel;
    double              x_onset;
    int                 x_nonsets;
    int                 x_ndurs;
    int                 x_ndtimes;
    int                 x_minindex;
    int                 x_indices[noteinfo_MAXVOICES];  /* 0 (free) or 1 (used) */
    int                 x_nvoices;
    t_noteinfo_voice    x_voices[noteinfo_MAXVOICES];
    t_outlet           *x_pitch_info_out;
    t_outlet           *x_n_voices_out;
}t_noteinfo;

static t_class *noteinfo_class;

static void noteinfo_float(t_noteinfo *x, t_float f){
    int pitch = (int)f;
    if((f - (t_float)pitch) == 0.){
        int index;
        if(pitch < 0 || pitch >= noteinfo_MAXVOICES)
            return;
        index = x->x_voices[pitch].v_index;
        float dtime;
        if(x->x_vel){
            if(index)
                return;  /* CHECKME */
            x->x_indices[index = x->x_minindex] = 1;
            while(x->x_indices[++x->x_minindex]);
            index++;  /* CHECKME one-based? */
            dtime = clock_gettimesince(x->x_onset);  /* CHECKME */
            x->x_onset = clock_getlogicaltime();  /* CHECKME (in delta?) */
            x->x_voices[pitch].v_index = index;
            x->x_voices[pitch].v_onset = x->x_onset;
            x->x_voices[pitch].v_nonsets = ++x->x_nonsets;
            x->x_nvoices++;
        }
        else{
            if(!index)
                return;  /* CHECKME */
            index--;
            x->x_indices[index] = 0;
            if(index < x->x_minindex)
                x->x_minindex = index;
            index++;
            dtime = clock_gettimesince(x->x_voices[pitch].v_onset);  /* CHECKME */
            x->x_voices[pitch].v_index = 0;
            x->x_nvoices--;
        }
        outlet_float(x->x_n_voices_out, x->x_nvoices);
        t_atom at[5];
        SETFLOAT(at, index - 1);
        SETFLOAT(at+1, x->x_voices[pitch].v_nonsets - 1);
        SETFLOAT(at+2, pitch);
        SETFLOAT(at+3, x->x_vel);
        SETFLOAT(at+4, dtime);
        outlet_list(x->x_pitch_info_out, &s_list, 5, at);
    }
}

static void noteinfo_reset(t_noteinfo *x){
    x->x_onset = clock_getlogicaltime();
    x->x_nonsets = x->x_ndurs = x->x_ndtimes = x->x_minindex = 0;
    memset(x->x_indices, 0, sizeof(x->x_indices));
    x->x_nvoices = 0;
    memset(x->x_voices, 0, sizeof(x->x_voices));
}

static void noteinfo_bang(t_noteinfo *x){
    for(int pitch = 0; pitch < noteinfo_MAXVOICES; pitch++){
        if(x->x_voices[pitch].v_index){
            float dur = clock_gettimesince(x->x_voices[pitch].v_onset);
            outlet_float(x->x_n_voices_out, --x->x_nvoices);
            t_atom at[5];
            SETFLOAT(at+0, x->x_voices[pitch].v_index - 1);
            SETFLOAT(at+1, x->x_voices[pitch].v_nonsets - 1);
            SETFLOAT(at+2, pitch);
            SETFLOAT(at+3, 0);
            SETFLOAT(at+4, dur);
            outlet_list(x->x_pitch_info_out, &s_list, 5, at);
        }
    }
    noteinfo_reset(x);
}

static void *noteinfo_new(void){
    t_noteinfo *x = (t_noteinfo *)pd_new(noteinfo_class);
    floatinlet_new(&x->x_obj, &x->x_vel);
    x->x_pitch_info_out = outlet_new((t_object *)x, &s_float);
    x->x_n_voices_out = outlet_new((t_object *)x, &s_float);
    noteinfo_reset(x);
    return(x);
}

void noteinfo_setup(void){
    noteinfo_class = class_new(gensym("noteinfo"), (t_newmethod)noteinfo_new, 0, sizeof(t_noteinfo), 0, 0);
    class_addfloat(noteinfo_class, noteinfo_float);
    class_addbang(noteinfo_class, noteinfo_bang);
}
