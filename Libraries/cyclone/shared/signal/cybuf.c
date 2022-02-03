//Derek Kwan - 2016: renaming cybuf to cybuf some restructuring, consilidating, cleaning up
//notes: now cybuf would be a holder for buffer names and channel info, not the type of the object itself
//essentially, want to decouple object from buf methods so objects stay t_obj

/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* generic array-based signal class */

#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "cybuf.h"
#include<stdarg.h>

//taken from old vefl_get
/* on failure *bufsize is not modified */
t_word *cybuf_get(t_cybuf *c, t_symbol * name, int *bufsize, int indsp, int complain){
//in dsp = used in dsp, 
  
    if (name && name != &s_){
	t_garray *ap = (t_garray *)pd_findbyclass(name, garray_class);
	if (ap){
	    int bufsz;
	    t_word *vec;
	    if (garray_getfloatwords(ap, &bufsz, &vec)){
   	        //c->c_len = garray_npoints(ap);
		if (indsp) garray_usedindsp(ap);
		if (bufsize) *bufsize = bufsz;
		return (vec);
	    }
	     else pd_error(c->c_owner,  /* always complain */
			"bad template of array '%s'", name->s_name);
        }
	else{
            if(complain){
	        pd_error(c->c_owner, "no such array '%s'", name->s_name);
            };
	};
    }
    return (0);
}

//making peek~ work with channel number choosing, assuming 1-indexed
void cybuf_getchannel(t_cybuf *c, int chan_num, int complain){
    int chan_idx;
    char buf[MAXPDSTRING];
    t_symbol * curname; //name of the current channel we want
    int vsz = c->c_npts;  
    t_word *retvec = NULL;//pointer to the corresponding channel to return
    //1-indexed bounds checking
    chan_num = chan_num < 1 ? 1 : (chan_num > CYBUF_MAXCHANS ? CYBUF_MAXCHANS : chan_num);
    c->c_single = chan_num;
    //convert to 0-indexing, separate steps and diff variable for sanity's sake
    chan_idx = chan_num - 1;
    //making the buffer channel name string we'll be looking for
    if(c->c_bufname != &s_){
        if(chan_idx == 0){
            //if channel idx is 0, check for just plain bufname as well
            //since checking for 0-bufname as well, don't complain here
            retvec = cybuf_get(c, c->c_bufname, &vsz, 1, 0);
            if(retvec){
                c->c_vectors[0] = retvec;
                if (vsz < c->c_npts) c->c_npts = vsz;
                return;
            };
        };
        sprintf(buf, "%d-%s", chan_idx, c->c_bufname->s_name);
        curname =  gensym(buf);
        retvec = cybuf_get(c, curname, &vsz, 1, complain);
        //if channel found and less than c_npts, reset c_npts
        if (vsz < c->c_npts) c->c_npts = vsz;
        c->c_vectors[0] = retvec;
        return;
    };

}

void cybuf_bug(char *fmt, ...)
{
    //copied from old loud.c
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    fprintf(stderr, "miXed consistency check failed: %s\n", buf);
#ifdef _WIN32
    fflush(stderr);
#endif
    bug("%s", buf);
}

void cybuf_clear(t_cybuf *c)
{
    c->c_npts = 0;
    memset(c->c_vectors, 0, c->c_numchans * sizeof(*c->c_vectors));
}

void cybuf_redraw(t_cybuf *c)
{
    if(!c->c_single){
        if (c->c_numchans <= 1 && c->c_bufname != &s_)
        {
            t_garray *ap = (t_garray *)pd_findbyclass(c->c_bufname, garray_class);
            if (ap) garray_redraw(ap);
            else if (c->c_vectors[0]) cybuf_bug("cybuf_redraw 1");
        }
        else if (c->c_numchans > 1){
            int ch = c->c_numchans;
            while (ch--){
                t_garray *ap = (t_garray *)pd_findbyclass(c->c_channames[ch], garray_class);
                if (ap) garray_redraw(ap);
                else if (c->c_vectors[ch]) cybuf_bug("cybuf_redraw 2");
            }
        };
    }
    else{
        int chan_idx;
        char buf[MAXPDSTRING];
        t_symbol * curname; //name of the current channel we want
        int chan_num = c->c_single; //1-indexed channel number
        chan_num = chan_num < 1 ? 1 : (chan_num > CYBUF_MAXCHANS ? CYBUF_MAXCHANS : chan_num);
         //convert to 0-indexing, separate steps and diff variable for sanity's sake
        chan_idx = chan_num - 1;
        //making the buffer channel name string we'll be looking for
        if(c->c_bufname != &s_){
            if(chan_idx == 0){
            //if channel idx is 0, check for just plain bufname as well
            t_garray *ap = (t_garray *)pd_findbyclass(c->c_bufname, garray_class);
            if (ap){
                garray_redraw(ap);
                return;
                };
            };
            sprintf(buf, "%d-%s", chan_idx, c->c_bufname->s_name);
            curname =  gensym(buf);
            t_garray *ap = (t_garray *)pd_findbyclass(curname, garray_class);
            if (ap) garray_redraw(ap);
            //not really sure what the specific message is for, just copied single channel one - DK
            else if (c->c_vectors[0]) cybuf_bug("cybuf_redraw 1");

        };
    };
}

