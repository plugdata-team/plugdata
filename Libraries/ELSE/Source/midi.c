// based on cyclone's [seq]

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "m_pd.h"
#include "elsefile.h"
#include "mifi.h"

#define PANIC_VOID                  0xFF
#define MIDI_INISEQSIZE             256    // LATER rethink
#define MIDI_INITEMPOMAPSIZE        128    // LATER rethink
#define MIDI_META                   255    // META marker
#define MIDI_TICKSPERSEC            48
#define MIDI_MINTICKDELAY           1.     // LATER rethink
#define MIDI_TICKEPSILON ((double) .0001)
#define MIDI_STARTEPSILON          .0001   // if inside: play unmodified
#define MIDI_TEMPOEPSILON          .0001   // if inside: pause
#define MIDI_ISRUNNING(x) ((x)->x_prevtime > (double).0001)
#define MIDI_ISPAUSED(x) ((x)->x_prevtime <= (double).0001)

enum{MIDI_IDLEMODE, MIDI_RECMODE, MIDI_PLAYMODE, MIDI_SLAVEMODE};

typedef struct _midievent{
    double         e_delta;
    unsigned char  e_bytes[4];
}t_midievent;

typedef struct _miditempo{
    double  t_scoretime;  /* score ticks from start */
    double  t_sr;         /* score ticks per second */
}t_miditempo;

typedef struct _midi{
    t_object       x_ob;
    t_canvas      *x_canvas;
    t_symbol      *x_defname;
    t_elsefile    *x_elsefilehandle;
    int            x_loop;
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
    int            x_midisize;  /* as allocated */
    int            x_nevents;  /* as used */
    t_midievent    *x_sequence;
    t_midievent     x_midiini[MIDI_INISEQSIZE];
    int            x_temporeadhead;
    int            x_tempomapsize;  /* as allocated */
    int            x_ntempi;        /* as used */
    t_miditempo    *x_tempomap;
    t_miditempo     x_tempomapini[MIDI_INITEMPOMAPSIZE];
    t_clock       *x_clock;
    t_clock       *x_slaveclock;
    t_outlet      *x_bangout;
// panic
    unsigned char  x_note_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    unsigned char  x_notes[16][128]; // [channels][pitches]
}t_midi;

static t_class *midi_class;

