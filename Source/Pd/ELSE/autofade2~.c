// Porres 2017-2018, based on fade~ and with the mighty help of the kind Pierre Gulot

#include <m_pd.h>
#include <float.h>
#include <math.h>
#include <string.h>

#define MAXCH 512

#define UNITBIT32 1572864.  // 3*2^19; bit 32 has place value 1

#define HALF_PI 3.14159265358979323846 * 0.5

/* machine-dependent definitions.  These ifdefs really
 should have been by CPU type and not by operating system! */
#ifdef IRIX
/* big-endian.  Most significant byte is at low address in memory */
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#define int32 long  /* a data type that has 32 bits */
#endif /* IRIX */

#ifdef MSW
/* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long
#endif /* MSW */

#ifdef _MSC_VER
/* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long
#endif // _MSC_VER

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifdef __linux__
#include <endian.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)
#error No byte order defined
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define HIOFFSET 1
#define LOWOFFSET 0
#else
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#endif /* __BYTE_ORDER */
#include <sys/types.h>
#define int32 int32_t
#endif /* __unix__ or __APPLE__*/

t_float *autofade2_table_lin=(t_float *)0L;
t_float *autofade2_table_linsin=(t_float *)0L;
t_float *autofade2_table_sqrt=(t_float *)0L;
t_float *autofade2_table_quartic=(t_float *)0L;
t_float *autofade2_table_sin=(t_float *)0L;
t_float *autofade2_table_hannsin=(t_float *)0L;
t_float *autofade2_table_hann=(t_float *)0L;

typedef struct _autofade2{
    t_object   x_obj;
    t_float    x_f;        // dummy
    t_int      x_channels; // number of channels
    t_sample  *x_temp;     // temporary audio vectors
    t_int      x_blksize;  // block size
    t_float    x_ms_in;
    t_float    x_ms_out;
    t_int      x_n_in;
    t_int      x_n_out;
    double     x_coef_in;
    double     x_coef_out;
    double     x_phase;
    double     x_incr;
    t_float    x_gate_status;
    t_float    x_sr_khz;
    t_int      x_left_n;
    t_int      x_status_flag;
    t_float   *x_table;
    t_outlet  *x_status_out;
}t_autofade2;

union tabfudge_d{
    double tf_d;
    int32 tf_i[2];
};

static t_class *autofade2_class;

static void autofade2_lin(t_autofade2 *x){
    x->x_table = autofade2_table_lin;
}

static void autofade2_linsin(t_autofade2 *x){
    x->x_table = autofade2_table_linsin;
}

static void autofade2_sqrt(t_autofade2 *x){
    x->x_table = autofade2_table_sqrt;
}

static void autofade2_quartic(t_autofade2 *x){
    x->x_table = autofade2_table_quartic;
}


static void autofade2_sin(t_autofade2 *x){
    x->x_table = autofade2_table_sin;
}

static void autofade2_hannsin(t_autofade2 *x){
    x->x_table = autofade2_table_hannsin;
}

static void autofade2_hann(t_autofade2 *x){
    x->x_table = autofade2_table_hann;
}

static void autofade2_fadein(t_autofade2 *x, t_floatarg ms_in){
    x->x_n_in = roundf(ms_in * x->x_sr_khz);
    if(x->x_n_in < 1)
        x->x_n_in = 1;
    x->x_coef_in = 1. / (double)x->x_n_in;
}

static void autofade2_fadeout(t_autofade2 *x, t_floatarg x_ms_out){
    x->x_n_out = roundf(x_ms_out * x->x_sr_khz);
    if(x->x_n_out < 1)
        x->x_n_out = 1;
    x->x_coef_out = 1. / (double)x->x_n_out;
}
 
static t_int *autofade2_perform(t_int *w){
    t_autofade2 *x = (t_autofade2 *)(w[1]); // first is object
    t_int channels      = (t_int)(w[2]);
    t_int blocksize     = (t_int)(w[3]);
    t_sample *gate      = (t_sample *)(w[4]);
    t_sample *temporary = (t_sample *)(w[5]);
    t_sample *tempio, *fadevalues, *temp;
    double incr = x->x_incr;
    double phase = x->x_phase;
    double dphase;
    t_float gate_status = x->x_gate_status;
    t_int n_left = x->x_left_n;
    t_float *tab = x->x_table, *addr, f1, f2, frac;
    t_int normhipart;
    union tabfudge_d tf;
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];
    t_int i, j;
