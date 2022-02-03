//Derek Kwan - 2016: renaming cybuf to cybuf some restructuring, consilidating, cleaning up
//notes: now cybuf would be a holder for buffer names and channel info, not the type of the object itself

/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __cybuf_H__
#define __cybuf_H__

#ifdef INT_MAX
#define SHARED_INT_MAX  INT_MAX
#else
#define SHARED_INT_MAX  0x7FFFFFFF
#endif

#define CYBUF_MAXCHANS 64 //max number of channels

typedef struct _cybuf
{
    //t_sic       s_sic;
    void     *c_owner; //owner of cybuf, note i don't know if this actually works
    int         c_npts;   /* used also as a validation flag, number of samples in an array */
    int         c_numchans;
    t_word    **c_vectors;
    t_symbol  **c_channames;
    //t_symbol   *s_mononame;  /* used also as an 'ismono' flag */
    t_symbol   *c_bufname;
    //float       c_ksr;
    int         c_playable;
    int         c_minsize;
    int         c_disabled;
    int         c_single; //flag for single channel mode
                        //0-regular mode, 1-load this particular channel (1-idx)
                        //should be used with c_numchans == 1
} t_cybuf;

void cybuf_bug(char *fmt, ...);
void cybuf_clear(t_cybuf *c);
void cybuf_redraw(t_cybuf *c);
void cybuf_playcheck(t_cybuf *c);
//int cybuf_getnchannels(t_cybuf *x);

//use this function during cybuf_init
//passing 0 to complain suppresses warnings
void cybuf_initarray(t_cybuf *c, t_symbol *name, int complain);
void cybuf_validate(t_cybuf *c, int complain);
//called by cybuf_validate
t_word *cybuf_get(t_cybuf *c, t_symbol * name, int *bufsize, int indsp, int complain);

//wrap around initarray, but allow warnings (pass 1 to complain)
void cybuf_setarray(t_cybuf *c, t_symbol *name);

void cybuf_setminsize(t_cybuf *c, int i);
void cybuf_enable(t_cybuf *c, t_floatarg f);
//void cybuf_dsp(t_cybuf *x, t_signal **sp, t_perfroutine perf, int complain);

//single channel mode used for poke~/peek~
void *cybuf_init(t_class *owner, t_symbol *bufname, int numchans, int singlemode);
void cybuf_free(t_cybuf *c);
//void cybuf_setup(t_class *c, void *dspfn, void *floatfn);
void cybuf_checkdsp(t_cybuf *c);
void cybuf_getchannel(t_cybuf *c, int chan_num, int complain);

#endif
