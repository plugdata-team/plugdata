/* Copyright (c) 2003-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKME undocumented: readbinbuf, writebinbuf (a clipboard-like thing?) */

//Derek Kwan 2016 - changing mtrack_list to mtrack_anything, fixing issues with first elts interpreted as selectors, changing mtrack_float and mtrack_symbol to feed through mtrack_anything, changing mtrack_donext to treat everything as a list

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/shared.h"
#include "common/file.h"

#define MTR_C74MAXTRACKS    64
#define MTR_FILEBUFSIZE   4096
#define MTR_FILEMAXCOLUMNS  78

enum { MTR_STEPMODE, MTR_RECMODE, MTR_PLAYMODE };

typedef struct _mtrack
{
    t_pd           tr_pd;
    struct _mtr   *tr_owner;
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
    t_outlet      *tr_mainout;
    t_file  *tr_filehandle;
} t_mtrack;

typedef void (*t_mtrackfn)(t_mtrack *tp);

typedef struct _mtr
{
    t_object       x_ob;
    t_canvas       *x_cnv;
    int            x_ntracks;  
    t_mtrack     **x_tracks;
    t_file  *x_filehandle;
} t_mtr;

static t_class *mtrack_class;
static t_class *mtr_class;

static void mtrack_donext(t_mtrack *tp)
{
    if (tp->tr_ixnext < 0)
	goto endoftrack;
    while (1)
    {
    	int natoms = binbuf_getnatom(tp->tr_binbuf);
	int ixmess = tp->tr_ixnext;
    	t_atom *atmess;
	if (ixmess >= natoms)
	    goto endoftrack;
	atmess = binbuf_getvec(tp->tr_binbuf) + ixmess;

	while (atmess->a_type == A_SEMI)
    	{
    	    if (++ixmess >= natoms)
		goto endoftrack;
	    atmess++;
	}
	if (!tp->tr_atdelta && atmess->a_type == A_FLOAT)
	{  /* delta atom */
	    float delta = atmess->a_w.w_float;
	    if (delta < 0.)
		delta = 0.;
	    tp->tr_atdelta = atmess;
    	    tp->tr_ixnext = ixmess + 1;
	    if (tp->tr_mode == MTR_PLAYMODE)
	    {
		clock_delay(tp->tr_clock,
			    tp->tr_clockdelay = delta * tp->tr_tempo);
		tp->tr_prevtime = clock_getlogicaltime();
	    }
	    else if (ixmess < 2)  /* LATER rethink */
		continue;  /* CHECKED first delta skipped */
	    else
	    {  /* CHECKED this is not blocked with the muted flag */
		t_atom at[2];
		SETFLOAT(&at[0], tp->tr_id);
		SETFLOAT(&at[1], delta);
		outlet_list(tp->tr_mainout, 0, 2, at);
	    }
    	    return;
    	}
	else
	{  /* message beginning */
	    int wasrestarted = tp->tr_restarted;  /* LATER rethink */
	    int ixnext = ixmess + 1;
	    t_atom *atnext = atmess + 1;
	    while (ixnext < natoms && atnext->a_type != A_SEMI)
		ixnext++, atnext++;
	    tp->tr_restarted = 0;
	    if (!tp->tr_muted)
	    {
		int ac = ixnext - ixmess;
		    if (atmess->a_type == A_FLOAT)
			outlet_list(tp->tr_trackout, &s_list, ac, atmess);
		    else if (atmess->a_type == A_SYMBOL)
			outlet_anything(tp->tr_trackout,
					atmess->a_w.w_symbol, ac-1, atmess+1);
	    }
	    tp->tr_atdelta = 0;
	    tp->tr_ixnext = ixnext;
	    if (tp->tr_restarted)
		return;
	    tp->tr_restarted = wasrestarted;
	}
    }
endoftrack:
    if (tp->tr_mode == MTR_PLAYMODE)
	tp->tr_ixnext = 0;   /* CHECKED ready to go in step mode after play */
    else
    {
	if (tp->tr_ixnext > 0)
	{
	    t_atom at[2];
	    SETFLOAT(&at[0], tp->tr_id);
	    SETFLOAT(&at[1], -1.);  /* CHECKED eot marker */
	    outlet_list(tp->tr_mainout, 0, 2, at);
	}
	tp->tr_ixnext = -1;  /* CHECKED no loop-over */
    }
    tp->tr_atdelta = 0;
    tp->tr_prevtime = 0.;
    tp->tr_mode = MTR_STEPMODE;
}

