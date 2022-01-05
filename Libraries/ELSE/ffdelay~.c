#include "m_pd.h"
#include <string.h>

#define FFDEL_DEFSIZE   192000                          // default buffer size
#define FFDEL_GUARD     4                               // guard points for interpolation
#define FFDEL_EXTRA     1                               // extra sample for interpolation
#define FFDEL_STACK  FFDEL_DEFSIZE + 2*FFDEL_GUARD - 1  // Stack Size

typedef struct _ffdelay{
    t_object        x_obj;
    t_glist        *x_glist;
    t_inlet        *x_inlet;
    t_float         *x_buf;
    t_float         *x_bufend;
    t_float         *x_whead;
    t_float         x_del_time;             // current delay in samples
    t_float         x_last_time;            // previous delay in samples
    t_float         x_sr_khz;               // sample rate in khz
    unsigned int    x_ms;                   // ms flag
    unsigned int	x_maxsize;              // buffer size in samples
    unsigned int	x_maxsofar;             // largest maxsize so far
    unsigned int    x_freeze;
    t_float         x_bufini[FFDEL_STACK];  // default stack buffer
}t_ffdelay;

static t_class *ffdelay_class;

static void ffdelay_clear(t_ffdelay *x){
    memset(x->x_buf, 0, (x->x_maxsize + 2*FFDEL_GUARD - 1) * sizeof(*x->x_buf));
}

static void ffdelay_bounds(t_ffdelay *x){
    x->x_whead = x->x_buf + FFDEL_GUARD - 1;
    x->x_bufend = x->x_buf + x->x_maxsize + 2*FFDEL_GUARD - 1;
}

static void ffdelay_resize(t_ffdelay *x, t_float f){
    unsigned int maxsize = (f < 1 ? 1 : (unsigned int)f);
    if(maxsize > x->x_maxsofar){
        x->x_maxsofar = maxsize;
        if(x->x_buf == x->x_bufini){
            if(!(x->x_buf = (t_float *)getbytes((maxsize + 2*FFDEL_GUARD - 1) * sizeof(*x->x_buf)))){
                x->x_buf = x->x_bufini;
                x->x_maxsize = FFDEL_DEFSIZE;
                pd_error(x, "unable to resize buffer; using size of %d samples", FFDEL_DEFSIZE);
            }
        }
        else if(x->x_buf){
            if (!(x->x_buf = (t_float *)resizebytes(x->x_buf,
            (x->x_maxsize + 2*FFDEL_GUARD - 1) * sizeof(*x->x_buf),
            (maxsize + 2*FFDEL_GUARD - 1) * sizeof(*x->x_buf)))){
                x->x_buf = x->x_bufini;
                x->x_maxsize = FFDEL_DEFSIZE;
                pd_error(x, "unable to resize buffer; using size of %d samples", FFDEL_DEFSIZE);
            }		
        }
    }
    x->x_maxsize = maxsize;
    if(x->x_del_time > (float)maxsize)
        x->x_del_time = (float)maxsize;
    ffdelay_clear(x);
    ffdelay_bounds(x);
}

static void ffdelay_size(t_ffdelay *x, t_float size){
    if(x->x_ms)
        size *= x->x_sr_khz;
    ffdelay_resize(x, size);
}

static void ffdelay_freeze(t_ffdelay *x, t_float f){
    x->x_freeze = (unsigned int)(f != 0);
}

