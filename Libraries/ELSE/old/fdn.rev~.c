// Feedback Delay Networks with a householder matrix (In - 2/n 11T)
// modified fom [creb/fdn~] by Porres

// TODO (from original code) - Add: delay time generation code / prime calculation for delay lengths
//       / more diffuse feedback matrix (hadamard) & check filtering code

/*   Copyright (c) 2000-2003 by Tom Schouten                                *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the Free Software            *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              */

#include "m_pd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef  union _isdenorm {
    t_float f;
    uint32_t ui;
}t_isdenorm;

static inline int denorm_check(t_float f)
{
    t_isdenorm mask;
    mask.f = f;
    return ((mask.ui & 0x07f800000) == 0);
}

// por mim essa merda toda vem pra baixo
typedef struct fdnctl{
    t_int    c_order;
    t_int    c_maxorder;
    t_float  c_leak;
    t_float  c_input;
    t_float  c_output;
    t_float *c_buf;
    t_float *c_gain_in;
    t_float *c_gain_state;
    t_int   *c_tap;         // cirular feed: N+1 pointers: 1 read, (N-1)r/w, 1 write
    t_float *c_time_ms;
    t_int    c_bufsize;
    t_float *c_vector[2];
    t_float *c_vectorbuffer;
    t_int    c_curvector;
}t_fdnctl;

typedef struct fdn{
    t_object x_obj;
    t_fdnctl x_ctl;
    t_int    x_exp;
    t_float  x_damping;
    t_float  x_t60_lo;
    t_float  x_t60_hi;
}t_fdn;

t_class *fdn_class;

// Decay filter equation: yn = (2*gl*gh ) / (gl+gh) x + (gl-gh) / (gl+gh) y[n-1]
// were gl is the DC gain and gh is the Nyquist gain (both calculated from t_60):
// Source: https://ccrma.stanford.edu/~jos/pasp/First_Order_Delay_Filter_Design.html
static void fdn_setgain(t_fdn *x){
    t_float gl, gh;
    for(t_int i = 0; i < x->x_ctl.c_order; i++){
        gl = pow(10, -3.f * x->x_ctl.c_time_ms[i] / x->x_t60_lo);
        gh = pow(10, -3.f * x->x_ctl.c_time_ms[i] / x->x_t60_hi);
        x->x_ctl.c_gain_in[i] = 2.0f * gl * gh / (gl + gh);
        x->x_ctl.c_gain_state[i] = (gl - gh) / (gl + gh);
    }
}

static void fdn_set_t60_hi(t_fdn *x){
    t_float t60 = x->x_t60_lo;
    x->x_t60_hi = t60 + (10 - t60) * x->x_damping; // rescale from 10ms to t60]
    fdn_setgain(x);
}

static void fdn_damping(t_fdn *x, t_float f){
    x->x_damping = (f < 0 ? 0 : f > 100 ? 100 : f) / 100;
    fdn_set_t60_hi(x);
}

static void fdn_time(t_fdn *x, t_float f){
    x->x_t60_lo = (f < .01f ? .01f : f) * 1000;
    fdn_set_t60_hi(x);
}

static void fdn_delsizes(t_fdn *x){
    t_int mask = x->x_ctl.c_bufsize - 1;
    t_int start = x->x_ctl.c_tap[0];
    t_int *tap = x->x_ctl.c_tap;
    tap[0] = (start & mask);
    float *length = x->x_ctl.c_time_ms;
    float scale = sys_getsr() * .001f;
    t_int sum = 0;
    for(t_int t = 1; t <= x->x_ctl.c_order; t++){
        sum += (t_int)(length[t-1] * scale); // delay time in samples
        tap[t] = (start+sum)&mask;
    }
    if(sum > mask)
        post("[fdn.rev~]: not enough delay memory (this could lead to instability)");
    fdn_setgain(x);
}

