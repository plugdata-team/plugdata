
#include "m_pd.h"
#include "buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* on failure *bufsize is not modified */
t_word *buffer_get(t_buffer *c, t_symbol * name, int *bufsize, int indsp, int complain){
//in dsp = used in dsp,
    if(name && name != &s_){
        t_garray *ap = (t_garray *)pd_findbyclass(name, garray_class);
        if(ap){
            int bufsz;
            t_word *vec;
            if(garray_getfloatwords(ap, &bufsz, &vec)){
                if(indsp)
                    garray_usedindsp(ap);
                if(bufsize)
                    *bufsize = bufsz;
                return(vec);
            }
            else // always complain
                pd_error(c->c_owner, "bad template of array '%s'", name->s_name);
        }
        else if(complain)
            pd_error(c->c_owner, "no such array '%s'", name->s_name);
    }
    return (0);
}

//making peek~ work with channel number choosing, assuming 1-indexed
void buffer_getchannel(t_buffer *c, int chan_num, int complain){
    int chan_idx;
    char buf[MAXPDSTRING];
    t_symbol * curname; //name of the current channel we want
    int vsz = c->c_npts;  
    t_word *retvec = NULL;//pointer to the corresponding channel to return
    //1-indexed bounds checking
    chan_num = chan_num < 1 ? 1 : (chan_num > buffer_MAXCHANS ? buffer_MAXCHANS : chan_num);
    c->c_single = chan_num;
    //convert to 0-indexing, separate steps and diff variable for sanity's sake
    chan_idx = chan_num - 1;
    //making the buffer channel name string we'll be looking for
    if(c->c_bufname != &s_){
        if(chan_idx == 0){
            //if channel idx is 0, check for just plain bufname as well
            //since checking for 0-bufname as well, don't complain here
            retvec = buffer_get(c, c->c_bufname, &vsz, 1, 0);
            if(retvec){
                c->c_vectors[0] = retvec;
                if (vsz < c->c_npts) c->c_npts = vsz;
                return;
            };
        };
        sprintf(buf, "%d-%s", chan_idx, c->c_bufname->s_name);
        curname =  gensym(buf);
        retvec = buffer_get(c, curname, &vsz, 1, complain);
        //if channel found and less than c_npts, reset c_npts
        if (vsz < c->c_npts) c->c_npts = vsz;
        c->c_vectors[0] = retvec;
        return;
    };

}

void buffer_bug(char *fmt, ...){ // from loud.c
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

void buffer_clear(t_buffer *c){
    c->c_npts = 0;
    memset(c->c_vectors, 0, c->c_numchans * sizeof(*c->c_vectors));
}

void buffer_redraw(t_buffer *c){
    if(!c->c_single){
        if(c->c_numchans <= 1 && c->c_bufname != &s_){
            t_garray *ap = (t_garray *)pd_findbyclass(c->c_bufname, garray_class);
            if (ap) garray_redraw(ap);
            else if (c->c_vectors[0]) buffer_bug("buffer_redraw 1");
        }
        else if (c->c_numchans > 1){
            int ch = c->c_numchans;
            while (ch--){
                t_garray *ap = (t_garray *)pd_findbyclass(c->c_channames[ch], garray_class);
                if (ap) garray_redraw(ap);
                else if (c->c_vectors[ch]) buffer_bug("buffer_redraw 2");
            }
        };
    }
    else{
        int chan_idx;
        char buf[MAXPDSTRING];
        t_symbol * curname; //name of the current channel we want
        int chan_num = c->c_single; //1-indexed channel number
        chan_num = chan_num < 1 ? 1 : (chan_num > buffer_MAXCHANS ? buffer_MAXCHANS : chan_num);
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
            if (ap)
                garray_redraw(ap);
            // not really sure what the specific message is for, just copied single channel one - DK
            else if (c->c_vectors[0])
                buffer_bug("buffer_redraw 1");

        };
    };
}

