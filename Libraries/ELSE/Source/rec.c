// based on cyclone/mtr

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#include "m_pd.h"
#include "elsefile.h"
#include <stdlib.h>
#include <string.h>

#ifdef FLT_MAX
#define SHARED_FLT_MAX  FLT_MAX
#else
#define SHARED_FLT_MAX  1E+36
#endif

#define REC_MAXTRACKS       64
#define REC_FILEBUFSIZE     4096
#define REC_FILEMAXCOLUMNS  78

enum{REC_STOPMODE, REC_RECMODE, REC_PLAYMODE};

typedef struct _rec_track{
    t_pd           tr_pd;
    struct _rec   *tr_owner;
    int            tr_id;
    int            tr_listed;
    int            tr_mode;
    int            tr_muted;
    int            tr_restarted;
    t_atom        *tr_atdelta;
    int            tr_ixnext;
    t_binbuf      *tr_binbuf;
    float          tr_tempo;
    double         tr_clockdelay;
    double         tr_prevtime;
    t_clock       *tr_clock;
    t_outlet      *tr_trackout;
}t_rec_track;

typedef void (*t_rec_trackfn)(t_rec_track *tp);

typedef struct _rec{
    t_object       x_obj;
    t_canvas      *x_canvas;
    int            x_ntracks;  
    t_rec_track  **x_tracks;
    t_elsefile    *x_elsefilehandle;
    t_outlet      *x_bangout;
}t_rec;

static t_class *rec_track_class;
static t_class *rec_class;

static void check_EOT(t_rec *x){
    int ntracks = x->x_ntracks;
    t_rec_track **tpp = x->x_tracks;
    while(ntracks--){
        if((*tpp++)->tr_mode != REC_STOPMODE)
            return;
    }
    outlet_bang(x->x_bangout);
}

static void rec_track_donext(t_rec_track *tp){
    if(tp->tr_ixnext < 0)
        goto endoftrack;
    while(1){
        int natoms = binbuf_getnatom(tp->tr_binbuf);
        int ixmess = tp->tr_ixnext;
        t_atom *atmess;
        int last_item = (ixmess >= (natoms - 2));
        if(ixmess >= natoms)
            goto endoftrack;
        atmess = binbuf_getvec(tp->tr_binbuf) + ixmess;
        while(atmess->a_type == A_SEMI){
            if(++ixmess >= natoms)
                goto endoftrack;
            atmess++;
        }
        if(!tp->tr_atdelta && atmess->a_type == A_FLOAT){  // delta atom
            float delta = atmess->a_w.w_float;
            if(delta < 0.)
                delta = 0.;
            tp->tr_atdelta = atmess;
            tp->tr_ixnext = ixmess + 1;
            if(tp->tr_mode == REC_PLAYMODE){
                clock_delay(tp->tr_clock, tp->tr_clockdelay = delta * tp->tr_tempo);
                tp->tr_prevtime = clock_getlogicaltime();
            }
            else if(ixmess < 2)  // LATER rethink
                continue;  // CHECKED first delta skipped
            else{  // CHECKED this is not blocked with the muted flag
                t_atom at[2];
                SETFLOAT(&at[0], tp->tr_id);
                SETFLOAT(&at[1], delta);
            }
            return;
        }
        else{  // message beginning
            int wasrestarted = tp->tr_restarted;  // LATER rethink
            int ixnext = ixmess + 1;
            t_atom *atnext = atmess + 1;
            while(ixnext < natoms && atnext->a_type != A_SEMI)
                ixnext++, atnext++;
            tp->tr_restarted = 0;
            if(!tp->tr_muted){
                int ac = ixnext - ixmess;
                if(!last_item){
                    if(atmess->a_type == A_FLOAT)
                        outlet_list(tp->tr_trackout, &s_list, ac, atmess);
                    else if(atmess->a_type == A_SYMBOL)
                        outlet_anything(tp->tr_trackout, atmess->a_w.w_symbol, ac-1, atmess+1);
                }
            }
            tp->tr_atdelta = 0;
            tp->tr_ixnext = ixnext;
            if(tp->tr_restarted)
                return;
            tp->tr_restarted = wasrestarted;
        }
    }
endoftrack:
    if(tp->tr_mode == REC_PLAYMODE){
        tp->tr_ixnext = 0; // ready to go in stop mode after play
    }
    tp->tr_atdelta = 0;
    tp->tr_prevtime = 0.;
    tp->tr_mode = REC_STOPMODE;
    check_EOT(tp->tr_owner);
}