static void mtrack_tick(t_mtrack *tp)
{
    if (tp->tr_mode == MTR_PLAYMODE)
    {
	tp->tr_prevtime = 0.;
	mtrack_donext(tp);
    }
}

static void mtrack_setmode(t_mtrack *tp, int newmode){
    if (tp->tr_mode == MTR_PLAYMODE){
        clock_unset(tp->tr_clock);
        tp->tr_ixnext = 0;
    }
    switch(tp->tr_mode = newmode){
        case MTR_STEPMODE:
            break;
        case MTR_RECMODE:
            binbuf_clear(tp->tr_binbuf);
            tp->tr_prevtime = clock_getlogicaltime();
            break;
        case MTR_PLAYMODE:
            tp->tr_atdelta = 0;
            tp->tr_ixnext = 0;
            tp->tr_prevtime = 0.;
            mtrack_donext(tp);
            break;
        default:
            post("[mtr]: bug in mtrack_setmode");
    }
}

static void mtrack_doadd(t_mtrack *tp, int ac, t_atom *av)
{
    if (tp->tr_prevtime > 0.)
    {
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

static void mtrack_anything(t_mtrack *tp, t_symbol *s, int ac, t_atom *av){
    if(tp->tr_mode == MTR_RECMODE){
        t_atom at[ac+1];
        SETSYMBOL(&at[0], s);
        for(int i = 0; i < ac; i++){
            if((av+i)->a_type == A_FLOAT)
                SETFLOAT(&at[i+1], atom_getfloatarg(i, ac, av));
            else if((av+i)->a_type == A_SYMBOL)
                SETSYMBOL(&at[i+1], atom_getsymbolarg(i, ac, av));
        }
        mtrack_doadd(tp, ac+1, at);
    }
}


static void mtrack_float(t_mtrack *tp, t_float f){
    if(tp->tr_mode == MTR_RECMODE){
        t_atom at;
        SETFLOAT(&at, f);
        mtrack_doadd(tp, 1, &at);
    }
}

static void mtrack_symbol(t_mtrack *tp, t_symbol *s){
    if(tp->tr_mode == MTR_RECMODE){
        t_atom at[2];
        SETSYMBOL(&at[0], gensym("symbol"));
        SETSYMBOL(&at[1], s);
        mtrack_doadd(tp, 2, at);
    }
}

static void mtrack_list(t_mtrack *tp, t_symbol *s, int ac, t_atom *av){
    if(tp->tr_mode == MTR_RECMODE){
        if(av->a_type == A_FLOAT)
            mtrack_doadd(tp, ac, av);
        else{
            t_atom at[ac+1];
            SETSYMBOL(&at[0], s);
            for(int i = 0; i < ac; i++){
                if((av+i)->a_type == A_FLOAT)
                    SETFLOAT(&at[i+1], atom_getfloatarg(i, ac, av));
                else if((av+i)->a_type == A_SYMBOL)
                    SETSYMBOL(&at[i+1], atom_getsymbolarg(i, ac, av));
            }
            mtrack_doadd(tp, ac+1, at);
        }
    }
}

static void mtrack_bang(t_mtrack *tp){
    if(tp->tr_mode == MTR_RECMODE){
        t_atom at[1];
        SETSYMBOL(&at[0], gensym("bang"));
        mtrack_doadd(tp, 1, at);
    }
}

static void mtrack_record(t_mtrack *tp)
{
    mtrack_setmode(tp, MTR_RECMODE);
}

static void mtrack_play(t_mtrack *tp)
{
    mtrack_setmode(tp, MTR_PLAYMODE);
}

static void mtrack_stop(t_mtrack *tp)
{
    mtrack_setmode(tp, MTR_STEPMODE);
}

static void mtrack_next(t_mtrack *tp)
{
    if (tp->tr_mode == MTR_STEPMODE)
	mtrack_donext(tp);
}

static void mtrack_rewind(t_mtrack *tp)
{
    if (tp->tr_mode == MTR_STEPMODE)
    {
	tp->tr_atdelta = 0;
	tp->tr_ixnext = 0;
    }
}

/* CHECKED step and play mode */
static void mtrack_mute(t_mtrack *tp)
{
    tp->tr_muted = 1;
}

/* CHECKED step and play mode */
static void mtrack_unmute(t_mtrack *tp)
{
    tp->tr_muted = 0;
}

static void mtrack_clear(t_mtrack *tp)
{
    binbuf_clear(tp->tr_binbuf);
}

static t_atom *mtrack_getdelay(t_mtrack *tp){
    int natoms = binbuf_getnatom(tp->tr_binbuf);
    if (natoms){
        t_atom *ap = binbuf_getvec(tp->tr_binbuf);
        while (natoms--){
            if (ap->a_type == A_FLOAT)
                return (ap);
                ap++;
        }
        post("[mtr]: bug in mtrack_getdelay");
    }
    return (0);
}

static void mtrack_delay(t_mtrack *tp, t_floatarg f)
{
    t_atom *ap = mtrack_getdelay(tp);
    if (ap)
	ap->a_w.w_float = f;
}

static void mtrack_first(t_mtrack *tp, t_floatarg f)
{
    mtrack_delay(tp, f);  /* CHECKED */
}

static void mtr_doread(t_mtr *x, t_mtrack *target, t_symbol *fname);
static void mtr_dowrite(t_mtr *x, t_mtrack *source, t_symbol *fname);

static void mtrack_readhook(t_pd *z, t_symbol *fname, int ac, t_atom *av)
{
    t_mtrack *tp = (t_mtrack *)z;
    mtr_doread(tp->tr_owner, tp, fname);
}

static void mtrack_writehook(t_pd *z, t_symbol *fname, int ac, t_atom *av)
{
    t_mtrack *tp = (t_mtrack *)z;
    mtr_dowrite(tp->tr_owner, tp, fname);
}

static void mtrack_read(t_mtrack *tp, t_symbol *s)
{
    if (s && s != &s_)
	mtr_doread(tp->tr_owner, tp, s);
    else  /* CHECKED no default */
	panel_open(tp->tr_filehandle, 0);
}

static void mtrack_write(t_mtrack *tp, t_symbol *s)
{
    if (s && s != &s_)
	mtr_dowrite(tp->tr_owner, tp, s);
    else  /* CHECKED no default */
	panel_save(tp->tr_filehandle,
			 canvas_getdir(tp->tr_owner->x_cnv), 0);
}

static void mtrack_tempo(t_mtrack *tp, t_floatarg f)
{
    float newtempo;
    if (f < 1e-20)
	f = 1e-20;
    else if (f > 1e20)
	f = 1e20;
    newtempo = 1. / f;
    if (tp->tr_prevtime > 0.)
    {
    	tp->tr_clockdelay -= clock_gettimesince(tp->tr_prevtime);
	tp->tr_clockdelay *= newtempo / tp->tr_tempo;
	if (tp->tr_clockdelay < 0.)
	    tp->tr_clockdelay = 0.;
    	clock_delay(tp->tr_clock, tp->tr_clockdelay);
	tp->tr_prevtime = clock_getlogicaltime();
    }
    tp->tr_tempo = newtempo;
}

static void mtr_calltracks(t_mtr *x, t_mtrackfn fn,
			   t_symbol *s, int ac, t_atom *av)
{
    int ntracks = x->x_ntracks;
    t_mtrack **tpp = x->x_tracks;
    if (ac)
    {
	/* FIXME: CHECKED tracks called in the order of being mentioned
	   (without duplicates) */
	while (ntracks--) (*tpp++)->tr_listed = 0;
	while (ac--)
	{
	    /* CHECKED silently ignoring out-of-bounds and non-ints */
	    if (av->a_type == A_FLOAT)
	    {
		int id = (int)av->a_w.w_float - 1;  /* CHECKED 1-based */
		if (id >= 0 && id < x->x_ntracks)
		    x->x_tracks[id]->tr_listed = 1;
	    }
	    av++;
	}
	ntracks = x->x_ntracks;
	tpp = x->x_tracks;
	while (ntracks--)
	{
	    if ((*tpp)->tr_listed) fn(*tpp);
	    tpp++;
	}
    }
    else while (ntracks--) fn(*tpp++);
}

static void mtr_record(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_record, s, ac, av);
}

static void mtr_play(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_play, s, ac, av);
}