static t_int *ffdelay_perform(t_int *w){
	t_ffdelay *x = (t_ffdelay *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float *buf = x->x_buf;
    t_float *bp = x->x_buf + FFDEL_GUARD - 1;
    t_float *ep = x->x_bufend;
    t_float *wp = x->x_whead;
    t_float *rp;
    t_float sr_khz = x->x_sr_khz;
    unsigned int maxsize = x->x_maxsize;
    while(nblock--){
    	t_sample f, del, frac, a, b, c, d, cminusb;
   		int idel;
        int lin = 0;
    	f = *in1++;
    	if(PD_BIGORSMALL(f))
    		f = 0.;
        del = *in2++;
        if(x->x_ms)
            del *= sr_khz;
        del = (del > 0. ? del : 0.);
        if(del >= 1)
            del-= 1;
        else
             lin = 1;
        del = (del < maxsize - 1 ? del : maxsize - 1);
        x->x_del_time = del;
    	idel = (int)del;
        frac = del - (t_sample)idel;
        if(!x->x_freeze)
            *wp = f;
    	rp = wp - idel;
    	if(rp < bp)
    		rp += (maxsize + FFDEL_GUARD);
    	d = rp[-3];
        c = rp[-2];
        b = rp[-1];
        a = rp[0];
        cminusb = c-b;
        if(lin)
            *out++ = a + (b-a) * frac;
        else
            *out++ = b + frac * (
                cminusb - 0.1666667f * (1.-frac) * (
                    (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
                )
            );
    	if(++wp == ep){
    		buf[0] = wp[-3];
    		buf[1] = wp[-2];
    		buf[2] = wp[-1];
    		wp = bp;
    	}
    }
    x->x_whead = wp;
    return(w + 6);
}

static void ffdelay_dsp(t_ffdelay *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(ffdelay_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *ffdelay_new(t_symbol *s, int argc, t_atom * argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_ffdelay *x;
    x = (t_ffdelay *)pd_new(ffdelay_class);
    x->x_sr_khz = sys_getsr() * 0.001;
    float delsize = 1000 * x->x_sr_khz; // default max size = 1 second
    float del_time = 0;
    x->x_freeze = 0;
    x->x_ms = 1;
/////////////////////////////////////////////////////////////////////////////////
    int symarg = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT && !symarg){
            delsize = del_time = atom_getfloatarg(0, argc, argv);
            argc--;
            argv++;
            argnum++;
        }
        else if(argv->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(cursym->s_name, "-size")){
                if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                    delsize = atom_getfloatarg(1, argc, argv);
                    argc -= 2;
                    argv += 2;
                    argnum++;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-samps")){
                x->x_ms = 0;
                argc--;
                argv++;
                argnum++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
/////////////////////////////////////////////////////////////////////////////////
    if(del_time < 0) // post error?
        del_time = 0;
    if(delsize <= 0) // post error?
        delsize = 1000 * x->x_sr_khz;
    float actual_time = del_time;
    if(x->x_ms){
        actual_time *= x->x_sr_khz;
        delsize *= x->x_sr_khz;
    }
    if(actual_time > delsize)
        delsize = actual_time;
    x->x_buf = x->x_whead = x->x_bufini;
    x->x_maxsize = x->x_maxsofar = FFDEL_DEFSIZE;
    ffdelay_resize(x, delsize);
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), del_time);
    outlet_new((t_object *)x, &s_signal);
    return(x);
errstate:
    pd_error(x, "[ffdelay~]: improper args");
    return NULL;
}

static void ffdelay_free(t_ffdelay *x){
    if (x->x_buf != x->x_bufini) freebytes(x->x_buf, x->x_maxsofar * sizeof(*x->x_buf));
}

void ffdelay_tilde_setup(void){
    ffdelay_class = class_new(gensym("ffdelay~"), (t_newmethod)ffdelay_new, (t_method)ffdelay_free,
			    sizeof(t_ffdelay), 0, A_GIMME, 0);
    class_addmethod(ffdelay_class, nullfn, gensym("signal"), 0);
    class_addmethod(ffdelay_class, (t_method)ffdelay_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(ffdelay_class, (t_method)ffdelay_clear, gensym("clear"), 0);
    class_addmethod(ffdelay_class, (t_method)ffdelay_size, gensym("size"), A_DEFFLOAT, 0);
    class_addmethod(ffdelay_class, (t_method)ffdelay_freeze, gensym("freeze"), A_DEFFLOAT, 0);
}
