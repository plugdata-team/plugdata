// based on cyclone's [seq]

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "shared/grow.h"
#include "shared/file.h"
#include "shared/mifi.h"

#define PANIC_VOID  0xFF

#define SEQ_INISEQSIZE              256   /* LATER rethink */
#define SEQ_INITEMPOMAPSIZE         128   /* LATER rethink */
#define SEQ_EOM                     255   /* end of message marker, LATER rethink */
#define SEQ_TICKSPERSEC             48
#define SEQ_MINTICKDELAY            1.  /* LATER rethink */
#define SEQ_TICKEPSILON  ((double)  .0001)
#define SEQ_STARTEPSILON            .0001  /* if inside: play unmodified */
#define SEQ_TEMPOEPSILON            .0001  /* if inside: pause */

#define SEQ_ISRUNNING(x)  ((x)->x_prevtime > (double).0001)
#define SEQ_ISPAUSED(x)  ((x)->x_prevtime <= (double).0001)

enum { SEQ_IDLEMODE, SEQ_RECMODE, SEQ_PLAYMODE, SEQ_SLAVEMODE };

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
    t_hammerfile  *x_filehandle;
    int            x_mode;
    int            x_playhead;
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
// panic
    unsigned char  x_note_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    unsigned char  x_notes[16][128]; // [channels][pitches]
} t_seq;

static t_class *seq_class;

static void panic_input(t_seq *x, t_float f){
    if(f >= 0 && f < 256){
        unsigned char val = (int)f;
        if(val & 0x80){
            x->x_note_status = val & 0xF0;
            if(x->x_note_status == 0x80 || x->x_note_status == 0x90)
                x->x_channel = val & 0x0F;
            else
                x->x_note_status = 0;
        }
        else if(x->x_note_status){
            if(x->x_pitch == PANIC_VOID){
                x->x_pitch = val;
                return;
            }
            else if(x->x_note_status == 0x90 && val)
                x->x_notes[x->x_channel][x->x_pitch]++;
            else
                x->x_notes[x->x_channel][x->x_pitch]--;
        }
        
    }
    x->x_pitch = PANIC_VOID;
}

static void panic_clear(t_seq *x){
    memset(x->x_notes, 0, sizeof(x->x_notes));
    x->x_pitch = PANIC_VOID;
}

static void seq_panic(t_seq *x){
    for(int chn = 0; chn < 16; chn++){
        for(int pch = 0; pch < 128; pch++){
            int status = 0x090 | chn;
            while(x->x_notes[chn][pch]){
                outlet_float(((t_object *)x)->ob_outlet, status);
                outlet_float(((t_object *)x)->ob_outlet, pch);
                outlet_float(((t_object *)x)->ob_outlet, 0);
                x->x_notes[chn][pch]--;
            }
        }
    }
    panic_clear(x);
}



static void seq_doclear(t_seq *x, int dofree){
    if(dofree){
        if (x->x_sequence != x->x_seqini){
            freebytes(x->x_sequence, x->x_seqsize * sizeof(*x->x_sequence));
            x->x_sequence = x->x_seqini;
            x->x_seqsize = SEQ_INISEQSIZE;
        }
        if (x->x_tempomap != x->x_tempomapini){
            freebytes(x->x_tempomap, x->x_tempomapsize * sizeof(*x->x_tempomap));
            x->x_tempomap = x->x_tempomapini;
            x->x_tempomapsize = SEQ_INITEMPOMAPSIZE;
        }
    }
    x->x_nevents = 0;
    x->x_ntempi = 0;
}