static void mtr_stop(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_stop, s, ac, av);
}

static void mtr_next(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_next, s, ac, av);
}

static void mtr_rewind(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_rewind, s, ac, av);
}

static void mtr_mute(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_mute, s, ac, av);
}

static void mtr_unmute(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_unmute, s, ac, av);
}

static void mtr_clear(t_mtr *x, t_symbol *s, int ac, t_atom *av)
{
    mtr_calltracks(x, mtrack_clear, s, ac, av);
}

static void mtr_delay(t_mtr *x, t_floatarg f)
{
    int ntracks = x->x_ntracks;
    t_mtrack **tpp = x->x_tracks;
    while (ntracks--) mtrack_delay(*tpp++, f);
}

static void mtr_first(t_mtr *x, t_floatarg f)
{
    int ntracks = x->x_ntracks;
    t_mtrack **tpp = x->x_tracks;
    float delta = SHARED_FLT_MAX;
    if (f < 0.)
	f = 0.;
    while (ntracks--)
    {
	t_atom *ap = mtrack_getdelay(*tpp);
	if (ap)
	{
	    if (delta > ap->a_w.w_float)
		delta = ap->a_w.w_float;
	    (*tpp)->tr_listed = 1;
	}
	else (*tpp)->tr_listed = 0;
	tpp++;
    }
    ntracks = x->x_ntracks;
    tpp = x->x_tracks;
    delta -= f;
    while (ntracks--)
    {
	if ((*tpp)->tr_listed)
	{
	    t_atom *ap = mtrack_getdelay(*tpp);
	    if (ap)
		ap->a_w.w_float -= delta;
	}
	tpp++;
    }
}