static void fdn_order(t_fdn *x, t_int order){
    x->x_ctl.c_order = order;
    x->x_ctl.c_leak = -2./ order;
    x->x_ctl.c_input = 1./ sqrt(order); // ???
}

static void fdn_set(t_fdn *x, t_float size, t_float min, t_float max){
    t_int order = ((int)size) & 0xfffffffc; // clip to powers of 2
    if(order < 4){
        post("[fdn.rev~]: number of delay lines clipped to minimum of 4");
        order = 4;
    }
    if(order > x->x_ctl.c_maxorder){
        post("[fdn.rev~]: number of delay lines clipped to maximum of %d:",
             x->x_ctl.c_maxorder);
        order = x->x_ctl.c_maxorder;
    }
    if(min <= 0){
        post("[fdn.rev~]: min can't be equal or less than 0, clipped to 1");
        min = 1;
    }
    if(max <= 0){
        post("[fdn.rev~]: max can't be equal or less than 0, clipped to 1");
        max = 1;
    }
    t_float inc;
    if(x->x_exp)
        inc = pow(max / min, 1.0f / ((float)(order - 1))); // EXPONENTIAL
    else
        inc = (max - min) / (float)(order - 1); // LINEAR
    t_float length = min;
    for(t_int i = 0; i < order; i++){
        x->x_ctl.c_time_ms[i] = length;
        if(x->x_exp)
            length *= inc;
        else
            length += inc;
    }
    fdn_order(x, order);
    fdn_delsizes(x);
}

static void fdn_exp(t_fdn *x, t_float mode){
    x->x_exp = (t_int)(mode != 0);
}

static void fdn_list (t_fdn *x,  t_symbol *s, int argc, t_atom *argv){
    t_symbol *dummy = s;
    dummy = NULL;
    int order = argc & 0xfffffffc; // clip to powers of two
    if(order < 4){
        post("[fdn.rev~]: needs at least 4 delay taps (list ignored)");
        return;
    }
    if(order > x->x_ctl.c_maxorder){
        post("[fdn.rev~]: list can't be bigger than %d (list ignored)",
             order = x->x_ctl.c_maxorder);
        return;
    }
    fdn_order(x, order);
    for(t_int i = 0; i < order; i++){
        if(argv[i].a_type == A_FLOAT)
            x->x_ctl.c_time_ms[i] = argv[i].a_w.w_float;
        else
            post("[fdn.rev~]: non float element in the list ignored");
    }
    fdn_delsizes(x);
}

static void fdn_print(t_fdn *x){
/*    post("[fdn.rev~]: max delay lines %d, buffer size %d samples (%.2f ms)",
         x->x_ctl.c_maxorder, x->x_ctl.c_bufsize ,
         ((float)x->x_ctl.c_bufsize) * 1000 / sys_getsr());
    post("damping = %.2f%%, t60_lo = %.2f sec, t60_hi = %.2f sec",
         x->x_damping * 100, x->x_t60_lo / 1000, x->x_t60_hi / 1000);*/
    post("[fdn.rev~]: delay times:");
    for(t_int i = 0; i < x->x_ctl.c_order; i++)
        post("line %d: %.2f ms", i + 1, x->x_ctl.c_time_ms[i]);
}

static void fdn_clear(t_fdn *x){
    if(x->x_ctl.c_buf)
        memset(x->x_ctl.c_buf, 0, x->x_ctl.c_bufsize * sizeof(float));
    if(x->x_ctl.c_vectorbuffer)
        memset(x->x_ctl.c_vectorbuffer, 0, x->x_ctl.c_maxorder * 2 * sizeof(float));
}