static void rec_track_tick(t_rec_track *tp){
    if(tp->tr_mode == REC_PLAYMODE){
        tp->tr_prevtime = 0.;
        rec_track_donext(tp);
    }
}

static void rec_track_setmode(t_rec_track *tp, int newmode){
    if(tp->tr_mode == REC_PLAYMODE){
        clock_unset(tp->tr_clock);
        tp->tr_ixnext = 0;
    }
    switch(tp->tr_mode = newmode){
        case REC_STOPMODE:
            break;
        case REC_RECMODE:
            binbuf_clear(tp->tr_binbuf);
            tp->tr_prevtime = clock_getlogicaltime();
            break;
        case REC_PLAYMODE:
            tp->tr_atdelta = 0;
            tp->tr_ixnext = 0;
            tp->tr_prevtime = 0.;
            rec_track_donext(tp);
            break;
        default:
            post("[rec]: bug in rec_track_setmode");
    }
}

static void rec_track_doadd(t_rec_track *tp, int ac, t_atom *av){
    if(tp->tr_prevtime > 0.){
        t_binbuf *bb = tp->tr_binbuf;
        t_atom at;
    	float elapsed = clock_gettimesince(tp->tr_prevtime);
        SETFLOAT(&at, elapsed);
        binbuf_add(bb, 1, &at);
        binbuf_add(bb, ac, av);
        SETSEMI(&at);
        binbuf_add(bb, 1, &at);
        tp->tr_prevtime = clock_getlogicaltime();
    }
}

static void rec_track_bang(t_rec_track *tp){
    if(tp->tr_mode == REC_RECMODE){
        t_atom at[1];
        SETSYMBOL(&at[0], gensym("bang"));
        rec_track_doadd(tp, 1, at);
    }
}

static void rec_track_float(t_rec_track *tp, t_float f){
    if(tp->tr_mode == REC_RECMODE){
        t_atom at;
        SETFLOAT(&at, f);
        rec_track_doadd(tp, 1, &at);
    }
}

static void rec_track_symbol(t_rec_track *tp, t_symbol *s){
    if(tp->tr_mode == REC_RECMODE){
        t_atom at[2];
        SETSYMBOL(&at[0], gensym("symbol"));
        SETSYMBOL(&at[1], s);
        rec_track_doadd(tp, 2, at);
    }
}

static void rec_track_list(t_rec_track *tp, t_symbol *s, int ac, t_atom *av){
    if(!ac){
        rec_track_bang(tp);
        return;
    }
    if(ac == 1){
        if(av->a_type == A_FLOAT)
            rec_track_float(tp, atom_getfloat(av));
        else if(av->a_type == A_SYMBOL)
            rec_track_symbol(tp, atom_getsymbol(av));
        return;
    }
    if(tp->tr_mode == REC_RECMODE){
        if(av->a_type == A_FLOAT)
            rec_track_doadd(tp, ac, av);
        else{
            t_atom* at = (t_atom*)malloc(sizeof(t_atom) * (ac+1));
            SETSYMBOL(&at[0], s);
            for(int i = 0; i < ac; i++){
                if((av+i)->a_type == A_FLOAT)
                    SETFLOAT(&at[i+1], atom_getfloatarg(i, ac, av));
                else if((av+i)->a_type == A_SYMBOL)
                    SETSYMBOL(&at[i+1], atom_getsymbolarg(i, ac, av));
            }
            rec_track_doadd(tp, ac+1, at);
            free(at);
        }
    }
}

static void rec_track_anything(t_rec_track *tp, t_symbol *s, int ac, t_atom *av){
    if(tp->tr_mode == REC_RECMODE){
        t_atom* at = (t_atom*)malloc(sizeof(t_atom) * (ac+1));
        SETSYMBOL(&at[0], s);
        for(int i = 0; i < ac; i++){
            if((av+i)->a_type == A_FLOAT)
                SETFLOAT(&at[i+1], atom_getfloatarg(i, ac, av));
            else if((av+i)->a_type == A_SYMBOL)
                SETSYMBOL(&at[i+1], atom_getsymbolarg(i, ac, av));
        }
        rec_track_doadd(tp, ac+1, at);
        free(at);
    }
}