static void mtr_doread(t_mtr *x, t_mtrack *target, t_symbol *fname){
    char path[MAXPDSTRING];
    char *bufptr;
/*    if(x->x_cnv)
        canvas_makefilename(x->x_cnv, fname->s_name, path, MAXPDSTRING);
    else{
    	strncpy(path, fname->s_name, MAXPDSTRING);
    	path[MAXPDSTRING-1] = 0;
    }*/
    int fd = canvas_open(x->x_cnv, fname->s_name, "", path, &bufptr, MAXPDSTRING, 1);
    if(fd > 0){
        path[strlen(path)]='/';
        sys_close(fd);
    }
    else{
        post("[mtr] file '%s' not found", fname->s_name);
        return;
    }
    FILE *fp;
    // CHECKED no global message
    if(fp = sys_fopen(path, "r")){
        t_mtrack *tp = 0;
        char linebuf[MTR_FILEBUFSIZE];
        t_binbuf *bb = binbuf_new();
        while (fgets(linebuf, MTR_FILEBUFSIZE, fp))
	{
	    char *line = linebuf;
	    int linelen;
	    while (*line && (*line == ' ' || *line == '\t')) line++;
	    if (linelen = strlen(line))
	    {
		if (tp)
		{
		    if (!strncmp(line, "end;", 4))
		    {
			post("ok");
			tp = 0;
		    }
		    else
		    {
			int ac;
			binbuf_text(bb, line, linelen);
			if (ac = binbuf_getnatom(bb))
			{
			    t_atom *ap = binbuf_getvec(bb);
			    if (!binbuf_getnatom(tp->tr_binbuf))
			    {
				if (ap->a_type != A_FLOAT)
				{
				    t_atom at;
				    SETFLOAT(&at, 0.);
				    binbuf_add(tp->tr_binbuf, 1, &at);
				}
				else if (ap->a_w.w_float < 0.)
				    ap->a_w.w_float = 0.;
			    }
			    binbuf_add(tp->tr_binbuf, ac, ap);
			}
		    }
		}
		else if (!strncmp(line, "track ", 6))
		{
		    int id = strtol(line + 6, 0, 10);
		    startpost("Track %d... ", id);
		    if (id < 1 || id > x->x_ntracks)
			post("no such track");  /* LATER rethink */
		    else if (target)
		    {
			if (id == target->tr_id)
			    tp = target;
			post("skipped");  /* LATER rethink */
		    }
		    else tp = x->x_tracks[id - 1];
		    if (tp)
		    {
			binbuf_clear(tp->tr_binbuf);
		    }
		}
	    }
	}
	fclose(fp);
	binbuf_free(bb);
    }
    else
    {
	/* CHECKED no complaint, open dialog not presented... */
	/* LATER rethink */
	panel_open(target ? target->tr_filehandle : x->x_filehandle, 0);
    }
}