// get fade values
    fadevalues = gate; // needs to be initialized somehow
    for(j = 0; j < blocksize; ++j){
        gate[j] = (fabsf(gate[j]) >= FLT_EPSILON); // gate on/off
        if(gate[j] != gate_status){ // gate change / update
            gate_status = gate[j];
            if(gate_status){
                incr = (double)(1 - phase) * x->x_coef_in;
                n_left = x->x_n_in;
            }
            else{
                incr = -(double)(phase * x->x_coef_out);
                n_left = x->x_n_out;
            }
            x->x_status_flag = 0;
        }
        if(n_left > 0){
            phase += incr;
            if(PD_BIGORSMALL(phase) || phase < 0)
                phase = 0;
            if(phase > 1)
                phase = 1;
            n_left--;
            dphase = (double)(phase * (t_float)(COSTABSIZE) * 0.99999) + UNITBIT32;
            tf.tf_d = dphase;
            addr = tab + (tf.tf_i[HIOFFSET] & (COSTABSIZE-1));
            tf.tf_i[HIOFFSET] = normhipart;
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            fadevalues[j] = f1 + frac * (f2 - f1);
        }
        else{
            phase = fadevalues[j] = gate_status;
            if(!x->x_status_flag){
                outlet_float(x->x_status_out, gate_status);
                x->x_status_flag = 1;
            }
        }
    };
// Record inputs * fade into temporary vector
    temp = temporary;
    for(i = 0; i < channels; ++i){
        tempio = (t_sample *)(w[i+6]);
        for(j = 0; j < blocksize; ++j){
            *temp++ = *tempio++ * fadevalues[j];
        }
    }
// Record temporary vector into outputs
    temp = temporary;
    for(i = 0; i < channels; ++i){
        tempio = (t_sample *)(w[i+6+channels]);
        for(j = 0; j < blocksize; ++j)
            *tempio++ = *temp++;
    }
    x->x_gate_status = (PD_BIGORSMALL(gate_status) ? 0. : gate_status);
    x->x_incr = incr;
    x->x_left_n = n_left;
    x->x_phase = phase;
    return(w + channels * 2 + 6);
}

static void autofade2_free(t_autofade2 *x){
    if(x->x_temp){
        freebytes(x->x_temp, x->x_channels * x->x_blksize * sizeof(*x->x_temp));
        x->x_temp = NULL;
        x->x_blksize = 0;
    }
}

static void autofade2_dsp(t_autofade2 *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    size_t i, vecsize;
    t_int* vec;
// Free memory if temp vector's been allocated
    autofade2_free(x);
// Allocate a C-like matrix (number of rows * number of columns)
    x->x_blksize = (t_int)sp[0]->s_n;
    x->x_temp    = (t_sample *)getbytes(x->x_channels * x->x_blksize * sizeof(*x->x_temp));
    if(x->x_temp){ // Pass arg to method and args to DSP chain
        vecsize = x->x_channels * 2 + 5;
        vec = (t_int *)getbytes(vecsize * sizeof(*vec));
        if(vec){
            vec[0] = (t_int *)x;             // first is the object
            vec[1] = (t_int)x->x_channels;
            vec[2] = (t_int)sp[0]->s_n;
            vec[3] = (t_int)sp[0]->s_vec;
            vec[4] = (t_int)x->x_temp;
            for(i = 0; i < x->x_channels * 2; ++i)
                vec[i+5] = (t_int)sp[i+1]->s_vec;
            dsp_addv(autofade2_perform, vecsize, vec);
            freebytes(vec, vecsize * sizeof(*vec));
        }
        else
            pd_error(x, "can't allocate temporary vectors.");
    }
    else{
        x->x_blksize = 0;
        pd_error(x, "can't allocate temporary vectors.");
    }
}

static void *autofade2_new(t_symbol *s, int argc, t_atom *argv){
    t_int i;
    t_autofade2 *x = (t_autofade2 *)pd_new(autofade2_class);
    if(x){
        x->x_sr_khz = sys_getsr() * 0.001;
        x->x_gate_status = 0.;
        x->x_status_flag = 1;
        x->x_incr = 0.;
        x->x_left_n = 0;
        x->x_coef_in = 0.;
        x->x_table = autofade2_table_quartic;
        t_float ms_in = 0;
        t_float ms_out = 0;
        x->x_channels = 1;
//
        t_int symarg = 0;
        t_int argnum = 0;
        while(argc){
            if(argv->a_type == A_SYMBOL){
                if(!symarg){
                    t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
                    if(!strcmp(curarg->s_name, "lin"))
                        x->x_table = autofade2_table_lin;
                    else if(!strcmp(curarg->s_name, "linsin"))
                        x->x_table = autofade2_table_linsin;
                    else if(!strcmp(curarg->s_name, "sqrt"))
                        x->x_table = autofade2_table_sqrt;
                    else if(!strcmp(curarg->s_name, "sin"))
                        x->x_table = autofade2_table_sin;
                    else if(!strcmp(curarg->s_name, "hannsin"))
                        x->x_table = autofade2_table_hannsin;
                    else if(!strcmp(curarg->s_name, "linsin"))
                        x->x_table = autofade2_table_linsin;
                    else if(!strcmp(curarg->s_name, "hann"))
                        x->x_table = autofade2_table_hann;
                    else if(!strcmp(curarg->s_name, "quartic"))
                        x->x_table = autofade2_table_quartic;
                    else
                        goto errstate;
                    symarg = 1;
                }
                else
                    goto errstate;
            }
            else if(argv->a_type == A_FLOAT){
                t_float argval = atom_getfloatarg(0, argc, argv);
                if(symarg){
                    switch(argnum){
                        case 1:
                            ms_in = argval;
                            break;
                        case 2:
                            ms_out = argval;
                            break;
                        case 3:
                            x->x_channels = (int)argval;
                            break;
                        default:
                            break;
                    };
                }
                else{
                    switch(argnum){
                        case 0:
                            ms_in = argval;
                            break;
                        case 1:
                            ms_out = argval;
                            break;
                        case 2:
                            x->x_channels = (int)argval;
                            break;
                        default:
                            break;
                    };
                }
            }
            argnum++;
            argc--;
            argv++;
        }
        if(x->x_channels < 1)
            x->x_channels = 1;
        if(x->x_channels > MAXCH)
            x->x_channels = MAXCH;
        autofade2_fadein(x, ms_in);
        autofade2_fadeout(x, ms_out);
        x->x_temp    = NULL;
        x->x_blksize = 0;
        for(i = 0; i < x->x_channels; i++){
            inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
            outlet_new((t_object *)x, &s_signal);
        }
        x->x_status_out = outlet_new((t_object *)x, &s_float);
    }
    return(x);
    errstate:
        pd_error(x, "autofade2~: improper args");
        return NULL;
}