static t_int *fdn_perform(t_int *w){
    t_fdnctl *ctl       = (t_fdnctl *)(w[1]);
    t_int n             = (t_int)(w[2]);
    t_float *in         = (float *)(w[3]);
    t_float *outl       = (float *)(w[4]);
    t_float *outr       = (float *)(w[5]);
    t_float *gain_in    = ctl->c_gain_in;
    t_float *gain_state = ctl->c_gain_state;
    t_int order         = ctl->c_order;
    t_int *tap          = ctl->c_tap;
    t_float *buf        = ctl->c_buf;
    t_int mask          = ctl->c_bufsize - 1;
    t_int i, j;
    t_float x, y, left, right, z;
    t_float *cvec, *lvec;
    t_float save;
    for(i = 0; i < n; i++){
        x = *in++;
        y = 0;
        left = 0;
        right = 0;
// get temporary vector buffers
        cvec = ctl->c_vector[ctl->c_curvector];
        lvec = ctl->c_vector[ctl->c_curvector ^ 1];
        ctl->c_curvector ^= 1;
// read input vector + get sum and left/right output
        for(j = 0; j < order;){
            z = buf[tap[j]];
            cvec[j] = z;
            y     += z;
            left  += z;
            right += z;
            j++;
            z = buf[tap[j]];
            cvec[j] = z;
            y     += z;
            left  -= z;
            right += z;
            j++;
            z = buf[tap[j]];
            cvec[j] = z;
            y     += z;
            left  += z;
            right -= z;
            j++;
            z = buf[tap[j]];
            cvec[j] = z;
            y     += z;
            left  -= z;
            right -= z;
            j++;
        }
        *outl++ = left;
        *outr++ = right;
        y *=  ctl->c_leak; // y == leak to all inputs
// perform feedback (todo: decouple feedback & permutation)
        save = cvec[0];
        for(j = 0; j < order-1; j++)
            cvec[j] = cvec[j+1] + y + x;
        cvec[order-1] = save + y + x;
// apply gain + store result vector in delay lines + increment taps
        tap[0] = (tap[0] + 1)&mask;
        for(j = 0; j < order; j++){
            save = gain_in[j] * cvec[j] + gain_state[j] * lvec[j];
            save = denorm_check(save) ? 0 : save;
            cvec[j] = save;
            buf[tap[j+1]] = save;
            tap[j+1] = (tap[j+1] + 1) & mask;
        }
    }
    return(w+6);
}