static int mtr_writetrack(t_mtr *x, t_mtrack *tp, FILE *fp)
{
    int natoms = binbuf_getnatom(tp->tr_binbuf);
    if (natoms)  /* CHECKED empty tracks not stored */
    {
	char sbuf[MTR_FILEBUFSIZE], *bp = sbuf, *ep = sbuf + MTR_FILEBUFSIZE;
	int ncolumn = 0;
	t_atom *ap = binbuf_getvec(tp->tr_binbuf);
	fprintf(fp, "track %d;\n", tp->tr_id);
	for (; natoms--; ap++)
	{
	    int length;
	    /* from binbuf_write():
	       ``estimate how many characters will be needed.  Printing out
	       symbols may need extra characters for inserting backslashes.'' */
	    if (ap->a_type == A_SYMBOL || ap->a_type == A_DOLLSYM)
		length = 80 + strlen(ap->a_w.w_symbol->s_name);
	    else
		length = 40;
	    if (bp > sbuf && ep - bp < length)
	    {
		if (fwrite(sbuf, bp - sbuf, 1, fp) < 1)
		    return (1);
		bp = sbuf;
	    }
	    if (ap->a_type == A_SEMI)
	    {
		*bp++ = ';';
		*bp++ = '\n';
		ncolumn = 0;
	    }
	    else if (ap->a_type == A_COMMA)
	    {
		*bp++ = ',';
		ncolumn++;
	    }
	    else
	    {
		if (ncolumn)
		{
		    *bp++ = ' ';
		    ncolumn++;
		}
		atom_string(ap, bp, (ep - bp) - 2);
		length = strlen(bp);
		if (ncolumn && ncolumn + length > MTR_FILEMAXCOLUMNS)
		{
		    bp[-1] = '\n';
		    ncolumn = length;
		}
		else ncolumn += length;
		bp += length;
	    }
	}
	if (bp > sbuf && fwrite(sbuf, bp - sbuf, 1, fp) < 1)
	    return (1);
	fputs("end;\n", fp);
	post("Track %d done", tp->tr_id);  /* CHECKED (0-based: not emulated) */
    }
    return (0);
}