static void autofade2_tilde_maketable(void){
    int i;
    t_float *fp, phase1, phase2, fff;
    t_float phlinc = 1.0 / ((t_float)COSTABSIZE*0.99999);
    t_float phsinc = HALF_PI * phlinc;
    union tabfudge_d tf;
    
    if(!autofade2_table_sin){
        autofade2_table_sin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_sin, phase1=0; i--; fp++, phase1+=phsinc)
            *fp = sin(phase1);
    }
    if(!autofade2_table_hannsin){
        autofade2_table_hannsin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_hannsin, phase1=0; i--; fp++, phase1+=phsinc){
            fff = sin(phase1);
            *fp = fff*sqrt(fff);
        }
    }
    if(!autofade2_table_hann){
        autofade2_table_hann = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_hann, phase1=0; i--; fp++, phase1+=phsinc){
            fff = sin(phase1);
            *fp = fff*fff;
        }
    }
    if(!autofade2_table_lin){
        autofade2_table_lin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_lin, phase1=0; i--; fp++, phase1+=phlinc)
            *fp = phase1;
    }
    if(!autofade2_table_linsin){
        autofade2_table_linsin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_linsin, phase1=phase2=0; i--; fp++, phase1+=phlinc, phase2+=phsinc)
            *fp = sqrt(phase1 * sin(phase2));
    }
    if(!autofade2_table_quartic){
        autofade2_table_quartic = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_quartic, phase1=0; i--; fp++, phase1+=phlinc)
            *fp = pow(phase1, 4);
    }
    if(!autofade2_table_sqrt){
        autofade2_table_sqrt = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
        for(i=COSTABSIZE+1, fp=autofade2_table_sqrt, phase1=0; i--; fp++, phase1+=phlinc)
            *fp = sqrt(phase1);
    }
    tf.tf_d = UNITBIT32 + 0.5;
    if((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
        bug("autofade2~: unexpected machine alignment");
}

void autofade2_tilde_setup(void){
    t_class* c = class_new(gensym("autofade2~"), (t_newmethod)autofade2_new, (t_method)autofade2_free,
            sizeof(t_autofade2), CLASS_DEFAULT, A_GIMME, 0);
    if(c){
        CLASS_MAINSIGNALIN(c, t_autofade2, x_f);
        class_addmethod(c, (t_method)autofade2_dsp, gensym("dsp"), A_CANT, 0);
        class_addmethod(c, (t_method)autofade2_fadein, gensym("fadein"), A_DEFFLOAT, 0);
        class_addmethod(c, (t_method)autofade2_fadeout, gensym("fadeout"), A_DEFFLOAT, 0);
        class_addmethod(c, (t_method)autofade2_lin, gensym("lin"), 0);
        class_addmethod(c, (t_method)autofade2_linsin, gensym("linsin"), 0);
        class_addmethod(c, (t_method)autofade2_sqrt, gensym("sqrt"), 0);
        class_addmethod(c, (t_method)autofade2_sin, gensym("sin"), 0);
        class_addmethod(c, (t_method)autofade2_hannsin, gensym("hannsin"), 0);
        class_addmethod(c, (t_method)autofade2_hann, gensym("hann"), 0);
        class_addmethod(c, (t_method)autofade2_quartic, gensym("quartic"), 0);
        autofade2_tilde_maketable();

        autofade2_class = c;
    }
}