static void rec_track_record(t_rec_track *tp){
    rec_track_setmode(tp, REC_RECMODE);
}

static void rec_track_play(t_rec_track *tp){
    rec_track_setmode(tp, REC_PLAYMODE);
}

static void rec_track_stop(t_rec_track *tp){
    if(tp->tr_mode == REC_RECMODE){
        t_atom at;
        SETSYMBOL(&at, gensym("EOT"));
        rec_track_doadd(tp, 1, &at);
    }
    rec_track_setmode(tp, REC_STOPMODE);
}

// stop and play mode
static void rec_track_mute(t_rec_track *tp){
    tp->tr_muted = 1;
}

// stop and play mode
static void rec_track_unmute(t_rec_track *tp){
    tp->tr_muted = 0;
}

static void rec_track_clear(t_rec_track *tp){
    binbuf_clear(tp->tr_binbuf);
}

static void rec_track_speed(t_rec_track *tp, t_floatarg f){
    float newtempo;
    if(f < 1e-20)
        f = 1e-20;
    else if(f > 1e20)
        f = 1e20;
    newtempo = 100. / f;
    if(tp->tr_prevtime > 0.){
        tp->tr_clockdelay -= clock_gettimesince(tp->tr_prevtime);
        tp->tr_clockdelay *= newtempo / tp->tr_tempo;
        if(tp->tr_clockdelay < 0.)
            tp->tr_clockdelay = 0.;
        clock_delay(tp->tr_clock, tp->tr_clockdelay);
        tp->tr_prevtime = clock_getlogicaltime();
    }
    tp->tr_tempo = newtempo;
}

static void rec_calltracks(t_rec *x, t_rec_trackfn fn, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int ntracks = x->x_ntracks;
    t_rec_track **tpp = x->x_tracks;
    if(ac){ // FIXME: CHECKED tracks called in the order of being mentioned (without duplicates)
        while(ntracks--)
            (*tpp++)->tr_listed = 0;
        while(ac--){ // silently ignore out-of-bounds and non-ints
            if(av->a_type == A_FLOAT){
                int id = (int)av->a_w.w_float - 1;  // 1-based
                if(id >= 0 && id < x->x_ntracks)
                    x->x_tracks[id]->tr_listed = 1;
            }
            av++;
        }
        ntracks = x->x_ntracks;
        tpp = x->x_tracks;
        while(ntracks--){
            if((*tpp)->tr_listed)
                fn(*tpp);
            tpp++;
        }
    }
    else while(ntracks--)
        fn(*tpp++);
}

static void rec_speed(t_rec *x, t_floatarg f){
    int ntracks = x->x_ntracks;
    t_rec_track **tpp = x->x_tracks;
    while(ntracks--)
        rec_track_speed(*tpp++, f);
}

static void rec_record(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_record, s, ac, av);
}

static void rec_play(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_play, s, ac, av);
}

static void rec_stop(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_stop, s, ac, av);
}

static void rec_mute(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_mute, s, ac, av);
}

static void rec_unmute(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_unmute, s, ac, av);
}

static void rec_clear(t_rec *x, t_symbol *s, int ac, t_atom *av){
    rec_calltracks(x, rec_track_clear, s, ac, av);
}

static void rec_doread(t_rec *x, t_symbol *fname){
    char path[MAXPDSTRING];
    char *bufptr;
    int fd = canvas_open(x->x_canvas, fname->s_name, "", path, &bufptr, MAXPDSTRING, 1);
    if(fd > 0){
        path[strlen(path)]='/';
        sys_close(fd);
    }
    else{
        post("[rec] file '%s' not found", fname->s_name);
        return;
    }
    FILE *fp;
    if((fp = sys_fopen(path, "r"))){
        t_rec_track *tp = 0;
        char linebuf[REC_FILEBUFSIZE];
        t_binbuf *bb = binbuf_new();
        while(fgets(linebuf, REC_FILEBUFSIZE, fp)){
            char *line = linebuf;
            int linelen;
            while(*line && (*line == ' ' || *line == '\t')) line++;
            if((linelen = strlen(line))){
                if(tp){
                    if(!strncmp(line, "end;", 4))
                        tp = 0;
                    else{
                        int ac;
                        binbuf_text(bb, line, linelen);
                        if((ac = binbuf_getnatom(bb))){
                            t_atom *ap = binbuf_getvec(bb);
                            if(!binbuf_getnatom(tp->tr_binbuf)){
                                if(ap->a_type != A_FLOAT){
                                    t_atom at;
                                    SETFLOAT(&at, 0.);
                                    binbuf_add(tp->tr_binbuf, 1, &at);
                                }
                                else if(ap->a_w.w_float < 0.)
                                    ap->a_w.w_float = 0.;
                            }
                            binbuf_add(tp->tr_binbuf, ac, ap);
                        }
                    }
                }
                else if(!strncmp(line, "track ", 6)){
                    int id = strtol(line + 6, 0, 10);
                    if(id < 1 || id > x->x_ntracks)
                        post("[rec] cann't load track %d... no such track", id);  // LATER rethink
                    else
                        tp = x->x_tracks[id - 1];
                    if(tp)
                        binbuf_clear(tp->tr_binbuf);
                }
            }
        }
        fclose(fp);
        binbuf_free(bb);
    }
    else
        elsefile_panel_click_open(x->x_elsefilehandle);
}

