/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKED no sharing of data among seq objects having the same creation arg */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
//#include "g_canvas.h"
#include <common/api.h>
#include "common/grow.h"
#include "common/file.h"
#include "control/mifi.h"

/* #ifdef KRZYSZCZ
#define SEQ_DEBUG
#endif */

#define SEQ_INISEQSIZE           256   /* LATER rethink */
#define SEQ_INITEMPOMAPSIZE      128   /* LATER rethink */
#define SEQ_EOM                  255   /* end of message marker, LATER rethink */
#define SEQ_TICKSPERSEC          48
#define SEQ_MINTICKDELAY         1.  /* LATER rethink */
#define SEQ_TICKEPSILON ((double).0001)
#define SEQ_STARTEPSILON         .0001  /* if inside: play unmodified */
#define SEQ_TEMPOEPSILON         .0001  /* if inside: pause */
#define SEQ_ISRUNNING(x) ((x)->x_prevtime > (double).0001)
#define SEQ_ISPAUSED(x) ((x)->x_prevtime <= (double).0001)

enum{SEQ_IDLEMODE, SEQ_RECMODE, SEQ_PLAYMODE, SEQ_SLAVEMODE};

typedef struct _seqevent{
    double         e_delta;
    unsigned char  e_bytes[4];
}t_seqevent;

typedef struct _seqtempo{
    double  t_scoretime;  /* score ticks from start */
    double  t_sr;         /* score ticks per second */
}t_seqtempo;

typedef struct _seq{
    t_object       x_ob;
    t_canvas      *x_canvas;
    t_symbol      *x_defname;
    t_file  *x_filehandle;
    int            x_mode;
    int            x_playhead;
    t_float        x_delay;
    t_float        x_event_delay;
    double         x_nextscoretime;
    float          x_timescale;
    float          x_newtimescale;
    double         x_prevtime;
    double         x_slaveprevtime;
    double         x_clockdelay;
    unsigned char  x_status;
    int            x_evelength;
    int            x_expectedlength;
    int            x_eventreadhead;
    int            x_seqsize;  /* as allocated */
    int            x_nevents;  /* as used */
    t_seqevent    *x_sequence;
    t_seqevent     x_seqini[SEQ_INISEQSIZE];
    int            x_temporeadhead;
    int            x_tempomapsize;  /* as allocated */
    int            x_ntempi;        /* as used */
    t_seqtempo    *x_tempomap;
    t_seqtempo     x_tempomapini[SEQ_INITEMPOMAPSIZE];
    t_clock       *x_clock;
    t_clock       *x_slaveclock;
    t_outlet      *x_bangout;
}t_seq;

static t_class *seq_class;

static void seq_eventstring(t_seq *x, char *buf, t_seqevent *ep, int editable, t_float *sum){
    x = NULL;
    unsigned char *bp = ep->e_bytes;
    int i;
    *sum += ep->e_delta;
    if(editable)
        sprintf(buf, "%g", *sum);
    else if(*bp < 128 || *bp == 247)
        sprintf(buf, "(%g)->", *sum);
    else
        sprintf(buf, "(%g)", *sum);
    buf += strlen(buf);
    sprintf(buf, " %g", (float)*bp);
    for(i = 0, bp++; i < 3 && *bp != SEQ_EOM; i++, bp++){
        buf += strlen(buf);
        sprintf(buf, " %g", (float)*bp);
    }
}

static void seq_update(t_seq *x){
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
    t_seqevent *ep = x->x_sequence;
    int nevents = x->x_nevents;
    char buf[MAXPDSTRING+2];
    t_float sum = 0;
    while(nevents--){  // LATER rethink sysex continuation
        seq_eventstring(x, buf, ep, 1, &sum);
        strcat(buf, ";\n");
        editor_append(x->x_filehandle, buf);
        ep++;
    }
    editor_setdirty(x->x_filehandle, 0);
}

static void seq_doclear(t_seq *x, int dofree){
    if(dofree){
        if(x->x_sequence != x->x_seqini){
            freebytes(x->x_sequence, x->x_seqsize * sizeof(*x->x_sequence));
            x->x_sequence = x->x_seqini;
            x->x_seqsize = SEQ_INISEQSIZE;
        }
        if(x->x_tempomap != x->x_tempomapini){
            freebytes(x->x_tempomap, x->x_tempomapsize * sizeof(*x->x_tempomap));
            x->x_tempomap = x->x_tempomapini;
            x->x_tempomapsize = SEQ_INITEMPOMAPSIZE;
        }
    }
    x->x_nevents = x->x_ntempi = 0;
}

static void seq_clear(t_seq *x){
    seq_doclear(x, 0);
    seq_update(x);
}