// CHECKED empty sequence stored as an empty file
static void mtr_dowrite(t_mtr *x, t_mtrack *source, t_symbol *fname){
    int failed = 0;
    char path[MAXPDSTRING];
    FILE *fp;
    if (x->x_cnv)
	canvas_makefilename(x->x_cnv, fname->s_name, path, MAXPDSTRING);
    else{
    	strncpy(path, fname->s_name, MAXPDSTRING);
    	path[MAXPDSTRING-1] = 0;
    }
    // CHECKED no global message
    if(fp = sys_fopen(path, "w")){ // CHECKED single-track writing does not seem to work (a bug?)
        if(source)
            failed = mtr_writetrack(x, source, fp);
        else{
            int id;
            t_mtrack **tpp;
            for(id = 0, tpp = x->x_tracks; id < x->x_ntracks; id++, tpp++)
                if(failed = mtr_writetrack(x, *tpp, fp))
                    break;
        }
//	if (failed) sys_unixerror(path);  // LATER rethink
        fclose(fp);
    }
    else{
//	sys_unixerror(path);  // LATER rethink
        failed = 1;
    }
    if(failed)
        pd_error(x, "[mtr]: writing text file \"%s\" failed", path);
}

static void mtr_readhook(t_pd *z, t_symbol *fname, int ac, t_atom *av)
{
    mtr_doread((t_mtr *)z, 0, fname);
}

static void mtr_writehook(t_pd *z, t_symbol *fname, int ac, t_atom *av)
{
    mtr_dowrite((t_mtr *)z, 0, fname);
}

static void mtr_read(t_mtr *x, t_symbol *s)
{
    if (s && s != &s_)
	mtr_doread(x, 0, s);
    else  /* CHECKED no default */
	panel_open(x->x_filehandle, 0);
}

static void mtr_write(t_mtr *x, t_symbol *s)
{
    if (s && s != &s_)
	mtr_dowrite(x, 0, s);
    else  /* CHECKED no default */
	panel_save(x->x_filehandle, canvas_getdir(x->x_cnv), 0);
}

static void mtr_free(t_mtr *x)
{
    if (x->x_tracks)
    {
	int ntracks = x->x_ntracks;
	t_mtrack **tpp = x->x_tracks;
	while (ntracks--)
	{
	    t_mtrack *tp = *tpp++;
	    if (tp->tr_binbuf) binbuf_free(tp->tr_binbuf);
	    if (tp->tr_clock) clock_free(tp->tr_clock);
	    pd_free((t_pd *)tp);
	}
	freebytes(x->x_tracks, x->x_ntracks * sizeof(*x->x_tracks));
    }
}