static int rec_writetrack(t_rec *x, t_rec_track *tp, FILE *fp){
    x = NULL;
    int natoms = binbuf_getnatom(tp->tr_binbuf);
    if(natoms) { /* CHECKED empty tracks not stored */
        char sbuf[REC_FILEBUFSIZE], *bp = sbuf, *ep = sbuf + REC_FILEBUFSIZE;
        int ncolumn = 0;
        t_atom *ap = binbuf_getvec(tp->tr_binbuf);
        fprintf(fp, "track %d;\n", tp->tr_id);
        for(; natoms--; ap++){
            int length;
            // from binbuf_write():
	       // ``estimate how many characters will be needed.  Printing out
	       // symbols may need extra characters for inserting backslashes.''
            if(ap->a_type == A_SYMBOL || ap->a_type == A_DOLLSYM)
                length = 80 + strlen(ap->a_w.w_symbol->s_name);
            else
                length = 40;
            if(bp > sbuf && ep - bp < length){
                if(fwrite(sbuf, bp - sbuf, 1, fp) < 1)
                    return(1);
                bp = sbuf;
            }
            if(ap->a_type == A_SEMI){
                *bp++ = ';';
                *bp++ = '\n';
                ncolumn = 0;
            }
            else if(ap->a_type == A_COMMA){
                *bp++ = ',';
                ncolumn++;
            }
            else{
                if(ncolumn){
                    *bp++ = ' ';
                    ncolumn++;
                }
                atom_string(ap, bp, (ep - bp) - 2);
                length = strlen(bp);
                if(ncolumn && ncolumn + length > REC_FILEMAXCOLUMNS){
                    bp[-1] = '\n';
                    ncolumn = length;
                }
                else
                    ncolumn += length;
                bp += length;
            }
        }
        if(bp > sbuf && fwrite(sbuf, bp - sbuf, 1, fp) < 1)
            return(1);
        fputs("end;\n", fp);
//        post("Track %d done", tp->tr_id);
    }
    return(0);
}

// CHECKED empty sequence stored as an empty elsefile
static void rec_dowrite(t_rec *x, t_symbol *fname){
    int failed = 0;
    char path[MAXPDSTRING];
    FILE *fp;
    if(x->x_canvas)
        canvas_makefilename(x->x_canvas, fname->s_name, path, MAXPDSTRING);
    else{
    	strncpy(path, fname->s_name, MAXPDSTRING);
    	path[MAXPDSTRING-1] = 0;
    }
    if((fp = sys_fopen(path, "w"))){
        int id;  // single-track writing does not seem to work (a bug?)
        t_rec_track **tpp;
        for(id = 0, tpp = x->x_tracks; id < x->x_ntracks; id++, tpp++)
            if((failed = rec_writetrack(x, *tpp, fp)))
                break;
        fclose(fp);
    }
    else
        failed = 1;
    if(failed)
        pd_error(x, "[rec]: writing text elsefile \"%s\" failed", path);
}

static void rec_readhook(t_pd *z, t_symbol *fname, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    rec_doread((t_rec *)z, fname);
}

static void rec_writehook(t_pd *z, t_symbol *fname, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    rec_dowrite((t_rec *)z, fname);
}