static int seq_dogrowing(t_seq *x, int nevents, int ntempi){
    if(nevents > x->x_seqsize){
        int nrequested = nevents;
/* #ifdef SEQ_DEBUG
    loudbug_post("growing for %d events...", nevents);
#endif */
        x->x_sequence = grow_nodata(&nrequested, &x->x_seqsize, x->x_sequence,
        SEQ_INISEQSIZE, x->x_seqini, sizeof(*x->x_sequence));
        if(nrequested < nevents){
            x->x_nevents = 0;
            x->x_ntempi = 0;
            return(0);
        }
    }
    if(ntempi > x->x_tempomapsize){
        int nrequested = ntempi;
/* #ifdef SEQ_DEBUG
    loudbug_post("growing for %d tempi...", ntempi);
#endif */
        x->x_tempomap = grow_nodata(&nrequested, &x->x_tempomapsize, x->x_tempomap,
        SEQ_INITEMPOMAPSIZE, x->x_tempomapini,
        sizeof(*x->x_tempomap));
        if(nrequested < ntempi){
            x->x_ntempi = 0;
            return(0);
        }
    }
    x->x_nevents = nevents;
    x->x_ntempi = ntempi;
    return(1);
}

static void seq_complete(t_seq *x){
    if(x->x_evelength < x->x_expectedlength){ /* CHECKED no warning if no data after status byte requiring data */
        if(x->x_evelength > 1)
            post("seq: truncated midi message");  /* CHECKED */
    /* CHECKED nothing stored */
    }
    else{
        t_seqevent *ep = &x->x_sequence[x->x_nevents];
        ep->e_delta = clock_gettimesince(x->x_prevtime);
        x->x_prevtime = clock_getlogicaltime();
        if(x->x_evelength < 4)
            ep->e_bytes[x->x_evelength] = SEQ_EOM;
        x->x_nevents++;
        if(x->x_nevents >= x->x_seqsize){
            int nexisting = x->x_seqsize;
        /* store-ahead scheme, LATER consider using x_currevent */
            int nrequested = x->x_nevents + 1;
/* #ifdef SEQ_DEBUG
        loudbug_post("growing...");
#endif */
            x->x_sequence = grow_withdata(&nrequested, &nexisting, &x->x_seqsize, x->x_sequence,
                  SEQ_INISEQSIZE, x->x_seqini, sizeof(*x->x_sequence));
            if(nrequested <= x->x_nevents)
                x->x_nevents = 0;
        }
    }
    x->x_evelength = 0;
}

static void seq_checkstatus(t_seq *x, unsigned char c){
    if(x->x_status && x->x_evelength > 1)  /* LATER rethink */
        seq_complete(x);
    if(c < 192)
        x->x_expectedlength = 3;
    else if(c < 224)
        x->x_expectedlength = 2;
    else if(c < 240)
        x->x_expectedlength = 3;
    else if(c < 248){
    /* FIXME */
        x->x_expectedlength = -1;
    }
    else{
        x->x_sequence[x->x_nevents].e_bytes[0] = c;
        x->x_evelength = x->x_expectedlength = 1;
        seq_complete(x);
        return;
    }
    x->x_status = x->x_sequence[x->x_nevents].e_bytes[0] = c;
    x->x_evelength = 1;
}

static void seq_addbyte(t_seq *x, unsigned char c, int docomplete){
    x->x_sequence[x->x_nevents].e_bytes[x->x_evelength++] = c;
    if(x->x_evelength == x->x_expectedlength){
        seq_complete(x);
        if(x->x_status){
            x->x_sequence[x->x_nevents].e_bytes[0] = x->x_status;
            x->x_evelength = 1;
        }
    }
    else if(x->x_evelength == 4){
        if(x->x_status != 240)
            pd_error(x, "bug [seq]: seq_addbyte");
    /* CHECKED sysex is broken into 4-byte packets marked with
       the actual delta time of last byte received in a packet */
        seq_complete(x);
    }
    else if(docomplete)
        seq_complete(x);
}

static void seq_endofsysex(t_seq *x){
    seq_addbyte(x, 247, 1);
    x->x_status = 0;
}

static void seq_stoprecording(t_seq *x){
    if(x->x_status == 240){
        post("seq: incomplete sysex");  /* CHECKED */
        seq_endofsysex(x);  /* CHECKED 247 added implicitly */
    }
    else if(x->x_status)
        seq_complete(x);
    /* CHECKED running status used in recording, but not across recordings */
    x->x_status = 0;
}