void cybuf_validate(t_cybuf *c, int complain)
{
    cybuf_clear(c);
    c->c_npts = SHARED_INT_MAX;
    if(!c->c_single){
        if (c->c_numchans <= 1 && c->c_bufname != &s_)
        {
            c->c_vectors[0] = cybuf_get(c, c->c_bufname, &c->c_npts, 1, 0);
            //check for 0-bufname if bufname array isn't found
            if(!c->c_vectors[0]){
                c->c_vectors[0] = cybuf_get(c, c->c_channames[0], &c->c_npts, 1, 0);
                //if neither found, post about it if complain
                if(!c->c_vectors[0] && complain){
                    pd_error(c->c_owner, "no such array '%s' (or '0-%s')", c->c_bufname->s_name, c->c_bufname->s_name);
                };
            };
        }
        else if (c->c_numchans > 1){
            int ch;
            for (ch = 0; ch < c->c_numchans ; ch++){
                int vsz = c->c_npts;  /* ignore missing arrays */
                c->c_vectors[ch] =
                    //only complain if can't find first channel (ch = 0)
                    cybuf_get(c, c->c_channames[ch], &vsz, 1, !ch && complain);
                if (vsz < c->c_npts) c->c_npts = vsz;
            };
        };
    }
    else{
        cybuf_getchannel(c, c->c_single, complain);
    };
    if (c->c_npts == SHARED_INT_MAX) c->c_npts = 0;
}

void cybuf_playcheck(t_cybuf *c){
    c->c_playable = (!c->c_disabled && c->c_npts >= c->c_minsize);
}

/*
int cybuf_getnchannels(t_cybuf *c)
{
    return (c->c_numchans);
}
*/

void cybuf_initarray(t_cybuf *c, t_symbol *name, int complain){   
    //setting array names	
    if (name){
	c->c_bufname = name;
	if(c->c_numchans >= 1){
	    char buf[MAXPDSTRING];
	    int ch;
	    for (ch = 0; ch < c->c_numchans; ch++){
		    sprintf(buf, "%d-%s", ch, c->c_bufname->s_name);
		    c->c_channames[ch] = gensym(buf);
		};
	};
	cybuf_validate(c, complain);
    };
    cybuf_playcheck(c);
}

//wrapper around cybuf_initarray so you don't have to pass the complain flag each time
void cybuf_setarray(t_cybuf *c, t_symbol *name){
   cybuf_initarray(c, name, 1); 
}

void cybuf_setminsize(t_cybuf *c, int i)
{
    c->c_minsize = i;
}

/*
void cybuf_dsp(t_cybuf *x, t_signal **sp, t_perfroutine perf, int complain)
{
    t_int *ap = x->s_perfargs;
    if (ap)
    {
	int i, nsigs = x->s_nperfargs - 2;
	x->s_ksr = sp[0]->s_sr * 0.001;
	cybuf_validate(x, complain);
	cybuf_playcheck(x);

	LATER consider glist traversing, and, if we have no feeders,
	   choosing an optimized version of perform routine 

	*ap++ = (t_int)x;
	*ap++ = (t_int)sp[0]->s_n;
	for (i = 0; i < nsigs; i++) *ap++ = (t_int)sp[i]->s_vec;
	dsp_addv(perf, x->s_nperfargs, x->s_perfargs);
    }
    else loudbug_bug("cybuf_dsp");
}

*/

void cybuf_checkdsp(t_cybuf *c){
    cybuf_validate(c, 1);
    cybuf_playcheck(c);

}


void cybuf_free(t_cybuf *c)
{
    if (c->c_vectors){
        freebytes(c->c_vectors, c->c_numchans * sizeof(*c->c_vectors));
    };
    if (c->c_channames){
        freebytes(c->c_channames, c->c_numchans * sizeof(*c->c_channames));
    };
    freebytes(c, sizeof(t_cybuf));
}

/* If nauxsigs is positive, then the number of signals is nchannels + nauxsigs;
   otherwise the channels are not used as signals, and the number of signals is
   nsigs -- provided that nsigs is positive -- or, if it is not, then an cybuf
   is not used in dsp (peek~). */
void *cybuf_init(t_class *owner, t_symbol *bufname, int numchans, int singlemode){
    //name of buffer (multichan usu, or not) and the number of channels associated with buffer
    t_cybuf *c = (t_cybuf *)getbytes(sizeof(t_cybuf));
    t_float **vectors;
    t_symbol **channames = 0;
    if (!bufname){
        bufname = &s_;
    };
    c->c_bufname = bufname;
    singlemode = singlemode > 0 ? 1 : 0;
    //single mode forces numchans = 1
    numchans = (numchans < 1 || singlemode) ? 1 : (numchans > CYBUF_MAXCHANS ? CYBUF_MAXCHANS : numchans);
    if (!(vectors = (t_float **)getbytes(numchans* sizeof(*vectors)))){
		return (0);
	};
    

	if (!(channames = (t_symbol **)getbytes(numchans * sizeof(*channames)))) {
		freebytes(vectors, numchans * sizeof(*vectors));
	return (0);
    };
    c->c_single = singlemode;
    c->c_owner = owner;
    c->c_npts = 0;
    c->c_vectors = vectors;
    c->c_channames = channames;
    c->c_disabled = 0;
    c->c_playable = 0;
    c->c_minsize = 1;
    c->c_numchans = numchans;
    if(bufname != &s_){
            cybuf_initarray(c, bufname, 0);
    };
    return (c);
}

void cybuf_enable(t_cybuf *c, t_floatarg f)
{
    c->c_disabled = (f == 0);
    cybuf_playcheck(c);
}

/*
void cybuf_setup(t_class *c, void *dspfn, void *floatfn)
{
    if (floatfn != SIC_NOMAINSIGNALIN)
    {
	if (floatfn)
	{
	    class_domainsignalin(c, -1);
	    class_addfloat(c, floatfn);
	}
	else CLASS_MAINSIGNALIN(c, t_sic, s_f);
    }
    class_addmethod(c, (t_method)dspfn, gensym("dsp"), 0);
    class_addmethod(c, (t_method)cybuf_enable, gensym("enable"), 0);
}
*/
