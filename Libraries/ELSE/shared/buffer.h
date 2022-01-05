
#ifndef __buffer_H__
#define __buffer_H__

#ifdef INT_MAX
#define SHARED_INT_MAX  INT_MAX
#else
#define SHARED_INT_MAX  0x7FFFFFFF
#endif

#define buffer_MAXCHANS 64 //max number of channels

typedef struct _buffer{
    //t_sic       s_sic;
    void     *c_owner; //owner of buffer, note i don't know if this actually works
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
}t_buffer;

void buffer_bug(char *fmt, ...);
void buffer_clear(t_buffer *c);
void buffer_redraw(t_buffer *c);
void buffer_playcheck(t_buffer *c);
//int buffer_getnchannels(t_buffer *x);

//use this function during buffer_init
//passing 0 to complain suppresses warnings
void buffer_initarray(t_buffer *c, t_symbol *name, int complain);
void buffer_validate(t_buffer *c, int complain);
//called by buffer_validate
t_word *buffer_get(t_buffer *c, t_symbol * name, int *bufsize, int indsp, int complain);

//wrap around initarray, but allow warnings (pass 1 to complain)
void buffer_setarray(t_buffer *c, t_symbol *name);

void buffer_setminsize(t_buffer *c, int i);
void buffer_enable(t_buffer *c, t_floatarg f);
//void buffer_dsp(t_buffer *x, t_signal **sp, t_perfroutine perf, int complain);

//single channel mode used for poke~/peek~
void *buffer_init(t_class *owner, t_symbol *bufname, int numchans, int singlemode);
void buffer_free(t_buffer *c);
//void buffer_setup(t_class *c, void *dspfn, void *floatfn);
void buffer_checkdsp(t_buffer *c);
void buffer_getchannel(t_buffer *c, int chan_num, int complain);

#endif