static void fdn_dsp(t_fdn *x, t_signal **sp){
  dsp_add(fdn_perform, 5, &x->x_ctl, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void fdn_free(t_fdn *x){
    if(x->x_ctl.c_tap)
        free( x->x_ctl.c_tap);
    if(x->x_ctl.c_time_ms)
        free( x->x_ctl.c_time_ms);
    if(x->x_ctl.c_gain_in)
        free( x->x_ctl.c_gain_in);
    if(x->x_ctl.c_gain_state)
        free( x->x_ctl.c_gain_state);
    if(x->x_ctl.c_buf)
        free (x->x_ctl.c_buf);
    if(x->x_ctl.c_vectorbuffer)
        free (x->x_ctl.c_vectorbuffer);
}

static void *fdn_new(t_symbol *s, int ac, t_atom *av){
    t_fdn *x = (t_fdn *)pd_new(fdn_class);
    t_symbol *dummy = s;
    dummy = NULL;
////////
    t_int order = 1024; // maximum order is 1024
    t_int size = 23;
    t_float t60 = 4;
    t_float damping = 0;
    x->x_exp = 0;
////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    int flag = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT && !flag){
//            t_float aval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                    /* case 0:
                     freq = aval;
                     break;
                     case 1:
                     reson = aval;
                     break;*/
                default:
                    break;
            };
            argnum++;
            ac--;
            av++;
        }
        else if(av->a_type == A_SYMBOL){
            if(!flag)
                flag = 1;
            t_symbol * cursym = atom_getsymbolarg(0, ac, av);
            if(!strcmp(cursym->s_name, "-size")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    size = (int)atom_getfloatarg(1, ac, av);
                    ac -= 2;
                    av += 2;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-time")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    t60 = atom_getfloatarg(1, ac, av);
                    ac -= 2;
                    av += 2;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-damping")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    damping = atom_getfloatarg(1, ac, av);
                    ac -= 2;
                    av += 2;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-max")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    order = (int)atom_getfloatarg(1, ac, av);
                    ac -= 2;
                    av += 2;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-exp"))
                x->x_exp = 1;
            else
                goto errstate;
        }
        else
            goto errstate;
    };
////////////////////////////////////////////////////////////////////////////////////
     if(order < 4)
         order = 4;
    order = ((int)order) & 0xfffffffc; // clip to powers of 2
    if(size < 16) // size in samples
        size = 16;
    if(size > 30) // size in samples
        size = 30;
    size = pow(2, size);
    t60 = (t60 < .01f ? .01f : t60) * 1000; // minimum 10 ms
    x->x_t60_lo = t60;
    x->x_damping = (damping < 0.f ? 0.f : damping > 100 ? 100 : damping) / 100;
    x->x_t60_hi = t60 + (10 - t60) * x->x_damping;
    x->x_ctl.c_maxorder = order;
    x->x_ctl.c_bufsize = size;
    x->x_ctl.c_buf = (float *)malloc(sizeof(float) * size);
    x->x_ctl.c_tap = (t_int *)malloc((order + 1) * sizeof(t_int));
    x->x_ctl.c_time_ms = (t_float *)malloc(order * sizeof(t_int));
    x->x_ctl.c_gain_in = (t_float *)malloc(order * sizeof(t_float));
    x->x_ctl.c_gain_state = (t_float *)malloc(order * sizeof(t_float));
    x->x_ctl.c_vectorbuffer = (t_float *)malloc(order * 2 * sizeof(float));
    memset(x->x_ctl.c_vectorbuffer, 0, order * 2 * sizeof(float));
    x->x_ctl.c_curvector = 0;
    x->x_ctl.c_vector[0] = &x->x_ctl.c_vectorbuffer[0];
    x->x_ctl.c_vector[1] = &x->x_ctl.c_vectorbuffer[order];
// default input list
    t_atom at[8];
    SETFLOAT(at, 7.f);
    SETFLOAT(at+1, 11.f);
    SETFLOAT(at+2, 13.f);
    SETFLOAT(at+3, 17.f);
    SETFLOAT(at+4, 19.f);
    SETFLOAT(at+5, 23.f);
    SETFLOAT(at+6, 29.f);
    SETFLOAT(at+7, 31.f);
    fdn_list(x, &s_list, 8, at);
// clear delay memory to zero
    fdn_clear(x);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("time"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("damping"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    return(void *)x;
errstate:
    pd_error(x, "[fdn.rev~]: improper args");
    return NULL;
}

void setup_fdn0x2erev_tilde(void){
    fdn_class = class_new(gensym("fdn.rev~"), (t_newmethod)fdn_new,
    	(t_method)fdn_free, sizeof(t_fdn), 0, A_GIMME, 0);
    class_addmethod(fdn_class, nullfn, gensym("signal"), 0);
    class_addmethod(fdn_class, (t_method)fdn_dsp, gensym("dsp"), A_CANT, 0);
    class_addlist(fdn_class, (t_method)fdn_list);
    class_addmethod(fdn_class, (t_method)fdn_time, gensym("time"), A_DEFFLOAT, 0);
    class_addmethod(fdn_class, (t_method)fdn_damping, gensym("damping"), A_DEFFLOAT, 0);
    class_addmethod(fdn_class, (t_method)fdn_set, gensym("set"),
                    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(fdn_class, (t_method)fdn_exp, gensym("exp"), A_DEFFLOAT, 0);
    class_addmethod(fdn_class, (t_method)fdn_clear, gensym("clear"), 0);
    class_addmethod(fdn_class, (t_method)fdn_print, gensym("print"), 0);
}
