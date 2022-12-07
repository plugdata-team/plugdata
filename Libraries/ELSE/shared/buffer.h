
#ifndef __buffer_H__
#define __buffer_H__

#ifdef INT_MAX
#define SHARED_INT_MAX  INT_MAX
#else
#define SHARED_INT_MAX  0x7FFFFFFF
#endif

#define buffer_MAXCHANS 64 //max number of channels

#include <math.h>

#define TWO_PI (3.14159265358979323846 * 2)
#define HALF_PI (3.14159265358979323846 * 0.5)

#define ONE_SIXTH 0.16666666666666666666667f

#define INDEX_2PT() \
    double xpos = phase*(double)size; \
    int ndx = (int)xpos; \
    double frac = xpos - ndx; \
    if(ndx == size) ndx = 0; \
    int ndx1 = ndx + 1; \
    if(ndx1 == size) ndx1 = 0; \
    double b = (double)vector[ndx].w_float; \
    double c = (double)vector[ndx1].w_float;

#define INDEX_4PT() \
    double xpos = phase*(double)size; \
    int ndx = (int)xpos; \
    double frac = xpos - ndx; \
    if(ndx == size) ndx = 0; \
    int ndxm1 = ndx - 1; \
    if(ndxm1 < 0) ndxm1 = size - 1; \
    int ndx1 = ndx + 1; \
    if(ndx1 == size) ndx1 = 0; \
    int ndx2 = ndx1 + 1; \
    if(ndx2 == size) ndx2 = 0; \
    double a = (double)vector[ndxm1].w_float; \
    double b = (double)vector[ndx].w_float; \
    double c = (double)vector[ndx1].w_float; \
    double d = (double)vector[ndx2].w_float;

typedef struct _buffer{
    void       *c_owner;     // owner of buffer, note i don't know if this actually works
    int         c_npts;      // used also as a validation flag, number of samples in an array */
    int         c_numchans;
    t_word    **c_vectors;
    t_symbol  **c_channames;
    t_symbol   *c_bufname;
    int         c_playable;
    int         c_minsize;
    int         c_disabled;
    int         c_single;    // flag: 0-regular mode, 1-load this particular channel (1-idx)
                             // should be used with c_numchans == 1
}t_buffer;

float interp_lin(double frac, double b, double c);
float interp_cos(double frac, double b, double c);
float interp_pow(double frac, double b, double c, double p);
float interp_lagrange(double frac, double a, double b, double c, double d);
float interp_cubic(double frac, double a, double b, double c, double d);
float interp_spline(double frac, double a, double b, double c, double d);
float interp_hermite(double frac, double a, double b, double c, double d,
    double bias, double tension);

void buffer_bug(char *fmt, ...);
void buffer_clear(t_buffer *c);
void buffer_redraw(t_buffer *c);
void buffer_playcheck(t_buffer *c);

// use this function during buffer_init
// passing 0 to complain suppresses warnings
void buffer_initarray(t_buffer *c, t_symbol *name, int complain);
void buffer_validate(t_buffer *c, int complain);
// called by buffer_validate
t_word *buffer_get(t_buffer *c, t_symbol * name, int *bufsize, int indsp, int complain);

// wrap around initarray, but allow warnings (pass 1 to complain)
void buffer_setarray(t_buffer *c, t_symbol *name);

void buffer_setminsize(t_buffer *c, int i);
void buffer_enable(t_buffer *c, t_floatarg f);

// single channel mode used for poke~/peek~
void *buffer_init(t_class *owner, t_symbol *bufname, int numchans, int singlemode);
void buffer_free(t_buffer *c);
void buffer_checkdsp(t_buffer *c);
void buffer_getchannel(t_buffer *c, int chan_num, int complain);

#endif