static void seq_stopplayback(t_seq *x){
    /* FIXME */
    /* CHECKED "seq: incomplete sysex" at playback stop, 247 added implicitly */
    /* CHECKME resetting controllers, etc. */
    /* CHECKED bang not sent if playback stopped early */
    clock_unset(x->x_clock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
}

static void seq_stopslavery(t_seq *x){
    /* FIXME */
    clock_unset(x->x_clock);
    clock_unset(x->x_slaveclock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
}

static void seq_startrecording(t_seq *x){
    x->x_prevtime = clock_getlogicaltime();
    x->x_status = 0;
    x->x_evelength = 0;
    x->x_expectedlength = -1;  /* LATER rethink */
}

/* CHECKED running status not used in playback */
static void seq_startplayback(t_seq *x, int modechanged){
    clock_unset(x->x_clock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
    
    /* CHECKED bang not sent if sequence is empty */
    if(x->x_nevents){
        if(modechanged){
            x->x_nextscoretime = x->x_sequence->e_delta;
            /* playback data never sent within the scheduler event of
           a start message (even for the first delta <= 0), LATER rethink */
            x->x_clockdelay = x->x_sequence->e_delta * x->x_newtimescale;
        }
        else {  /* CHECKED timescale change */
            if(SEQ_ISRUNNING(x))
                x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
            x->x_clockdelay *= x->x_newtimescale / x->x_timescale;
        }
        if(x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        x->x_timescale = x->x_newtimescale;
        clock_delay(x->x_clock, x->x_clockdelay);
        x->x_prevtime = clock_getlogicaltime();
    }
    else
        x->x_mode = SEQ_IDLEMODE;
}

static void seq_startslavery(t_seq *x){
    if(x->x_nevents){
        x->x_playhead = 0;
        x->x_nextscoretime = 0.;
        x->x_prevtime = 0.;
        x->x_slaveprevtime = 0.;
    }
    else
        x->x_mode = SEQ_IDLEMODE;
}

static void seq_setmode(t_seq *x, int newmode){
    int changed = (x->x_mode != newmode);
    if(changed){
        switch (x->x_mode){
            case SEQ_IDLEMODE:
                break;
            case SEQ_RECMODE:
                seq_stoprecording(x);
                break;
            case SEQ_PLAYMODE:
                seq_stopplayback(x);
                break;
            case SEQ_SLAVEMODE:
                seq_stopslavery(x);
                break;
            default:
                pd_error(x, "bug [seq]: seq_setmode (old)");
                return;
        }
        x->x_mode = newmode;
    }
    switch (newmode){
        case SEQ_IDLEMODE:
            break;
        case SEQ_RECMODE:
            seq_startrecording(x);
            break;
        case SEQ_PLAYMODE:
            seq_startplayback(x, changed);
            break;
        case SEQ_SLAVEMODE:
            seq_startslavery(x);
            break;
        default:
            pd_error(x, "bug [seq]: seq_setmode (new)");
    }
}

static void seq_settimescale(t_seq *x, float newtimescale){
    if(newtimescale < 1e-20)
        x->x_newtimescale = 1e-20;
    else if(newtimescale > 1e20)
        x->x_newtimescale = 1e20;
    else
        x->x_newtimescale = newtimescale;
}

static void seq_clocktick(t_seq *x){
    if(x->x_mode == SEQ_PLAYMODE || x->x_mode == SEQ_SLAVEMODE){
        t_seqevent *ep = &x->x_sequence[x->x_playhead++];
        unsigned char *bp = ep->e_bytes;
nextevent:
        outlet_float(((t_object *)x)->ob_outlet, *bp++);
        if(*bp != SEQ_EOM){
            outlet_float(((t_object *)x)->ob_outlet, *bp++);
            if(*bp != SEQ_EOM){
                outlet_float(((t_object *)x)->ob_outlet, *bp++);
            if(*bp != SEQ_EOM)
                outlet_float(((t_object *)x)->ob_outlet, *bp++);
            }
        }
        if(x->x_mode != SEQ_PLAYMODE && x->x_mode != SEQ_SLAVEMODE)
            return;  /* protecting against outlet -> 'stop' etc. */
        if(x->x_playhead < x->x_nevents){
            ep++;
            x->x_nextscoretime += ep->e_delta;
            if(ep->e_delta < SEQ_TICKEPSILON){
            /* continue output in the same scheduler event, LATER rethink */
                x->x_playhead++;
                bp = ep->e_bytes;
                goto nextevent;
            }
            else{
                x->x_clockdelay = ep->e_delta * x->x_timescale;
                if(x->x_clockdelay < 0.)
                    x->x_clockdelay = 0.;
                clock_delay(x->x_clock, x->x_clockdelay);
                x->x_prevtime = clock_getlogicaltime();
            }
        }
        else{
            seq_setmode(x, SEQ_IDLEMODE);
            /* CHECKED bang sent immediately _after_ last byte */
            outlet_bang(x->x_bangout);  /* LATER think about reentrancy */
        }
    }
}

/* timeout handler ('tick' is late) */
static void seq_slaveclocktick(t_seq *x){
    if(x->x_mode == SEQ_SLAVEMODE) clock_unset(x->x_clock);
}

// LATER dealing with self-invokation (outlet -> 'tick')
static void seq_tick(t_seq *x){
    if(x->x_mode == SEQ_SLAVEMODE){
        if(x->x_slaveprevtime >= 0){
            double elapsed = clock_gettimesince(x->x_slaveprevtime);
            if(elapsed < SEQ_MINTICKDELAY)
                return;
            clock_delay(x->x_slaveclock, elapsed);
            seq_settimescale(x, (float)(elapsed * (SEQ_TICKSPERSEC / 1000.)));
            if(SEQ_ISRUNNING(x)){
                x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
                x->x_clockdelay *= x->x_newtimescale / x->x_timescale;
            }
            else
                x->x_clockdelay = x->x_sequence[x->x_playhead].e_delta * x->x_newtimescale;
            if(x->x_clockdelay < 0.)
                x->x_clockdelay = 0.;
            clock_delay(x->x_clock, x->x_clockdelay);
            x->x_prevtime = clock_getlogicaltime();
            x->x_slaveprevtime = x->x_prevtime;
            x->x_timescale = x->x_newtimescale;
        }
        else{
            x->x_clockdelay = 0.;  // redundant
            x->x_prevtime = 0.;    // redundant
            x->x_slaveprevtime = clock_getlogicaltime();
            x->x_timescale = 1.;   // redundant
        }
    }
}

// CHECKED bang does the same as 'start 1024', not 'start <current-timescale>' (also if already in SEQ_PLAYMODE)
static void seq_bang(t_seq *x){
    seq_settimescale(x, 1.);
    seq_setmode(x, SEQ_PLAYMODE);  // CHECKED 'bang' stops recording
}

static void seq_float(t_seq *x, t_float f){
    if(x->x_mode == SEQ_RECMODE){
        unsigned char c = (unsigned char)f; // CHECKED noninteger and out of range silently truncated
        if(c < 128){
            if(x->x_status)
                seq_addbyte(x, c, 0);
        }
        else if(c != 254){  // CHECKED active sensing ignored
            if(x->x_status == 240){
                if(c == 247)
                    seq_endofsysex(x);
                else{
                    /* CHECKED rt bytes alike */
                    post("seq: unterminated sysex");  /* CHECKED */
                    seq_endofsysex(x);  /* CHECKED 247 added implicitly */
                    seq_checkstatus(x, c);
                }
            }
            else if(c != 247)
                seq_checkstatus(x, c);
        }
        seq_update(x);
    }
}

static void seq_symbol(t_seq *x, t_symbol *s){
    s = NULL;
    pd_error(x, "[seq]: no method for symbol");
}

static void seq_list(t_seq *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac && av->a_type == A_FLOAT)
        seq_float(x, av->a_w.w_float);
    /* CHECKED anything else/more silently ignored */
}

static void seq_record(t_seq *x){ // CHECKED 'record' stops playback, resets recording
    seq_doclear(x, 0);
    seq_update(x);
    seq_setmode(x, SEQ_RECMODE);
}

static void seq_append(t_seq *x){
    /* CHECKED 'append' stops playback */
    /* CHECKED if in SEQ_RECMODE, 'append' resets the timer */
    seq_setmode(x, SEQ_RECMODE);
}

static void seq_start(t_seq *x, t_floatarg f){
    if(f < -SEQ_STARTEPSILON) /* FIXME */
        seq_setmode(x, SEQ_SLAVEMODE); // ticks
    else{
        seq_settimescale(x, (f > SEQ_STARTEPSILON ? 1024. / f : 1.));
        seq_setmode(x, SEQ_PLAYMODE);  // CHECKED 'start' stops recording
    }
}

static void seq_stop(t_seq *x){
    seq_setmode(x, SEQ_IDLEMODE);
}

// CHECKED first delta time is set permanently (it is stored in a file)
static void seq_delay(t_seq *x, t_floatarg f){
    if(x->x_nevents){ // CHECKED signed/unsigned bug (not emulated)
        t_float total_delay;
        x->x_delay = (f > SEQ_TICKEPSILON ? f : 0.);
        total_delay = (x->x_delay + x->x_event_delay);
        x->x_sequence->e_delta = (total_delay < 0 ? 0 : total_delay);
    }
}

static void seq_eventdelay(t_seq *x, t_floatarg f){
    if(x->x_nevents){ // CHECKED signed/unsigned bug (not emulated)
        x->x_event_delay += f;
        t_float total_delay = (x->x_delay + x->x_event_delay);
        x->x_sequence->e_delta = (total_delay < 0 ? 0 : total_delay);
    }
}

// CHECKED all delta times are set permanently (they are stored in a file)
static void seq_hook(t_seq *x, t_floatarg f){
    int nevents;
    if((nevents = x->x_nevents)){
        t_seqevent *ev = x->x_sequence;
        if(f < 0)
            f = 0;  /* CHECKED signed/unsigned bug (not emulated) */
        while(nevents--)
            ev++->e_delta *= f;
    }
}

// start of extra (not available in Max)
static void seq_pause(t_seq *x){
    if(x->x_mode == SEQ_PLAYMODE && SEQ_ISRUNNING(x)){
        x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
        if(x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_unset(x->x_clock);
        x->x_prevtime = 0.;
    }
}

static void seq_continue(t_seq *x){
    if(x->x_mode == SEQ_PLAYMODE && SEQ_ISPAUSED(x)){
        if(x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_delay(x->x_clock, x->x_clockdelay);
        x->x_prevtime = clock_getlogicaltime();
    }
}

static void seq_click(t_seq *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    ctrl = alt = xpos = ypos = shift = 0;
    t_seqevent *ep = x->x_sequence;
    int nevents = x->x_nevents;
    char buf[MAXPDSTRING+2];
    editor_open(x->x_filehandle, (char*)(x->x_defname && x->x_defname != &s_ ? x->x_defname->s_name : "<anonymous>"), 0);
    t_float sum = 0;
    while(nevents--){  // LATER rethink sysex continuation
        seq_eventstring(x, buf, ep, 1, &sum);
        strcat(buf, ";\n");
        editor_append(x->x_filehandle, buf);
        ep++;
    }
    editor_setdirty(x->x_filehandle, 0);
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  wm deiconify .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  raise .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  focus .%lx.text\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
}


 static void seq_goto(t_seq *x, t_floatarg f1, t_floatarg f2){ // takes sec / ms
     if(x->x_nevents){
         t_seqevent *ev;
         int ndx, nevents = x->x_nevents;
         double ms = (double)f1 * 1000. + f2, sum;
         if(ms <= SEQ_TICKEPSILON)
             ms = 0.;
         if(x->x_mode != SEQ_PLAYMODE){
             seq_settimescale(x, x->x_timescale);
             seq_setmode(x, SEQ_PLAYMODE);
             // clock_delay() has been called in setmode, LATER avoid
             clock_unset(x->x_clock);
             x->x_prevtime = 0.;
         }
         for(ndx = 0, ev = x->x_sequence, sum = SEQ_TICKEPSILON; ndx < nevents; ndx++, ev++){
             if((sum += ev->e_delta) >= ms){
                 x->x_playhead = ndx;
                 x->x_nextscoretime = sum;
                 x->x_clockdelay = sum - SEQ_TICKEPSILON - ms;
                 if(x->x_clockdelay < 0.)
                     x->x_clockdelay = 0.;
                 if(SEQ_ISRUNNING(x)){
                     clock_delay(x->x_clock, x->x_clockdelay);
                     x->x_prevtime = clock_getlogicaltime();
                 }
                 break;
             }
         }
     }
 }

 static void seq_scoretime(t_seq *x, t_symbol *s){ // send score time to a receive name
     s = canvas_realizedollar(x->x_canvas, s);
     if(s && s->s_thing && x->x_mode == SEQ_PLAYMODE){  // LATER other modes
         t_atom aout[2];
         double ms, clockdelay = x->x_clockdelay;
         t_float f1, f2;
         if(SEQ_ISRUNNING(x))
             clockdelay -= clock_gettimesince(x->x_prevtime);
         ms = x->x_nextscoretime - clockdelay / x->x_timescale;
 // Send ms as a pair of floats (f1, f2) = (coarse in sec, fine in msec).
 // Any ms may then be exactly reconstructed as (double)f1 * 1000. + f2.
 // Currently, f2 may be negative.  LATER consider truncating f1 so that
 // only significant digits are on (when using 1 ms resolution, a float
 // stores only up to 8.7 minutes without distortion, 100 ms resolution
 // gives 14.5 hours of non-distorted floats, etc.)
         f1 = ms * .001;
         f2 = ms - (double)f1 * 1000.;
         if(f2 < .001 && f2 > -.001)
             f2 = 0.;
         SETFLOAT(&aout[0], f1);
         SETFLOAT(&aout[1], f2);
         pd_list(s->s_thing, &s_list, 2, aout);
     }
 }
 
 static void seq_tempo(t_seq *x, t_floatarg f){
     if(f > SEQ_TEMPOEPSILON){
         seq_settimescale(x, 1. / f);
         if(x->x_mode == SEQ_PLAYMODE)
             seq_startplayback(x, 0);
     }
     // FIXME else pause, LATER reverse playback if(f < -SEQ_TEMPOEPSILON)
 }
 
 static void seq_cd(t_seq *x, t_symbol *s){
    panel_setopendir(x->x_filehandle, s);
}

static void seq_pwd(t_seq *x, t_symbol *s){
    t_symbol *dir;
    s = canvas_realizedollar(x->x_canvas, s);
    if(s && s->s_thing && (dir = panel_getopendir(x->x_filehandle)))
        pd_symbol(s->s_thing, dir);
}
// end of extra

static int seq_eventcomparehook(const void *e1, const void *e2){
    return(((t_seqevent *)e1)->e_delta > ((t_seqevent *)e2)->e_delta ? 1 : -1);
}

static int seq_tempocomparehook(const void *t1, const void *t2){
    return(((t_seqtempo *)t1)->t_scoretime > ((t_seqtempo *)t2)->t_scoretime ? 1 : -1);
}

static int seq_mrhook(t_mifiread *mr, void *hookdata, int evtype){
    t_seq *x = (t_seq *)hookdata;
    double scoretime = mifiread_getscoretime(mr);
    if((evtype >= 0x80 && evtype < 0xf0) || (evtype == MIFIMETA_EOT)){
        if(x->x_eventreadhead < x->x_nevents){
            t_seqevent *sev = &x->x_sequence[x->x_eventreadhead++];
            int status = mifiread_getstatus(mr);
            sev->e_delta = scoretime;
            sev->e_bytes[0] = status | mifiread_getchannel(mr);
            sev->e_bytes[1] = mifiread_getdata1(mr);
            if(MIFI_ONEDATABYTE(status) || evtype == 0x2f)
                sev->e_bytes[2] = SEQ_EOM;
            else{
                sev->e_bytes[2] = mifiread_getdata2(mr);
                sev->e_bytes[3] = SEQ_EOM;
            }
        }
        else if(x->x_eventreadhead == x->x_nevents){
            pd_error(x, "bug [seq]: seq_mrhook 1");
            x->x_eventreadhead++;
        }
    }
    else if(evtype == MIFIMETA_TEMPO){
        if(x->x_temporeadhead < x->x_ntempi){
            t_seqtempo *stm = &x->x_tempomap[x->x_temporeadhead++];
            stm->t_scoretime = scoretime;
            stm->t_sr = mifiread_gettempo(mr);
/* #ifdef SEQ_DEBUG
        loudbug_post("tempo %g at %g", stm->t_sr, scoretime);
#endif */
        }
        else if(x->x_temporeadhead == x->x_ntempi){
            pd_error(x, "bug [seq]: seq_mrhook 2");
            x->x_temporeadhead++;
        }
    }
    return(1);
}

/* apply tempo and fold */
static void seq_foldtime(t_seq *x, double deftempo){
    t_seqevent *sev;
    t_seqtempo *stm = x->x_tempomap;
    double coef = 1000. / deftempo;
    int ex, tx = 0;
    double prevscoretime = 0.;
    while(tx < x->x_ntempi && stm->t_scoretime < SEQ_TICKEPSILON)
    tx++, coef = 1000. / stm++->t_sr;
    for(ex = 0, sev = x->x_sequence; ex < x->x_nevents; ex++, sev++){
        double clockdelta = 0.;
        while(tx < x->x_ntempi && stm->t_scoretime <= sev->e_delta){
            clockdelta += (stm->t_scoretime - prevscoretime) * coef;
            prevscoretime = stm->t_scoretime;
            tx++;
            coef = 1000. / stm++->t_sr;
        }
        clockdelta += (sev->e_delta - prevscoretime) * coef;
        prevscoretime = sev->e_delta;
        sev->e_delta = clockdelta;
    }
}

static int seq_mfread(t_seq *x, char *path){
    int result = 0;
    t_mifiread *mr = mifiread_new((t_pd *)x);
    if(!mifiread_open(mr, path, "", 0))
        goto mfreadfailed;
/* #ifdef SEQ_DEBUG
    post("midifile (format %d): %d tracks, %d ticks",
        mifiread_getformat(mr), mifiread_gethdtracks(mr),
        mifiread_getbeatticks(mr));
    if(mifiread_getnframes(mr))
        post(" (%d smpte frames)", mifiread_getnframes(mr));
    else
        post(" per beat");
#endif */
    if(!seq_dogrowing(x, mifiread_getnevents(mr), mifiread_getntempi(mr)))
        goto mfreadfailed;
    x->x_eventreadhead = 0;
    x->x_temporeadhead = 0;
    if(mifiread_doit(mr, seq_mrhook, x) != MIFIREAD_EOF)
        goto mfreadfailed;
    if(x->x_eventreadhead < x->x_nevents){
        pd_error(x, "bug [seq]: seq_mfread 1");
        post("declared %d events, got %d",
        x->x_nevents, x->x_eventreadhead);
        x->x_nevents = x->x_eventreadhead;
    }
    if(x->x_nevents)
        qsort(x->x_sequence, x->x_nevents, sizeof(*x->x_sequence), seq_eventcomparehook);
    if(x->x_temporeadhead < x->x_ntempi){
        pd_error(x, "bug [seq]: seq_mfread 2");
        post("declared %d tempi, got %d", x->x_ntempi, x->x_temporeadhead);
        x->x_ntempi = x->x_temporeadhead;
    }
    if(x->x_ntempi)
        qsort(x->x_tempomap, x->x_ntempi, sizeof(*x->x_tempomap), seq_tempocomparehook);
    seq_foldtime(x, mifiread_getdeftempo(mr));
/* #ifdef SEQ_DEBUG
    loudbug_post("seq: got %d events from midi file", x->x_nevents);
#endif */
    result = 1;
mfreadfailed:
    mifiread_free(mr);
    return(result);
}

static int seq_mfwrite(t_seq *x, char *path){
    int result = 0;
    t_seqevent *sev = x->x_sequence;
    int nevents = x->x_nevents;
    t_mifiwrite *mw = mifiwrite_new((t_pd *)x);
    if(!mifiwrite_open(mw, path, "", 1, 1))
        goto mfwritefailed;
    if(!mifiwrite_opentrack(mw, "seq-track", 1))
        goto mfwritefailed;
    while(nevents--){
        unsigned char *bp = sev->e_bytes;
        unsigned status = *bp & 0xf0;
        if(status > 127 && status < 240){
            if(!mifiwrite_channelevent(mw, sev->e_delta, status, *bp & 0x0f, bp[1], bp[2])){  /* SEQ_EOM ignored */
                pd_error(x, "[seq] cannot write channel event %d", status);
                goto mfwritefailed;
            }
        }
        /* FIXME system, sysex (first, and continuation) */
        sev++;
    }
    if(!mifiwrite_closetrack(mw, 0., 1))
        goto mfwritefailed;
    mifiwrite_close(mw);
    result = 1;
mfwritefailed:
    if(!result)
        post("while saving sequence into midi file \"%s\"", path);
    mifiwrite_free(mw);
    return(result);
}

/* CHECKED text file input: absolute timestamps, semi-terminated, verified */
/* FIXME prevent loading .pd files... */
static int seq_fromatoms(t_seq *x, int ac, t_atom *av){
    int i, nevents = 0;
    t_atom *ap;
    for(i = 0, ap = av; i < ac; i++, ap++)
        if(ap->a_type == A_SEMI)  /* FIXME parsing */
            nevents++;
    if(nevents){
        t_seqevent *ep;
        float prevtime = 0;
        if(!seq_dogrowing(x, nevents, 0))
            return(0);
        i = -1;
        nevents = 0;
        ep = x->x_sequence;
        while(ac--){
            if(av->a_type == A_FLOAT){
                if(i < 0){
                    ep->e_delta = av->a_w.w_float - prevtime;
                    prevtime = av->a_w.w_float;
                    i = 0;
                }
                else if(i < 4)
                    ep->e_bytes[i++] = av->a_w.w_float;
                // CHECKME else
            }
            else if(av->a_type == A_SEMI && i > 0){
                if(i < 4)
                    ep->e_bytes[i] = SEQ_EOM;
                nevents++;
                ep++;
                i = -1;
            }
            // CHECKME else
            av++;
        }
        x->x_nevents = nevents;
    }
    return(nevents);
}

static void seq_tobinbuf(t_seq *x, t_binbuf *bb){
    int nevents = x->x_nevents;
    t_seqevent *ep = x->x_sequence;
    t_atom at[5];
    float timestamp = 0;
    while(nevents--){
        unsigned char *bp = ep->e_bytes;
        int i;
        t_atom *ap = at;
        timestamp += ep->e_delta;
        SETFLOAT(ap, timestamp);  /* CHECKED same for sysex continuation */
        ap++;
        SETFLOAT(ap, *bp);
        for(i = 0, ap++, bp++; i < 3 && *bp != SEQ_EOM; i++, ap++, bp++)
            SETFLOAT(ap, *bp);
        binbuf_add(bb, i + 2, at);
        binbuf_addsemi(bb);
        ep++;
    }
}

static void seq_textread(t_seq *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    if(binbuf_read(bb, path, "", 0)) // CHECKED no complaint, open dialog presented
        panel_open(x->x_filehandle, 0);  /* LATER rethink */
    else{
        int nlines = /* CHECKED absolute timestamps */
            seq_fromatoms(x, binbuf_getnatom(bb), binbuf_getvec(bb));
        if(nlines < 0)
            /* CHECKED "bad MIDI file (truncated)" alert, even if a text file */
            pd_error(x, "[seq]: bad text file (truncated)");
        else if(nlines == 0){
        /* CHECKED no complaint, sequence erased, LATER rethink */
        }
    }
    binbuf_free(bb);
}

static void seq_textwrite(t_seq *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    seq_tobinbuf(x, bb);
    /* CHECKED empty sequence stored as an empty file */
    if(binbuf_write(bb, path, "", 0)){
        /* CHECKME complaint and FIXME */
        pd_error(x, "[seq]: error writing text file");
    }
    binbuf_free(bb);
}

static void seq_doread(t_seq *x, t_symbol *fn){
    static char fname[MAXPDSTRING];
    char *bufptr;
    int fd = canvas_open(x->x_canvas, fn->s_name, "", fname, &bufptr, MAXPDSTRING, 1);
    if(fd < 0){
        post("[seq] file '%s' not found", fn->s_name);
        return;
    }
    else{
        fname[strlen(fname)]='/';
        sys_close(fd);
    }
    if(!seq_mfread(x, fname))
        seq_textread(x, fname);
    x->x_playhead = 0;
    seq_update(x);
}

static void seq_dowrite(t_seq *x, t_symbol *fn){
    char buf[MAXPDSTRING], *dotp;
    if(x->x_canvas)
        canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    else{
        strncpy(buf, fn->s_name, MAXPDSTRING);
        buf[MAXPDSTRING-1] = 0;
    }
//    post("seq: writing %s", fn->s_name);
    /* save as text for any extension other then ".mid" */
    if((dotp = strrchr(fn->s_name, '.')) && strcmp(dotp + 1, "mid"))
        seq_textwrite(x, buf);
    else  /* save as mf for ".mid" (FIXME ignore case?) or no extension at all, LATER rethink */
        seq_mfwrite(x, buf);
}

static void seq_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    seq_doread((t_seq *)z, fn);
}

static void seq_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    seq_dowrite((t_seq *)z, fn);
}

static void seq_read(t_seq *x, t_symbol *s){
    if(s && s != &s_)
        seq_doread(x, s);
    else  /* CHECKED no default file name */
        /* start in a dir last read from, if any, otherwise in a canvas dir */
        panel_open(x->x_filehandle, 0);
}

static void seq_write(t_seq *x, t_symbol *s){
    if(s && s != &s_)
        seq_dowrite(x, s);
    else  /* CHECKED creation arg is a default file name */
        panel_save(x->x_filehandle, canvas_getdir(x->x_canvas), x->x_defname); /* always start in canvas dir */
}

static void seq_print(t_seq *x){
    int nevents = x->x_nevents;
    startpost("midiseq:");  // CHECKED
    if(nevents){
        t_seqevent *ep = x->x_sequence;
        char buf[MAXPDSTRING+2];
        int truncated;
        if(nevents > 16)
            nevents = 16, truncated = 1;
        else
            truncated = 0;
        endpost();
        t_float sum = 0;
        while(nevents--){  // CHECKED bytes are space-separated, no semi
            seq_eventstring(x, buf, ep, 0, &sum);
            post(buf);
            ep++;
        }
        if(truncated)
            post("...");  // CHECKED
    }
    else
        post(" no sequence");  // CHECKED
}

static void seq_editorhook(t_pd *z, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    seq_fromatoms((t_seq *)z, ac, av);
}

static void seq_free(t_seq *x){
    if(x->x_clock)
        clock_free(x->x_clock);
    if(x->x_slaveclock)
        clock_free(x->x_slaveclock);
    if(x->x_filehandle)
        file_free(x->x_filehandle);
    if(x->x_sequence != x->x_seqini)
        freebytes(x->x_sequence, x->x_seqsize * sizeof(*x->x_sequence));
    if(x->x_tempomap != x->x_tempomapini)
        freebytes(x->x_tempomap, x->x_tempomapsize * sizeof(*x->x_tempomap));
}

static void *seq_new(t_symbol *s){
    t_seq *x = (t_seq *)pd_new(seq_class);
    x->x_canvas = canvas_getcurrent();
    x->x_filehandle = file_new((t_pd *)x, 0, seq_readhook, seq_writehook, seq_editorhook);
    x->x_timescale = 1.;
    x->x_newtimescale = 1.;
    x->x_prevtime = 0.;
    x->x_slaveprevtime = 0.;
    x->x_seqsize = SEQ_INISEQSIZE;
    x->x_nevents = 0;
    x->x_delay = x->x_event_delay = 0;
    x->x_sequence = x->x_seqini;
    x->x_tempomapsize = SEQ_INITEMPOMAPSIZE;
    x->x_ntempi = 0;
    x->x_tempomap = x->x_tempomapini;
    outlet_new((t_object *)x, &s_anything);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    if(s && s != &s_){
        x->x_defname = s;  /* CHECKME if 'read' changes this */
        seq_doread(x, s);
    }
    else
        x->x_defname = &s_;
    x->x_clock = clock_new(x, (t_method)seq_clocktick);
    x->x_slaveclock = clock_new(x, (t_method)seq_slaveclocktick);
    return(x);
}

CYCLONE_OBJ_API void seq_setup(void){
    seq_class = class_new(gensym("seq"), (t_newmethod)seq_new,
        (t_method)seq_free, sizeof(t_seq), 0, A_DEFSYM, 0);
    class_addbang(seq_class, seq_bang);
    class_addfloat(seq_class, seq_float);
// CHECKED symbol rejected
    class_addsymbol(seq_class, seq_symbol);
// CHECKED 1st atom of a list accepted if a float, ignored if a symbol
    class_addlist(seq_class, seq_list);
    class_addmethod(seq_class, (t_method)seq_clear, gensym("clear"), 0);
    class_addmethod(seq_class, (t_method)seq_record, gensym("record"), 0);
    class_addmethod(seq_class, (t_method)seq_append, gensym("append"), 0);
    class_addmethod(seq_class, (t_method)seq_start, gensym("start"), A_DEFFLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_stop, gensym("stop"), 0);
    class_addmethod(seq_class, (t_method)seq_tick, gensym("tick"), 0);
    class_addmethod(seq_class, (t_method)seq_delay, gensym("delay"), A_FLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_eventdelay, gensym("addeventdelay"), A_FLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_hook, gensym("hook"), A_FLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_read, gensym("read"), A_DEFSYM, 0);
    class_addmethod(seq_class, (t_method)seq_write, gensym("write"), A_DEFSYM, 0);
    class_addmethod(seq_class, (t_method)seq_print, gensym("print"), 0);
// not available in Max
    class_addmethod(seq_class, (t_method)seq_pause, gensym("pause"), 0);
    class_addmethod(seq_class, (t_method)seq_continue, gensym("continue"), 0);
    class_addmethod(seq_class, (t_method)seq_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
// not available in Max and not documented for being considered problematic (hence, unsupported)
// see https://github.com/porres/pd-cyclone/issues/566
  class_addmethod(seq_class, (t_method)seq_goto, gensym("goto"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_scoretime, gensym("scoretime"), A_SYMBOL, 0);
    class_addmethod(seq_class, (t_method)seq_tempo, gensym("tempo"), A_FLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_cd, gensym("cd"), A_DEFSYM, 0);
    class_addmethod(seq_class, (t_method)seq_pwd, gensym("pwd"), A_SYMBOL, 0);
    file_setup(seq_class, 0);
}