static void *mtr_new(t_floatarg f)
{
    t_mtr *x = 0;
    int ntracks = (int)f;
    t_mtrack **tracks;
    if (ntracks < 1)
	ntracks = 1;
    if (tracks = getbytes(ntracks * sizeof(*tracks)))
    {
	int i;
	t_mtrack **tpp;
	for (i = 0, tpp = tracks; i < ntracks; i++, tpp++)
	{
	    if (!(*tpp = (t_mtrack *)pd_new(mtrack_class)) ||
		!((*tpp)->tr_binbuf = binbuf_new()) ||
		!((*tpp)->tr_clock = clock_new(*tpp, (t_method)mtrack_tick)))
	    {
		if (*tpp) pd_free((t_pd *)*tpp);
		if ((*tpp)->tr_binbuf) binbuf_free((*tpp)->tr_binbuf);
		while (i--)
		{
		    tpp--;
		    binbuf_free((*tpp)->tr_binbuf);
		    clock_free((*tpp)->tr_clock);
		    pd_free((t_pd *)*tpp);
		}
		return (0);
	    }
	}
	if (x = (t_mtr *)pd_new(mtr_class))
	{
	    int id;
	    t_outlet *mainout = outlet_new((t_object *)x, &s_list);
	    x->x_cnv = canvas_getcurrent();
	    x->x_filehandle = file_new((t_pd *)x, 0,
					     mtr_readhook, mtr_writehook, 0);
	    if (ntracks > MTR_C74MAXTRACKS)
            ntracks = MTR_C74MAXTRACKS;
	    x->x_ntracks = ntracks;
	    x->x_tracks = tracks;
	    for (id = 1; id <= ntracks; id++, tracks++)  /* CHECKED 1-based */
	    {
		t_mtrack *tp = *tracks;
		inlet_new((t_object *)x, (t_pd *)tp, 0, 0);
		tp->tr_trackout = outlet_new((t_object *)x, &s_);
		tp->tr_mainout = mainout;
		tp->tr_owner = x;
		tp->tr_id = id;
		tp->tr_listed = 0;
		tp->tr_filehandle =  /* LATER rethink */
		    file_new((t_pd *)tp, 0,
				   mtrack_readhook, mtrack_writehook, 0);
		tp->tr_mode = MTR_STEPMODE;
		tp->tr_muted = 0;
		tp->tr_restarted = 0;
		tp->tr_atdelta = 0;
		tp->tr_ixnext = 0;
		tp->tr_tempo = 1.;
		tp->tr_clockdelay = 0.;
		tp->tr_prevtime = 0.;
	    }
	}
    }
    return (x);
}

CYCLONE_OBJ_API void mtr_setup(void){
    mtrack_class = class_new(gensym("_mtrack"), 0, 0,
        sizeof(t_mtrack), CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(mtrack_class, mtrack_bang);
    class_addfloat(mtrack_class, mtrack_float);
    class_addsymbol(mtrack_class, mtrack_symbol);
    class_addanything(mtrack_class, mtrack_anything);
    class_addlist(mtrack_class, mtrack_list);
    class_addmethod(mtrack_class, (t_method)mtrack_record,
		    gensym("record"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_play,
		    gensym("play"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_stop,
		    gensym("stop"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_next,
		    gensym("next"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_rewind,
		    gensym("rewind"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_mute,
		    gensym("mute"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_unmute,
		    gensym("unmute"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_clear,
		    gensym("clear"), 0);
    class_addmethod(mtrack_class, (t_method)mtrack_delay,
		    gensym("delay"), A_FLOAT, 0);
    class_addmethod(mtrack_class, (t_method)mtrack_first,
		    gensym("first"), A_FLOAT, 0);
    class_addmethod(mtrack_class, (t_method)mtrack_read,
		    gensym("read"), A_DEFSYM, 0);
    class_addmethod(mtrack_class, (t_method)mtrack_write,
		    gensym("write"), A_DEFSYM, 0);
    class_addmethod(mtrack_class, (t_method)mtrack_tempo,
		    gensym("tempo"), A_FLOAT, 0);

    mtr_class = class_new(gensym("mtr"),
			  (t_newmethod)mtr_new,
			  (t_method)mtr_free,
			  sizeof(t_mtr), 0,
			  A_DEFFLOAT, 0);
    class_addmethod(mtr_class, (t_method)mtr_record,
		    gensym("record"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_play,
		    gensym("play"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_stop,
		    gensym("stop"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_next,
		    gensym("next"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_rewind,
		    gensym("rewind"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_mute,
		    gensym("mute"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_unmute,
		    gensym("unmute"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_clear,
		    gensym("clear"), A_GIMME, 0);
    class_addmethod(mtr_class, (t_method)mtr_delay,
		    gensym("delay"), A_FLOAT, 0);
    class_addmethod(mtr_class, (t_method)mtr_first,
		    gensym("first"), A_FLOAT, 0);
    class_addmethod(mtr_class, (t_method)mtr_read,
		    gensym("read"), A_DEFSYM, 0);
    class_addmethod(mtr_class, (t_method)mtr_write,
		    gensym("write"), A_DEFSYM, 0);
    file_setup(mtr_class, 0);
}