static void panic_input(t_midi *x, t_float f){
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

static void midi_panic_clear(t_midi *x){
    memset(x->x_notes, 0, sizeof(x->x_notes));
    x->x_pitch = PANIC_VOID;
}

static void midi_panic(t_midi *x){
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
    midi_panic_clear(x);
}

/* Prior to this call a caller is supposed to check for *nrequested > *sizep.
   Returns a reallocated buffer's pointer (success) or a given 'bufini'
   default value (failure).
   Upon return *nrequested contains the actual number of elements:
   requested (success) or a given default value of 'inisize' (failure). */
static void *grow_nodata(int *nrequested, int *sizep, void *bufp,
int inisize, void *bufini, size_t typesize){
    int newsize = *sizep * 2;
    while(newsize < *nrequested)
        newsize *= 2;
    if(bufp == bufini)
        bufp = getbytes(newsize * typesize);
    else
        bufp = resizebytes(bufp, *sizep * typesize, newsize * typesize);
    if(bufp){
        *sizep = newsize;
        return (bufp);
    }
    else{
        *nrequested = *sizep = inisize;
        return (bufini);
    }
}

/* Like grow_nodata(), but preserving first *nexisting elements. */
static void *grow_withdata(int *nrequested, int *nexisting, int *sizep, void *bufp,
int inisize, void *bufini, size_t typesize){
    int newsize = *sizep * 2;
    while(newsize < *nrequested)
        newsize *= 2;
    if(bufp == bufini){
        if(!(bufp = getbytes(newsize * typesize))){
            *nrequested = *sizep = inisize;
            return (bufini);
        }
        *sizep = newsize;
        memcpy(bufp, bufini, *nexisting * typesize);
    }
    else{
//    int oldsize = *sizep;
        if(!(bufp = resizebytes(bufp, *sizep * typesize, newsize * typesize))){
            *nrequested = *sizep = inisize;
            *nexisting = 0;
            return (bufini);
        }
        *sizep = newsize;
    }
    return(bufp);
}

static void midi_clear(t_midi *x){
    x->x_nevents = x->x_ntempi = 0;
}

static int midi_dogrowing(t_midi *x, int nevents, int ntempi){
    if(nevents > x->x_midisize){
        int nrequested = nevents;
        x->x_sequence = grow_nodata(&nrequested, &x->x_midisize, x->x_sequence,
        MIDI_INISEQSIZE, x->x_midiini, sizeof(*x->x_sequence));
        if(nrequested < nevents){
            x->x_nevents = 0;
            x->x_ntempi = 0;
            return(0);
        }
    }
    if(ntempi > x->x_tempomapsize){
        int nrequested = ntempi;
        x->x_tempomap = grow_nodata(&nrequested, &x->x_tempomapsize, x->x_tempomap,
        MIDI_INITEMPOMAPSIZE, x->x_tempomapini,
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

static void midi_complete(t_midi *x){
    if(x->x_evelength < x->x_expectedlength){
        // no warning if no data after status byte requiring data
        if(x->x_evelength > 1)
            post("midi: truncated midi message");  // CHECKED
    // CHECKED nothing stored
    }
    else{
        t_midievent *ep = &x->x_sequence[x->x_nevents];
        ep->e_delta = clock_gettimesince(x->x_prevtime);
        x->x_prevtime = clock_getlogicaltime();
        if(x->x_evelength < 4)
            ep->e_bytes[x->x_evelength] = MIDI_META;
        x->x_nevents++;
        if(x->x_nevents >= x->x_midisize){
            int nexisting = x->x_midisize;
        // store-ahead scheme, LATER consider using x_currevent
            int nrequested = x->x_nevents + 1;
            x->x_sequence = grow_withdata(&nrequested, &nexisting, &x->x_midisize, x->x_sequence,
                  MIDI_INISEQSIZE, x->x_midiini, sizeof(*x->x_sequence));
            if(nrequested <= x->x_nevents)
                x->x_nevents = 0;
        }
    }
    x->x_evelength = 0;
}

static void midi_checkstatus(t_midi *x, unsigned char c){
    if(x->x_status && x->x_evelength > 1)  /* LATER rethink */
        midi_complete(x);
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
        midi_complete(x);
        return;
    }
    x->x_status = x->x_sequence[x->x_nevents].e_bytes[0] = c;
    x->x_evelength = 1;
}

static void midi_addbyte(t_midi *x, unsigned char c, int docomplete){
    x->x_sequence[x->x_nevents].e_bytes[x->x_evelength++] = c;
    if(x->x_evelength == x->x_expectedlength){
        midi_complete(x);
        if(x->x_status){
            x->x_sequence[x->x_nevents].e_bytes[0] = x->x_status;
            x->x_evelength = 1;
        }
    }
    else if(x->x_evelength == 4){
        if(x->x_status != 240)
            pd_error(x, "bug [midi]: midi_addbyte");
    /* CHECKED sysex is broken into 4-byte packets marked with
       the actual delta time of last byte received in a packet */
        midi_complete(x);
    }
    else if(docomplete)
        midi_complete(x);
}

static void midi_endofsysex(t_midi *x){
    midi_addbyte(x, 247, 1);
    x->x_status = 0;
}

static void midi_stoprecording(t_midi *x){
    if(x->x_status == 240){
        post("midi: incomplete sysex");  /* CHECKED */
        midi_endofsysex(x);  /* CHECKED 247 added implicitly */
    }
    else if(x->x_status)
        midi_complete(x);
    /* CHECKED running status used in recording, but not across recordings */
    x->x_status = 0;
}

static void midi_stopplayback(t_midi *x){
    /* FIXME */
    /* CHECKED "midi: incomplete sysex" at playback stop, 247 added implicitly */
    /* CHECKME resetting controllers, etc. */
    /* CHECKED bang not sent if playback stopped early */
    clock_unset(x->x_clock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
}

static void midi_stopslavery(t_midi *x){
    /* FIXME */
    clock_unset(x->x_clock);
    clock_unset(x->x_slaveclock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
}

static void midi_startrecording(t_midi *x){
    x->x_prevtime = clock_getlogicaltime();
    x->x_status = 0;
    x->x_evelength = 0;
    x->x_expectedlength = -1;  /* LATER rethink */
}

/* CHECKED running status not used in playback */
static void midi_startplayback(t_midi *x, int modechanged){
    clock_unset(x->x_clock);
    x->x_playhead = 0;
    x->x_nextscoretime = 0.;
    
    // CHECKED bang not sent if sequence is empty
    if(x->x_nevents){
        if(modechanged){
            x->x_nextscoretime = x->x_sequence->e_delta;
            /* playback data never sent within the scheduler event of
           a start message (even for the first delta <= 0), LATER rethink */
            x->x_clockdelay = x->x_sequence->e_delta * x->x_newtimescale;
        }
        else{  // CHECKED timescale change
            if(MIDI_ISRUNNING(x))
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
        x->x_mode = MIDI_IDLEMODE;
}

static void midi_startslavery(t_midi *x){
    if(x->x_nevents){
        x->x_playhead = 0;
        x->x_nextscoretime = 0.;
        x->x_prevtime = 0.;
        x->x_slaveprevtime = 0.;
    }
    else
        x->x_mode = MIDI_IDLEMODE;
}

static void midi_setmode(t_midi *x, int newmode){
    int changed = (x->x_mode != newmode);
    if(changed){
        switch (x->x_mode){
            case MIDI_IDLEMODE:
                break;
            case MIDI_RECMODE:
                midi_stoprecording(x);
                break;
            case MIDI_PLAYMODE:
                midi_stopplayback(x);
                break;
            case MIDI_SLAVEMODE:
                midi_stopslavery(x);
                break;
            default:
                pd_error(x, "bug [midi]: midi_setmode (old)");
                return;
        }
        x->x_mode = newmode;
    }
    switch(newmode){
        case MIDI_IDLEMODE:
            break;
        case MIDI_RECMODE:
            midi_startrecording(x);
            break;
        case MIDI_PLAYMODE:
            midi_startplayback(x, changed);
            break;
        case MIDI_SLAVEMODE:
            midi_startslavery(x);
            break;
        default:
            pd_error(x, "bug [midi]: midi_setmode (new)");
    }
}

static void midi_settimescale(t_midi *x, float newtimescale){
    if(newtimescale < 1e-20)
        x->x_newtimescale = 1e-20;
    else if(newtimescale > 1e20)
        x->x_newtimescale = 1e20;
    else
        x->x_newtimescale = newtimescale;
}

/* timeout handler ('tick' is late) */
static void midi_slaveclocktick(t_midi *x){
    if(x->x_mode == MIDI_SLAVEMODE) clock_unset(x->x_clock);
}

// LATER dealing with self-invokation (outlet -> 'tick')
static void midi_tick(t_midi *x){
    if(x->x_mode == MIDI_SLAVEMODE){
        if(x->x_slaveprevtime >= 0){
            double elapsed = clock_gettimesince(x->x_slaveprevtime);
            if(elapsed < MIDI_MINTICKDELAY)
                return;
            clock_delay(x->x_slaveclock, elapsed);
            midi_settimescale(x, (float)(elapsed * (MIDI_TICKSPERSEC / 1000.)));
            if(MIDI_ISRUNNING(x)){
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

static void midi_record(t_midi *x){ // CHECKED 'record' stops playback, resets recording
    midi_clear(x);
    midi_setmode(x, MIDI_RECMODE);
}

static void midi_play(t_midi *x){
    midi_setmode(x, MIDI_PLAYMODE);
}

static void midi_start(t_midi *x){
    midi_setmode(x, MIDI_SLAVEMODE); // ticks
}

static void midi_stop(t_midi *x){
    if(x->x_mode != MIDI_IDLEMODE){
        if(x->x_mode == MIDI_PLAYMODE || x->x_mode == MIDI_SLAVEMODE)
            midi_panic(x);
        midi_setmode(x, MIDI_IDLEMODE);
    }
}

// All delta times are set permanently (they are stored in a elsefile)
/*static void midi_hook(t_midi *x, t_floatarg f){
    int nevents;
    if((nevents = x->x_nevents)){
        t_midievent *ev = x->x_sequence;
        if(f < 0)
            f = 0;  // CHECKED signed/unsigned bug (not emulated)
        while(nevents--)
            ev++->e_delta *= f;
    }
}*/

static void midi_pause(t_midi *x){
    if(x->x_mode == MIDI_PLAYMODE && MIDI_ISRUNNING(x)){
        x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
        if(x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_unset(x->x_clock);
        x->x_prevtime = 0.;
    }
}

static void midi_continue(t_midi *x){
    if(x->x_mode == MIDI_PLAYMODE && MIDI_ISPAUSED(x)){
        if(x->x_clockdelay < 0.)
            x->x_clockdelay = 0.;
        clock_delay(x->x_clock, x->x_clockdelay);
        x->x_prevtime = clock_getlogicaltime();
    }
    else if(x->x_mode == MIDI_RECMODE && MIDI_ISPAUSED(x))
        midi_setmode(x, MIDI_RECMODE); // append
}

static void midi_float(t_midi *x, t_float f){
    if(x->x_mode == MIDI_RECMODE){
        unsigned char c = (unsigned char)f; // noninteger and out of range silently truncated
        if(c < 128 && x->x_status)
            midi_addbyte(x, c, 0);
        else if(c != 254){ // CHECKED active sensing ignored
            if(x->x_status == 240){
                if(c == 247)
                    midi_endofsysex(x);
                else{ // rt bytes alike
                    post("[midi]: unterminated sysex");  // CHECKED
                    midi_endofsysex(x);  // CHECKED 247 added implicitly
                    midi_checkstatus(x, c);
                }
            }
            else if(c != 247)
                midi_checkstatus(x, c);
        }
    }
    else if(f != 0)
        midi_setmode(x, MIDI_PLAYMODE);
    else
        midi_stop(x);
}

static void midi_loop(t_midi *x, t_floatarg f){
    x->x_loop = (f != 0);
}

static void midi_dump(t_midi *x){
    t_midievent *ep = x->x_sequence;
    int nevents = x->x_nevents;
    while(nevents--){  // LATER rethink sysex continuation
        unsigned char *bp = ep->e_bytes;
        outlet_float(((t_object *)x)->ob_outlet, (float)*bp);
        int i;
        for(i = 0, bp++; i < 3 && *bp != MIDI_META; i++, bp++)
            outlet_float(((t_object *)x)->ob_outlet, (float)*bp);
        ep++;
    }
    outlet_bang(x->x_bangout);
}

static void midi_clocktick(t_midi *x){
    t_float output;
    if(x->x_mode == MIDI_PLAYMODE || x->x_mode == MIDI_SLAVEMODE){
        t_midievent *ep = &x->x_sequence[x->x_playhead++];
        unsigned char *bp = ep->e_bytes;
    nextevent:
        output = (t_float)*bp++;
        outlet_float(((t_object *)x)->ob_outlet, output);
        panic_input(x, output);
        if(*bp != MIDI_META){
            output = (t_float)*bp++;
            outlet_float(((t_object *)x)->ob_outlet, output);
            panic_input(x, output);
            if(*bp != MIDI_META){
                output = (t_float)*bp++;
                outlet_float(((t_object *)x)->ob_outlet, output);
                panic_input(x, output);
                if(*bp != MIDI_META){
                    output = (t_float)*bp++;
                    outlet_float(((t_object *)x)->ob_outlet, output);
                    panic_input(x, output);
                }
            }
        }
        if(x->x_mode != MIDI_PLAYMODE && x->x_mode != MIDI_SLAVEMODE)
            return;  // protecting against outlet -> 'stop' etc.
        if(x->x_playhead < x->x_nevents){
            ep++;
            x->x_nextscoretime += ep->e_delta;
            if(ep->e_delta < MIDI_TICKEPSILON){ // continue output in the same scheduler event, LATER rethink
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
        else{ // CHECKED bang sent immediately _after_ last byte
            midi_setmode(x, MIDI_IDLEMODE);
            outlet_bang(x->x_bangout);  // LATER think about reentrancy
            if(x->x_loop)
                midi_float(x, 1);
        }
    }
}

/*static void midi_goto(t_midi *x, t_floatarg f){ // takes time in seconds
     if(x->x_nevents){
         t_midievent *ev;
         int ndx, nevents = x->x_nevents;
         double ms = (double)f * 1000., sum;
         if(ms <= MIDI_TICKEPSILON)
             ms = 0.;
         if(x->x_mode != MIDI_PLAYMODE){
             midi_settimescale(x, x->x_timescale);
             midi_setmode(x, MIDI_PLAYMODE);
             // clock_delay() has been called in setmode, LATER avoid
             clock_unset(x->x_clock);
             x->x_prevtime = 0.;
         }
         for(ndx = 0, ev = x->x_sequence, sum = MIDI_TICKEPSILON; ndx < nevents; ndx++, ev++){
             if((sum += ev->e_delta) >= ms){
                 x->x_playhead = ndx;
                 x->x_nextscoretime = sum;
                 x->x_clockdelay = sum - MIDI_TICKEPSILON - ms;
                 if(x->x_clockdelay < 0.)
                     x->x_clockdelay = 0.;
                 if(MIDI_ISRUNNING(x)){
                     clock_delay(x->x_clock, x->x_clockdelay);
                     x->x_prevtime = clock_getlogicaltime();
                 }
                 break;
             }
         }
     }
 }*/
 
 static void midi_speed(t_midi *x, t_floatarg f){
     if(f > MIDI_TEMPOEPSILON){
         midi_settimescale(x, 100./f);
         if(MIDI_ISRUNNING(x)){
             clock_unset(x->x_clock);
             x->x_nextscoretime = 0.;
             x->x_clockdelay -= clock_gettimesince(x->x_prevtime);
             x->x_clockdelay *= x->x_newtimescale / x->x_timescale;
             if(x->x_clockdelay < 0.)
                 x->x_clockdelay = 0.;
             x->x_timescale = x->x_newtimescale;
             clock_delay(x->x_clock, x->x_clockdelay);
             x->x_prevtime = clock_getlogicaltime();
         }
     }
     // FIXME else pause, LATER reverse playback if(f < -MIDI_TEMPOEPSILON)
 }

static int midi_eventcomparehook(const void *e1, const void *e2){
    return(((t_midievent *)e1)->e_delta > ((t_midievent *)e2)->e_delta ? 1 : -1);
}

static int midi_tempocomparehook(const void *t1, const void *t2){
    return(((t_miditempo *)t1)->t_scoretime > ((t_miditempo *)t2)->t_scoretime ? 1 : -1);
}

static int midi_mrhook(t_mifiread *mr, void *hookdata, int evtype){
    t_midi *x = (t_midi *)hookdata;
    double scoretime = mifiread_getscoretime(mr);
    if((evtype >= 0x80 && evtype < 0xf0) || (evtype == MIFIMETA_EOT)){
        if(x->x_eventreadhead < x->x_nevents){
            t_midievent *sev = &x->x_sequence[x->x_eventreadhead++];
            int status = mifiread_getstatus(mr);
            sev->e_delta = scoretime;
            sev->e_bytes[0] = status | mifiread_getchannel(mr);
            sev->e_bytes[1] = mifiread_getdata1(mr);
            if(MIFI_ONEDATABYTE(status) || evtype == 0x2f)
                sev->e_bytes[2] = MIDI_META;
            else{
                sev->e_bytes[2] = mifiread_getdata2(mr);
                sev->e_bytes[3] = MIDI_META;
            }
        }
        else if(x->x_eventreadhead == x->x_nevents){
            pd_error(x, "bug [midi]: midi_mrhook 1");
            x->x_eventreadhead++;
        }
    }
    else if(evtype == MIFIMETA_TEMPO){
        if(x->x_temporeadhead < x->x_ntempi){
            t_miditempo *stm = &x->x_tempomap[x->x_temporeadhead++];
            stm->t_scoretime = scoretime;
            stm->t_sr = mifiread_gettempo(mr);
        }
        else if(x->x_temporeadhead == x->x_ntempi){
            pd_error(x, "bug [midi]: midi_mrhook 2");
            x->x_temporeadhead++;
        }
    }
    return(1);
}

/* apply tempo and fold */
static void midi_foldtime(t_midi *x, double deftempo){
    t_midievent *sev;
    t_miditempo *stm = x->x_tempomap;
    double coef = 1000. / deftempo;
    int ex, tx = 0;
    double prevscoretime = 0.;
    while(tx < x->x_ntempi && stm->t_scoretime < MIDI_TICKEPSILON)
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

static int midi_mfread(t_midi *x, char *path){
    int result = 0;
    t_mifiread *mr = mifiread_new((t_pd *)x);
    if(!mifiread_open(mr, path, "", 0))
        goto mfreadfailed;
    if(!midi_dogrowing(x, mifiread_getnevents(mr), mifiread_getntempi(mr)))
        goto mfreadfailed;
    x->x_eventreadhead = 0;
    x->x_temporeadhead = 0;
    if(mifiread_doit(mr, midi_mrhook, x) != MIFIREAD_EOF)
        goto mfreadfailed;
    if(x->x_eventreadhead < x->x_nevents){
        pd_error(x, "bug [midi]: midi_mfread 1");
        post("declared %d events, got %d", x->x_nevents, x->x_eventreadhead);
        x->x_nevents = x->x_eventreadhead;
    }
    if(x->x_nevents)
        qsort(x->x_sequence, x->x_nevents, sizeof(*x->x_sequence), midi_eventcomparehook);
    if(x->x_temporeadhead < x->x_ntempi){
        pd_error(x, "bug [midi]: midi_mfread 2");
        post("declared %d tempi, got %d", x->x_ntempi, x->x_temporeadhead);
        x->x_ntempi = x->x_temporeadhead;
    }
    if(x->x_ntempi)
        qsort(x->x_tempomap, x->x_ntempi, sizeof(*x->x_tempomap), midi_tempocomparehook);
    midi_foldtime(x, mifiread_getdeftempo(mr));
    result = 1;
mfreadfailed:
    mifiread_free(mr);
    return(result);
}

static void midi_click(t_midi *x){
    elsefile_panel_click_open(x->x_elsefilehandle);
}

static int midi_mfwrite(t_midi *x, char *path){
    int result = 0;
    t_midievent *sev = x->x_sequence;
    int nevents = x->x_nevents;
    t_mifiwrite *mw = mifiwrite_new((t_pd *)x);
    if(!mifiwrite_open(mw, path, "", 1, 1))
        goto mfwritefailed;
    if(!mifiwrite_opentrack(mw, "midi-track", 1))
        goto mfwritefailed;
    while(nevents--){
        unsigned char *bp = sev->e_bytes;
        unsigned status = *bp & 0xf0;
        if(status > 127 && status < 240){
            if(!mifiwrite_channelevent(mw, sev->e_delta, status, *bp & 0x0f, bp[1], bp[2])){  /* MIDI_META ignored */
                pd_error(x, "[midi] cannot write channel event %d", status);
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
        post("while saving sequence into midi elsefile \"%s\"", path);
    mifiwrite_free(mw);
    return(result);
}

/* CHECKED text elsefile input: absolute timestamps, semi-terminated, verified */
/* FIXME prevent loading .pd elsefiles... */
static int midi_fromatoms(t_midi *x, int ac, t_atom *av){
    int i, nevents = 0;
    t_atom *ap;
    for(i = 0, ap = av; i < ac; i++, ap++)
        if(ap->a_type == A_SEMI)  /* FIXME parsing */
            nevents++;
    if(nevents){
        t_midievent *ep;
        if(!midi_dogrowing(x, nevents, 0))
            return(0);
        i = -1;
        nevents = 0;
        ep = x->x_sequence;
        while(ac--){
            if(av->a_type == A_FLOAT){
                if(i < 0){
                    ep->e_delta = av->a_w.w_float;
                    i = 0;
                }
                else if(i < 4)
                    ep->e_bytes[i++] = av->a_w.w_float;
                // CHECKME else
            }
            else if(av->a_type == A_SEMI && i > 0){
                if(i < 4)
                    ep->e_bytes[i] = MIDI_META;
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

static void midi_textread(t_midi *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    if(binbuf_read(bb, path, "", 0)) // CHECKED no complaint, open dialog presented
        elsefile_panel_click_open(x->x_elsefilehandle);  // LATER rethink
    else{
        int nlines = /* CHECKED absolute timestamps */
            midi_fromatoms(x, binbuf_getnatom(bb), binbuf_getvec(bb));
        if(nlines < 0) // "bad MIDI elsefile (truncated)" alert, even if a text elsefile
            pd_error(x, "[midi]: bad text elsefile (truncated)");
        else if(nlines == 0){ // no complaint, sequence erased, LATER rethink
        }
    }
    binbuf_free(bb);
}

static void midi_tobinbuf(t_midi *x, t_binbuf *bb){
    int nevents = x->x_nevents;
    t_midievent *ep = x->x_sequence;
    t_atom at[5];
    while(nevents--){
        unsigned char *bp = ep->e_bytes;
        int i;
        t_atom *ap = at;
        SETFLOAT(ap, ep->e_delta);  // CHECKED same for sysex continuation
        ap++;
        SETFLOAT(ap, *bp);
        for(i = 0, ap++, bp++; i < 3 && *bp != MIDI_META; i++, ap++, bp++)
            SETFLOAT(ap, *bp);
        binbuf_add(bb, i + 2, at);
        binbuf_addsemi(bb);
        ep++;
    }
}

static void midi_textwrite(t_midi *x, char *path){
    t_binbuf *bb;
    bb = binbuf_new();
    midi_tobinbuf(x, bb);
    // empty sequence stored as an empty elsefile
    if(binbuf_write(bb, path, "", 0)) // CHECKME complaint and FIXME
        pd_error(x, "[midi]: error writing text elsefile");
    binbuf_free(bb);
}

static void midi_doread(t_midi *x, t_symbol *fn){
    static char fname[MAXPDSTRING];
    char *bufptr;
    int fd = canvas_open(x->x_canvas, fn->s_name, "", fname, &bufptr, MAXPDSTRING, 1);
    if(fd < 0){
        post("[midi] file '%s' not found", fn->s_name);
        return;
    }
    else{
        fname[strlen(fname)]='/';
        sys_close(fd);
    }
    if(!midi_mfread(x, fname))
        midi_textread(x, fname);
    x->x_playhead = 0;
}

static void midi_dowrite(t_midi *x, t_symbol *fn){
    char buf[MAXPDSTRING];
    if(x->x_canvas)
        canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    else{
        strncpy(buf, fn->s_name, MAXPDSTRING);
        buf[MAXPDSTRING-1] = 0;
    }
    char *dotp = strrchr(fn->s_name, '.');
    if(dotp){
        if(!strcmp(dotp + 1, "txt")){
           midi_textwrite(x, buf);
            return;
        }
        else if(!strcmp(dotp + 1, "mid")){
            midi_mfwrite(x, buf);
            return;
        }
    }
    pd_error(x, "[midi]: can only save to .txt or .mid");
}

static void midi_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    midi_doread((t_midi *)z, fn);
}

static void midi_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    midi_dowrite((t_midi *)z, fn);
}

static void midi_read(t_midi *x, t_symbol *s){
    if(s && s != &s_)
        midi_doread(x, s);
    else
        elsefile_panel_click_open(x->x_elsefilehandle);
}

static void midi_write(t_midi *x, t_symbol *s){
    if(s && s != &s_)
        midi_dowrite(x, s);
    else  // creation arg is a default elsefile name
        elsefile_panel_save(x->x_elsefilehandle, canvas_getdir(x->x_canvas), x->x_defname); // always start in canvas dir
}

static void midi_free(t_midi *x){
    if(x->x_clock)
        clock_free(x->x_clock);
    if(x->x_slaveclock)
        clock_free(x->x_slaveclock);
    if(x->x_elsefilehandle)
        elsefile_free(x->x_elsefilehandle);
    if(x->x_sequence != x->x_midiini)
        freebytes(x->x_sequence, x->x_midisize * sizeof(*x->x_sequence));
    if(x->x_tempomap != x->x_tempomapini)
        freebytes(x->x_tempomap, x->x_tempomapsize * sizeof(*x->x_tempomap));
}

static void *midi_new(t_symbol * s, int ac, t_atom *av){
    t_midi *x = (t_midi *)pd_new(midi_class);
    x->x_canvas = canvas_getcurrent();
    x->x_elsefilehandle = elsefile_new((t_pd *)x, midi_readhook, midi_writehook);
    x->x_timescale = 1.;
    x->x_newtimescale = 1.;
    x->x_prevtime = 0.;
    x->x_slaveprevtime = 0.;
    x->x_midisize = MIDI_INISEQSIZE;
    x->x_nevents = 0;
    x->x_loop = 0;
    x->x_sequence = x->x_midiini;
    x->x_tempomapsize = MIDI_INITEMPOMAPSIZE;
    x->x_ntempi = 0;
    x->x_tempomap = x->x_tempomapini;
    x->x_defname = &s_;
    int argn = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            s = atom_getsymbolarg(0, ac, av);
            if(s == gensym("-loop") && !argn){
                x->x_loop = 1;
                ac--, av++;
            }
            else{
                argn = 1;
                midi_doread(x, x->x_defname = s);
                ac--, av++;
            }
        }
    };
    x->x_clock = clock_new(x, (t_method)midi_clocktick);
    x->x_slaveclock = clock_new(x, (t_method)midi_slaveclocktick);
    outlet_new((t_object *)x, &s_anything);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
// panic
    x->x_note_status = 0;
    x->x_pitch = PANIC_VOID;
    midi_panic_clear(x);
    return(x);
}

void midi_setup(void){
    midi_class = class_new(gensym("midi"), (t_newmethod)midi_new,
        (t_method)midi_free, sizeof(t_midi), 0, A_GIMME, 0);
    class_addbang(midi_class, midi_tick);
    class_addfloat(midi_class, midi_float);
    class_addmethod(midi_class, (t_method)midi_clear, gensym("clear"), 0);
    class_addmethod(midi_class, (t_method)midi_record, gensym("record"), 0);
    class_addmethod(midi_class, (t_method)midi_play, gensym("play"), 0);
    class_addmethod(midi_class, (t_method)midi_start, gensym("start"), 0);
    class_addmethod(midi_class, (t_method)midi_loop, gensym("loop"), A_DEFFLOAT, 0);
    class_addmethod(midi_class, (t_method)midi_stop, gensym("stop"), 0);
    class_addmethod(midi_class, (t_method)midi_read, gensym("open"), A_DEFSYM, 0);
    class_addmethod(midi_class, (t_method)midi_write, gensym("save"), A_DEFSYM, 0);
    class_addmethod(midi_class, (t_method)midi_panic, gensym("panic"), 0);
    class_addmethod(midi_class, (t_method)midi_dump, gensym("dump"), 0);
    class_addmethod(midi_class, (t_method)midi_pause, gensym("pause"), 0);
    class_addmethod(midi_class, (t_method)midi_continue, gensym("continue"), 0);
    class_addmethod(midi_class, (t_method)midi_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
//    class_addmethod(midi_class, (t_method)midi_goto, gensym("goto"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(midi_class, (t_method)midi_speed, gensym("speed"), A_FLOAT, 0);;
    elsefile_setup();
}