static int seq_dogrowing(t_seq *x, int nevents, int ntempi){
    if (nevents > x->x_seqsize){
        int nrequested = nevents;
        x->x_sequence =
        grow_nodata(&nrequested, &x->x_seqsize, x->x_sequence,
            SEQ_INISEQSIZE, x->x_seqini, sizeof(*x->x_sequence));
        if (nrequested < nevents){
            x->x_nevents = 0;
            x->x_ntempi = 0;
            return (0);
        }
    }
    if (ntempi > x->x_tempomapsize){
        int nrequested = ntempi;
        x->x_tempomap =
        grow_nodata(&nrequested, &x->x_tempomapsize, x->x_tempomap,
                    SEQ_INITEMPOMAPSIZE, x->x_tempomapini,
                    sizeof(*x->x_tempomap));
        if (nrequested < ntempi){
            x->x_ntempi = 0;
            return (0);
        }
    }
    x->x_nevents = nevents;
    x->x_ntempi = ntempi;
    return (1);
}

static void seq_complete(t_seq *x){
    if (x->x_evelength < x->x_expectedlength){ // no warning if no data after status byte requiring data
        if(x->x_evelength > 1)
            post("[midi]: truncated midi message");
    }
    else{
        t_seqevent *ep = &x->x_sequence[x->x_nevents];
        ep->e_delta = clock_gettimesince(x->x_prevtime);
        x->x_prevtime = clock_getlogicaltime();
        if(x->x_evelength < 4)
            ep->e_bytes[x->x_evelength] = SEQ_EOM;
        x->x_nevents++;
        if (x->x_nevents >= x->x_seqsize){
            int nexisting = x->x_seqsize;
            /* store-ahead scheme, LATER consider using x_currevent */
            int nrequested = x->x_nevents + 1;
            x->x_sequence = grow_withdata(&nrequested, &nexisting, &x->x_seqsize,
                            x->x_sequence, SEQ_INISEQSIZE, x->x_seqini, sizeof(*x->x_sequence));
            if (nrequested <= x->x_nevents)
                x->x_nevents = 0;
        }
    }
    x->x_evelength = 0;
}

static void seq_checkstatus(t_seq *x, unsigned char c){
    if(x->x_status && x->x_evelength > 1)  /* LATER rethink */
        seq_complete(x);
    if (c < 192)
        x->x_expectedlength = 3;
    else if (c < 224)
        x->x_expectedlength = 2;
    else if (c < 240)
        x->x_expectedlength = 3;
    else if (c < 248) /* FIXME */
        x->x_expectedlength = -1;
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
    if (x->x_evelength == x->x_expectedlength){
        seq_complete(x);
        if (x->x_status){
            x->x_sequence[x->x_nevents].e_bytes[0] = x->x_status;
            x->x_evelength = 1;
        }
    }
    else if (x->x_evelength == 4){
        if (x->x_status != 240)
            pd_error(x, "bug [midi]: seq_addbyte");
        /* CHECKED sysex is broken into 4-byte packets marked with
         the actual delta time of last byte received in a packet */
        seq_complete(x);
    }
    else if (docomplete)
        seq_complete(x);
}

static void seq_endofsysex(t_seq *x){
    seq_addbyte(x, 247, 1);
    x->x_status = 0;
}

static void seq_stoprecording(t_seq *x){
    if (x->x_status == 240){
        post("[midi]: incomplete sysex");  /* CHECKED */
        seq_endofsysex(x);  /* CHECKED 247 added implicitly */
    }
    else if (x->x_status)
        seq_complete(x);
    /* CHECKED running status used in recording, but not across recordings */
    x->x_status = 0;
}