void buffer_validate(t_buffer *c, int complain){
    buffer_clear(c);
    c->c_npts = SHARED_INT_MAX;
    if(!c->c_single){
        if (c->c_numchans <= 1 && c->c_bufname != &s_){
            c->c_vectors[0] = buffer_get(c, c->c_bufname, &c->c_npts, 1, 0);
            if(!c->c_vectors[0]){ // check for 0-bufname if bufname array isn't found
                c->c_vectors[0] = buffer_get(c, c->c_channames[0], &c->c_npts, 1, 0);
                //if neither found, post about it if complain
                if(!c->c_vectors[0] && complain)
                    pd_error(c->c_owner, "no such array '%s' (or '0-%s')",
                             c->c_bufname->s_name, c->c_bufname->s_name);
            };
        }
        else if(c->c_numchans > 1){
            int ch;
            for (ch = 0; ch < c->c_numchans ; ch++){
                int vsz = c->c_npts;  /* ignore missing arrays */
                // only complain if can't find first channel (ch = 0)
                c->c_vectors[ch] = buffer_get(c, c->c_channames[ch], &vsz, 1, !ch && complain);
                if(vsz < c->c_npts)
                    c->c_npts = vsz;
            };
        };
    }
    else
        buffer_getchannel(c, c->c_single, complain);
    if(c->c_npts == SHARED_INT_MAX)
        c->c_npts = 0;
}

void buffer_playcheck(t_buffer *c){
    c->c_playable = (!c->c_disabled && c->c_npts >= c->c_minsize);
}

void buffer_initarray(t_buffer *c, t_symbol *name, int complain){
    if(name){ // setting array names
        c->c_bufname = name;
        if(c->c_numchans >= 1){
            char buf[MAXPDSTRING];
            int ch;
            for(ch = 0; ch < c->c_numchans; ch++){
                sprintf(buf, "%d-%s", ch, c->c_bufname->s_name);
                c->c_channames[ch] = gensym(buf);
            };
        };
        buffer_validate(c, complain);
    };
    buffer_playcheck(c);
}

//wrapper around buffer_initarray so you don't have to pass the complain flag each time
void buffer_setarray(t_buffer *c, t_symbol *name){
   buffer_initarray(c, name, 1); 
}

void buffer_setminsize(t_buffer *c, int i){
    c->c_minsize = i;
}

void buffer_checkdsp(t_buffer *c){
    buffer_validate(c, 1);
    buffer_playcheck(c);

}

void buffer_free(t_buffer *c){
    if (c->c_vectors)
        freebytes(c->c_vectors, c->c_numchans * sizeof(*c->c_vectors));
    if (c->c_channames)
        freebytes(c->c_channames, c->c_numchans * sizeof(*c->c_channames));
    freebytes(c, sizeof(t_buffer));
}

/* If nauxsigs is positive, then the number of signals is nchannels + nauxsigs;
   otherwise the channels are not used as signals, and the number of signals is
   nsigs -- provided that nsigs is positive -- or, if it is not, then an buffer
   is not used in dsp (peek~). */

void *buffer_init(t_class *owner, t_symbol *bufname, int numchans, int singlemode){
// name of buffer (multichan usu, or not) and the number of channels associated with buffer
    t_buffer *c = (t_buffer *)getbytes(sizeof(t_buffer));
    t_float **vectors;
    t_symbol **channames = 0;
    if(!bufname)
        bufname = &s_;
    c->c_bufname = bufname;
    singlemode = singlemode > 0 ? 1 : 0;
// single mode forces numchans = 1
    numchans = (numchans < 1 || singlemode) ? 1 : (numchans > buffer_MAXCHANS ? buffer_MAXCHANS : numchans);
    if (!(vectors = (t_float **)getbytes(numchans* sizeof(*vectors))))
		return (0);
	if(!(channames = (t_symbol **)getbytes(numchans * sizeof(*channames)))){
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
    if(bufname != &s_)
        buffer_initarray(c, bufname, 0);
    return (c);
}

void buffer_enable(t_buffer *c, t_floatarg f){
    c->c_disabled = (f == 0);
    buffer_playcheck(c);
}