static void rec_read(t_rec *x, t_symbol *s){
    if(s && s != &s_)
        rec_doread(x, s);
    else
        elsefile_panel_click_open(x->x_elsefilehandle);
}

static void rec_write(t_rec *x, t_symbol *s){
    if(s && s != &s_)
        rec_dowrite(x, s);
    else
        elsefile_panel_save(x->x_elsefilehandle, canvas_getdir(x->x_canvas), 0);
}

static void rec_click(t_rec *x){
    elsefile_panel_click_open(x->x_elsefilehandle);
}

static void rec_free(t_rec *x){
    if(x->x_tracks){
        int ntracks = x->x_ntracks;
        t_rec_track **tpp = x->x_tracks;
        while(ntracks--){
            t_rec_track *tp = *tpp++;
            if(tp->tr_binbuf)
                binbuf_free(tp->tr_binbuf);
            if(tp->tr_clock)
                clock_free(tp->tr_clock);
	    pd_free((t_pd *)tp);
        }
	freebytes(x->x_tracks, x->x_ntracks * sizeof(*x->x_tracks));
    }
}

static void *rec_new(t_symbol * s, int ac, t_atom *av){
    s = NULL;
    t_rec *x = (t_rec *)pd_new(rec_class);;
    int ntracks = 1;
    t_symbol *name = &s_;
    if(ac == 1 || ac == 2){
        if(av->a_type == A_FLOAT){
            ntracks = (int)atom_getfloatarg(0, ac, av);
            if(ntracks < 1)
                ntracks = 1;
            ac--, av++;
            if(ac && av->a_type == A_SYMBOL){
                name = atom_getsymbolarg(0, ac, av);
                ac--, av++;
            }
        }
    }
    t_rec_track **tracks = getbytes(ntracks * sizeof(*tracks));
    t_rec_track **tpp;
    int i;
    for(i = 0, tpp = tracks; i < ntracks; i++, tpp++){
        *tpp = (t_rec_track *)pd_new(rec_track_class);
        (*tpp)->tr_binbuf = binbuf_new();
        (*tpp)->tr_clock = clock_new(*tpp, (t_method)rec_track_tick);
    }
    x->x_canvas = canvas_getcurrent();
    x->x_elsefilehandle = elsefile_new((t_pd *)x, rec_readhook, rec_writehook);
    if(ntracks > REC_MAXTRACKS)
        ntracks = REC_MAXTRACKS;
    x->x_ntracks = ntracks;
    x->x_tracks = tracks;
    for(int id = 1; id <= ntracks; id++, tracks++){ // 1-based
        t_rec_track *tp = *tracks;
        inlet_new((t_object *)x, (t_pd *)tp, 0, 0);
        tp->tr_trackout = outlet_new((t_object *)x, &s_);
        tp->tr_owner = x;
        tp->tr_id = id;
        tp->tr_listed = 0;
        tp->tr_mode = REC_STOPMODE;
        tp->tr_muted = 0;
        tp->tr_restarted = 0;
        tp->tr_atdelta = 0;
        tp->tr_ixnext = 0;
        tp->tr_tempo = 1.;
        tp->tr_clockdelay = 0.;
        tp->tr_prevtime = 0.;
    }
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    if(name != &s_)
        rec_doread(x, name);
    return(x);
}

void rec_setup(void){
    rec_track_class = class_new(gensym("_rec_track"), 0, 0,
        sizeof(t_rec_track), CLASS_PD | CLASS_NOINLET, 0);
    class_addlist(rec_track_class, rec_track_list);
    class_addanything(rec_track_class, rec_track_anything);
    rec_class = class_new(gensym("rec"), (t_newmethod)rec_new,
        (t_method)rec_free, sizeof(t_rec), 0, A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_record, gensym("record"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_play, gensym("play"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_stop, gensym("stop"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_mute, gensym("mute"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_unmute, gensym("unmute"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_clear, gensym("clear"), A_GIMME, 0);
    class_addmethod(rec_class, (t_method)rec_read, gensym("open"), A_DEFSYM, 0);
    class_addmethod(rec_class, (t_method)rec_write, gensym("save"), A_DEFSYM, 0);
    class_addmethod(rec_class, (t_method)rec_speed, gensym("speed"), A_DEFFLOAT, 0);
    class_addmethod(rec_class, (t_method)rec_click, gensym("click"), 0);
    elsefile_setup();
}