static void seq_stopplayback(t_seq *x){
    /* FIXME */
    /* CHECKED "[midi]: incomplete sysex" at playback stop, 247 added implicitly */
    /* CHECKME resetting controllers, etc. */
    /* CHECKED bang not sent if playback stopped early */
    clock_unset(x->x_clock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
}

static void seq_stopslavery(t_seq *x){ /* FIXME */
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
    if (x->x_nevents){
        if (modechanged){
            x->x_nextscoretime = x->x_sequence->e_delta;
            /* playback data never sent within the scheduler event of
             a start message (even for the first delta <= 0), LATER rethink */
            x->x_clockdelay = x->x_sequence->e_delta * x->x_newtimescale;
        }
        else{  /* CHECKED timescale change */
            if (SEQ_ISRUNNING(x))
                x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
            x->x_clockdelay *= x->x_newtimescale / x->x_timescale;
        }
        if (x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        x->x_timescale = x->x_newtimescale;
        clock_delay(x->x_clock, x->x_clockdelay);
        x->x_prevtime = clock_getlogicaltime();
    }
    else x->x_mode = SEQ_IDLEMODE;
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
        switch(x->x_mode){
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
//                pd_error(x, "bug [midi]: seq_setmode (old)");
                return;
        }
        x->x_mode = newmode;
    }
    switch(newmode){
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
            pd_error(x, "bug [midi]: seq_setmode (new)");
    }
}

static void seq_settimescale(t_seq *x, t_floatarg f){
    x->x_newtimescale = f < 1e-20 ? 1e-20 : f > 1e20 ? 1e20 : f;
}

static void seq_clocktick(t_seq *x){
    t_float output;
    if (x->x_mode == SEQ_PLAYMODE || x->x_mode == SEQ_SLAVEMODE){
        t_seqevent *ep = &x->x_sequence[x->x_playhead++];
        unsigned char *bp = ep->e_bytes;
    nextevent:
        output = (t_float)*bp++;
        outlet_float(((t_object *)x)->ob_outlet, output);
        panic_input(x, output);
        if (*bp != SEQ_EOM){
            output = (t_float)*bp++;
            outlet_float(((t_object *)x)->ob_outlet, output);
            panic_input(x, output);
            if (*bp != SEQ_EOM){
                output = (t_float)*bp++;
                outlet_float(((t_object *)x)->ob_outlet, output);
                panic_input(x, output);
                if (*bp != SEQ_EOM){
                    output = (t_float)*bp++;
                    outlet_float(((t_object *)x)->ob_outlet, output);
                    panic_input(x, output);
                }
            }
        }
        if (x->x_mode != SEQ_PLAYMODE && x->x_mode != SEQ_SLAVEMODE)
            return;  /* protecting against outlet -> 'stop' etc. */
        if (x->x_playhead < x->x_nevents){
            ep++;
            x->x_nextscoretime += ep->e_delta;
            if (ep->e_delta < SEQ_TICKEPSILON){
                /* continue output in the same scheduler event, LATER rethink */
                x->x_playhead++;
                bp = ep->e_bytes;
                goto nextevent;
            }
            else{
                x->x_clockdelay = ep->e_delta * x->x_timescale;
                if (x->x_clockdelay < 0.)
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
    if (x->x_mode == SEQ_SLAVEMODE)
        clock_unset(x->x_clock);
}

static void seq_bang(t_seq *x){
    if(x->x_mode == SEQ_SLAVEMODE){
        if (x->x_slaveprevtime > 0){
            double elapsed = clock_gettimesince(x->x_slaveprevtime);
            if (elapsed < SEQ_MINTICKDELAY)
                return;
            clock_delay(x->x_slaveclock, elapsed);
            seq_settimescale(x, (float)(elapsed * (SEQ_TICKSPERSEC / 1000.)));
            if (SEQ_ISRUNNING(x)){
                x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
                x->x_clockdelay *= x->x_newtimescale / x->x_timescale;
            }
            else x->x_clockdelay =
                x->x_sequence[x->x_playhead].e_delta * x->x_newtimescale;
            if (x->x_clockdelay < 0.)
                x->x_clockdelay = 0.;
            clock_delay(x->x_clock, x->x_clockdelay);
            x->x_prevtime = clock_getlogicaltime();
            x->x_slaveprevtime = x->x_prevtime;
            x->x_timescale = x->x_newtimescale;
        }
        else{
            x->x_clockdelay = 0.;  /* redundant */
            x->x_prevtime = 0.;    /* redundant */
            x->x_slaveprevtime = clock_getlogicaltime();
            x->x_timescale = 1.;       /* redundant */
        }
    }
}

static void seq_pause(t_seq *x){
    if (x->x_mode == SEQ_PLAYMODE && SEQ_ISRUNNING(x)){
        x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
        if (x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_unset(x->x_clock);
        x->x_prevtime = 0.;
    }
}

static void seq_float(t_seq *x, t_float f){
    if (x->x_mode == SEQ_RECMODE){
        /* CHECKED noninteger and out of range silently truncated */
        unsigned char c = (unsigned char)f;
        if (c < 128 && x->x_status)
            seq_addbyte(x, c, 0);
        else if (c != 254){  /* CHECKED active sensing ignored */
            if (x->x_status == 240){
                if (c == 247)
                    seq_endofsysex(x);
                else{
                    /* CHECKED rt bytes alike */
                    post("[midi]: unterminated sysex");  /* CHECKED */
                    seq_endofsysex(x);  /* CHECKED 247 added implicitly */
                    seq_checkstatus(x, c);
                }
            }
            else if (c != 247)
                seq_checkstatus(x, c);
        }
    }
    else if(f != 0){
        seq_settimescale(x, 1);
        seq_setmode(x, SEQ_PLAYMODE);
    }
    else{ // stop
        seq_pause(x);
        seq_panic(x);
    }
}

static void seq_record(t_seq *x){ // stops playback, resets recording
    seq_doclear(x, 0);
    seq_setmode(x, SEQ_RECMODE);
}

static void seq_start(t_seq *x, t_floatarg f){
    if(f < -SEQ_STARTEPSILON) // FIXME
        seq_setmode(x, SEQ_SLAVEMODE); // ticks
    else{
        seq_settimescale(x, (f > SEQ_STARTEPSILON ? (100. / f) : 1.));
        seq_setmode(x, SEQ_PLAYMODE);  /* CHECKED 'start' stops recording */
    }
}

static void seq_dump(t_seq *x){
    seq_start(x, 1e20);
}

static void seq_stop(t_seq *x){
//    seq_setmode(x, SEQ_IDLEMODE);
    seq_pause(x);
    seq_panic(x);
}

static void seq_continue(t_seq *x){
    if (x->x_mode == SEQ_PLAYMODE && SEQ_ISPAUSED(x)){
        if (x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_delay(x->x_clock, x->x_clockdelay);
        x->x_prevtime = clock_getlogicaltime();
    }
}

static void seq_goto(t_seq *x, t_floatarg f1, t_floatarg f2){
    if (x->x_nevents){
        t_seqevent *ev;
        int ndx, nevents = x->x_nevents;
        double ms = (double)f1 * 1000. + f2, sum;
        if (ms <= SEQ_TICKEPSILON)
            ms = 0.;
        if (x->x_mode != SEQ_PLAYMODE){
            seq_settimescale(x, x->x_timescale);
            seq_setmode(x, SEQ_PLAYMODE);
            /* clock_delay() has been called in setmode, LATER avoid */
            clock_unset(x->x_clock);
            x->x_prevtime = 0.;
        }
        for (ndx = 0, ev = x->x_sequence, sum = SEQ_TICKEPSILON; ndx < nevents; ndx++, ev++){
            if ((sum += ev->e_delta) >= ms){
                x->x_playhead = ndx;
                x->x_nextscoretime = sum;
                x->x_clockdelay = sum - SEQ_TICKEPSILON - ms;
                if (x->x_clockdelay < 0.)
                    x->x_clockdelay = 0.;
                if (SEQ_ISRUNNING(x)){
                    clock_delay(x->x_clock, x->x_clockdelay);
                    x->x_prevtime = clock_getlogicaltime();
                }
                break;
            }
        }
    }
}

static void seq_scoretime(t_seq *x, t_symbol *s){
    if (s && s->s_thing && x->x_mode == SEQ_PLAYMODE){  /* LATER other modes */
        t_atom aout[2];
        double ms, clockdelay = x->x_clockdelay;
        t_float f1, f2;
        if (SEQ_ISRUNNING(x))
            clockdelay -= clock_gettimesince(x->x_prevtime);
        ms = x->x_nextscoretime - clockdelay / x->x_timescale;
        /* Send ms as a pair of floats (f1, f2) = (coarse in sec, fine in msec).
         Any ms may then be exactly reconstructed as (double)f1 * 1000. + f2.
         Currently, f2 may be negative.  LATER consider truncating f1 so that
         only significant digits are on (when using 1 ms resolution, a float
         stores only up to 8.7 minutes without distortion, 100 ms resolution
         gives 14.5 hours of non-distorted floats, etc.) */
        f1 = ms * .001;
        f2 = ms - (double)f1 * 1000.;
        if (f2 < .001 && f2 > -.001)
            f2 = 0.;
        SETFLOAT(&aout[0], f1);
        SETFLOAT(&aout[1], f2);
        pd_list(s->s_thing, &s_list, 2, aout);
    }
}

static void seq_tempo(t_seq *x, t_floatarg f){ // not available in Max
    if (f > SEQ_TEMPOEPSILON){
        seq_settimescale(x, 1. / f);
        if (x->x_mode == SEQ_PLAYMODE)
            seq_startplayback(x, 0);
    }
    // FIXME else pause, LATER reverse playback if (f < -SEQ_TEMPOEPSILON)
}

static int seq_eventcomparehook(const void *e1, const void *e2){
    return(((t_seqevent *)e1)->e_delta > ((t_seqevent *)e2)->e_delta ? 1 : -1);
}

static int seq_tempocomparehook(const void *t1, const void *t2){
    return (((t_seqtempo *)t1)->t_scoretime >
	    ((t_seqtempo *)t2)->t_scoretime ? 1 : -1);
}

static int seq_mrhook(t_mifiread *mr, void *hookdata, int evtype){
    t_seq *x = (t_seq *)hookdata;
    double scoretime = mifiread_getscoretime(mr);
    if (evtype >= 0xf0){
    }
    else if (evtype >= 0x80){
        if (x->x_eventreadhead < x->x_nevents){
            t_seqevent *sev = &x->x_sequence[x->x_eventreadhead++];
            int status = mifiread_getstatus(mr);
            sev->e_delta = scoretime;
            sev->e_bytes[0] = status | mifiread_getchannel(mr);
            sev->e_bytes[1] = mifiread_getdata1(mr);
            if (MIFI_ONEDATABYTE(status))
                sev->e_bytes[2] = SEQ_EOM;
            else{
                sev->e_bytes[2] = mifiread_getdata2(mr);
                sev->e_bytes[3] = SEQ_EOM;
            }
        }
        else if (x->x_eventreadhead == x->x_nevents){
            pd_error(x, "bug [midi]: seq_mrhook 1");
            x->x_eventreadhead++;
        }
    }
    else if (evtype == MIFIMETA_TEMPO){
        if (x->x_temporeadhead < x->x_ntempi){
            t_seqtempo *stm = &x->x_tempomap[x->x_temporeadhead++];
            stm->t_scoretime = scoretime;
            stm->t_sr = mifiread_gettempo(mr);
        }
        else if (x->x_temporeadhead == x->x_ntempi){
            pd_error(x, "bug [midi]: seq_mrhook 2");
            x->x_temporeadhead++;
        }
    }
    return (1);
}

/* apply tempo and fold */
static void seq_foldtime(t_seq *x, double deftempo){
    t_seqevent *sev;
    t_seqtempo *stm = x->x_tempomap;
    double coef = 1000. / deftempo;
    int ex, tx = 0;
    double prevscoretime = 0.;
    while (tx < x->x_ntempi && stm->t_scoretime < SEQ_TICKEPSILON)
	tx++, coef = 1000. / stm++->t_sr;
    for (ex = 0, sev = x->x_sequence; ex < x->x_nevents; ex++, sev++){
        double clockdelta = 0.;
        while (tx < x->x_ntempi && stm->t_scoretime <= sev->e_delta){
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
    if (!mifiread_open(mr, path, "", 0))
        goto mfreadfailed;
    if (!seq_dogrowing(x, mifiread_getnevents(mr), mifiread_getntempi(mr)))
        goto mfreadfailed;
    x->x_eventreadhead = 0;
    x->x_temporeadhead = 0;
    if (mifiread_doit(mr, seq_mrhook, x) != MIFIREAD_EOF)
        goto mfreadfailed;
    if (x->x_eventreadhead < x->x_nevents){
        pd_error(x, "bug [midi]: seq_mfread 1");
        post("declared %d events, got %d",
             x->x_nevents, x->x_eventreadhead);
        x->x_nevents = x->x_eventreadhead;
    }
    if (x->x_nevents)
	qsort(x->x_sequence, x->x_nevents, sizeof(*x->x_sequence),
	      seq_eventcomparehook);
    if (x->x_temporeadhead < x->x_ntempi)
    {
    pd_error(x, "bug [midi]: seq_mfread 2");
    post("declared %d tempi, got %d",
		     x->x_ntempi, x->x_temporeadhead);
	x->x_ntempi = x->x_temporeadhead;
    }
    if (x->x_ntempi)
	qsort(x->x_tempomap, x->x_ntempi, sizeof(*x->x_tempomap),
	      seq_tempocomparehook);
    seq_foldtime(x, mifiread_getdeftempo(mr));
    result = 1;
mfreadfailed:
    mifiread_free(mr);
    return (result);
}

static int seq_mfwrite(t_seq *x, char *path)
{
    int result = 0;
    t_seqevent *sev = x->x_sequence;
    int nevents = x->x_nevents;
    t_mifiwrite *mw = mifiwrite_new((t_pd *)x);
    if (!mifiwrite_open(mw, path, "", 1, 1))
	goto mfwritefailed;
    if (!mifiwrite_opentrack(mw, "seq-track", 1))
	goto mfwritefailed;
    while (nevents--)
    {
	unsigned char *bp = sev->e_bytes;
	unsigned status = *bp & 0xf0;
	if (status > 127 && status < 240)
	{
	    if (!mifiwrite_channelevent(mw, sev->e_delta, status, *bp & 0x0f,
					bp[1], bp[2]))  /* SEQ_EOM ignored */
	    {
		pd_error(x, "[midi] cannot write channel event %d", status);
		goto mfwritefailed;
	    }
	}
	/* FIXME system, sysex (first, and continuation) */
	sev++;
    }
    if (!mifiwrite_closetrack(mw, 0., 1))
	goto mfwritefailed;
    mifiwrite_close(mw);
    result = 1;
mfwritefailed:
    if (!result)
        post("while saving sequence into midi file \"%s\"", path);
    mifiwrite_free(mw);
    return (result);
}

/* CHECKED text file input: absolute timestamps, semi-terminated, verified */
/* FIXME prevent loading .pd files... */
static int seq_fromatoms(t_seq *x, int ac, t_atom *av, int abstime){
    int i, nevents = 0;
    t_atom *ap;
    for (i = 0, ap = av; i < ac; i++, ap++)
        if (ap->a_type == A_SEMI)  /* FIXME parsing */
            nevents++;
    if (nevents){
        t_seqevent *ep;
        float prevtime = 0;
        if (!seq_dogrowing(x, nevents, 0))
        return (0);
        i = -1;
        nevents = 0;
        ep = x->x_sequence;
        while (ac--){
            if (av->a_type == A_FLOAT){
                if (i < 0){
                    if (abstime){
                        ep->e_delta = av->a_w.w_float - prevtime;
                        prevtime = av->a_w.w_float;
                    }
                    else
                        ep->e_delta = av->a_w.w_float;
                    i = 0;
                }
                else if (i < 4) // CHECKME else
                    ep->e_bytes[i++] = av->a_w.w_float;
            }
            else if (av->a_type == A_SEMI && i > 0){
                if (i < 4)
                    ep->e_bytes[i] = SEQ_EOM;
                nevents++;
                ep++;
                i = -1;
            }
            /* CHECKME else */
            av++;
        }
        x->x_nevents = nevents;
    }
    return (nevents);
}

static void seq_tobinbuf(t_seq *x, t_binbuf *bb){
    int nevents = x->x_nevents;
    t_seqevent *ep = x->x_sequence;
    t_atom at[5];
    float timestamp = 0;
    while (nevents--){
        unsigned char *bp = ep->e_bytes;
        int i;
        t_atom *ap = at;
        timestamp += ep->e_delta;
        SETFLOAT(ap, timestamp);  /* CHECKED same for sysex continuation */
        ap++;
        SETFLOAT(ap, *bp);
        for (i = 0, ap++, bp++; i < 3 && *bp != SEQ_EOM; i++, ap++, bp++)
        SETFLOAT(ap, *bp);
        binbuf_add(bb, i + 2, at);
        binbuf_addsemi(bb);
        ep++;
    }
}

static void seq_textread(t_seq *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    if (binbuf_read(bb, path, "", 0))
	/* CHECKED no complaint, open dialog presented */
        hammerpanel_open(x->x_filehandle, 0);  /* LATER rethink */
    else{
        int nlines = /* CHECKED absolute timestamps */
        seq_fromatoms(x, binbuf_getnatom(bb), binbuf_getvec(bb), 1);
        if (nlines < 0)
        /* CHECKED "bad MIDI file (truncated)" alert, even if a text file */
        pd_error(x, "[midi]: bad text file (truncated)");
        else if (nlines == 0){
        } /* CHECKED no complaint, sequence erased, LATER rethink */
    }
    binbuf_free(bb);
}

static void seq_textwrite(t_seq *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    seq_tobinbuf(x, bb);
    /* CHECKED empty sequence stored as an empty file */
    if (binbuf_write(bb, path, "", 0))
	/* CHECKME complaint and FIXME */
        pd_error(x, "[midi]: error writing text file");
    binbuf_free(bb);
}

static void seq_doread(t_seq *x, t_symbol *fn, int creation){
    char buf[MAXPDSTRING];
    /* FIXME use open_via_path() */
    if (x->x_canvas)
        canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    else{
    	strncpy(buf, fn->s_name, MAXPDSTRING);
    	buf[MAXPDSTRING-1] = 0;
    }
    if (creation){
	/* loading during object creation -- CHECKED no warning if a file
	   specified with an arg does not exist, LATER rethink */
        FILE *fp;
        if (!(fp = sys_fopen(buf, "r")))
        return;
        fclose(fp);
    }
    /* CHECKED all cases: arg or not, message and creation */
//    post("[midi]: reading %s", fn->s_name);
    if(!seq_mfread(x, buf))
        seq_textread(x, buf);
}

static void seq_dowrite(t_seq *x, t_symbol *fn){
    char buf[MAXPDSTRING], *dotp;
    if (x->x_canvas)
        canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    else{
    	strncpy(buf, fn->s_name, MAXPDSTRING);
    	buf[MAXPDSTRING-1] = 0;
    }
    post("[midi]: writing %s", fn->s_name);  /* CHECKED arg or not */
    /* save as text for any extension other then ".mid" */
    if ((dotp = strrchr(fn->s_name, '.')) && strcmp(dotp + 1, "mid"))
        seq_textwrite(x, buf);
    else  /* save as mf for ".mid" (FIXME ignore case?) or no extension at all,
	     LATER rethink */
        seq_mfwrite(x, buf);
}

static void seq_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    seq_doread((t_seq *)z, fn, 0);
}

static void seq_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    seq_dowrite((t_seq *)z, fn);
}

static void seq_read(t_seq *x, t_symbol *s){
    if (s && s != &s_)
        seq_doread(x, s, 0);
    else  /* CHECKED no default file name */
	/* start in a dir last read from, if any, otherwise in a canvas dir */
        hammerpanel_open(x->x_filehandle, 0);
}

static void seq_write(t_seq *x, t_symbol *s){
    if (s && s != &s_)
        seq_dowrite(x, s);
    else  // CHECKED creation arg is a default file name
        hammerpanel_save(x->x_filehandle,
    canvas_getdir(x->x_canvas), x->x_defname); // always start in canvas dir
}

static void seq_editorhook(t_pd *z, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    seq_fromatoms((t_seq *)z, ac, av, 0);
}

static void seq_click(t_seq *x){
        hammerpanel_open(x->x_filehandle, 0);
}

static void seq_free(t_seq *x){
    if (x->x_clock)
        clock_free(x->x_clock);
    if (x->x_slaveclock)
        clock_free(x->x_slaveclock);
    if (x->x_filehandle)
        hammerfile_free(x->x_filehandle);
    if (x->x_sequence != x->x_seqini)
        freebytes(x->x_sequence, x->x_seqsize * sizeof(*x->x_sequence));
    if (x->x_tempomap != x->x_tempomapini)
        freebytes(x->x_tempomap, x->x_tempomapsize * sizeof(*x->x_tempomap));
}

static void *seq_new(t_symbol *s){
    t_seq *x = (t_seq *)pd_new(seq_class);
    x->x_canvas = canvas_getcurrent();
    x->x_filehandle = hammerfile_new((t_pd *)x, 0, seq_readhook, seq_writehook,
				     seq_editorhook);
    x->x_timescale = 1.;
    x->x_newtimescale = 1.;
    x->x_prevtime = 0.;
    x->x_slaveprevtime = 0.;
    x->x_seqsize = SEQ_INISEQSIZE;
    x->x_nevents = 0;
    x->x_sequence = x->x_seqini;
    x->x_tempomapsize = SEQ_INITEMPOMAPSIZE;
    x->x_ntempi = 0;
    x->x_tempomap = x->x_tempomapini;
    outlet_new((t_object *)x, &s_anything);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    if(s && s != &s_){
        x->x_defname = s;  /* CHECKME if 'read' changes this */
        seq_doread(x, s, 1);
    }
    else
        x->x_defname = &s_;
    x->x_clock = clock_new(x, (t_method)seq_clocktick);
    x->x_slaveclock = clock_new(x, (t_method)seq_slaveclocktick);
// panic
    x->x_note_status = 0;
    x->x_pitch = PANIC_VOID;
    panic_clear(x);
    return (x);
}

void midi_setup(void){
    seq_class = class_new(gensym("midi"), (t_newmethod)seq_new,
        (t_method)seq_free, sizeof(t_seq), 0, A_DEFSYM, 0);
    class_addbang(seq_class, seq_bang); // tick
    class_addfloat(seq_class, seq_float);
    class_addmethod(seq_class, (t_method)seq_record, gensym("record"), 0);
    class_addmethod(seq_class, (t_method)seq_start, gensym("start"), A_DEFFLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_stop, gensym("stop"), 0);
    class_addmethod(seq_class, (t_method)seq_dump, gensym("dump"), 0);
    class_addmethod(seq_class, (t_method)seq_read, gensym("open"), A_DEFSYM, 0);
    class_addmethod(seq_class, (t_method)seq_write, gensym("save"), A_DEFSYM, 0);
    class_addmethod(seq_class, (t_method)seq_panic, gensym("panic"), 0);
    class_addmethod(seq_class, (t_method)seq_pause, gensym("pause"), 0);
    class_addmethod(seq_class, (t_method)seq_continue, gensym("continue"), 0);
    class_addmethod(seq_class, (t_method)seq_click, gensym("click"), 0);
// extra stuff & "not ready yet"
    class_addmethod(seq_class, (t_method)seq_goto, gensym("goto"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(seq_class, (t_method)seq_scoretime, gensym("scoretime"), A_SYMBOL, 0);
    class_addmethod(seq_class, (t_method)seq_tempo, gensym("tempo"), A_FLOAT, 0);
    hammerfile_setup(seq_class, 0);
}
