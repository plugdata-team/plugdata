// a rip off from iemlib/fade~ with some changes

#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

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

t_float *fader_table_lin=(t_float *)0L;
t_float *fader_table_linsin=(t_float *)0L;
t_float *fader_table_sqrt=(t_float *)0L;
t_float *fader_table_quartic=(t_float *)0L;
t_float *fader_table_sin=(t_float *)0L;
t_float *fader_table_hannsin=(t_float *)0L;
t_float *fader_table_hann=(t_float *)0L;

static t_class *fader_tilde_class;

typedef struct _fader_tilde{
  t_object x_obj;
  t_float *x_table;
  t_float x_f;
} t_fader_tilde;

union tabfudge_d{
    double tf_d;
    int32 tf_i[2];
};

static void fader_lin(t_fader_tilde *x){
    x->x_table = fader_table_lin;
}

static void fader_linsin(t_fader_tilde *x){
    x->x_table = fader_table_linsin;
}

static void fader_sqrt(t_fader_tilde *x){
    x->x_table = fader_table_sqrt;
}

static void fader_quartic(t_fader_tilde *x){
    x->x_table = fader_table_quartic;
}


static void fader_sin(t_fader_tilde *x){
    x->x_table = fader_table_sin;
}

static void fader_hannsin(t_fader_tilde *x){
    x->x_table = fader_table_hannsin;
}

static void fader_hann(t_fader_tilde *x){
    x->x_table = fader_table_hann;
}


static void *fader_tilde_new(t_symbol *s){
  t_fader_tilde *x = (t_fader_tilde *)pd_new(fader_tilde_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->x_f = 0;
    
    if(s == gensym("lin"))
        x->x_table = fader_table_lin;
    else if(s == gensym("linsin"))
        x->x_table = fader_table_linsin;
    else if(s == gensym("sqrt"))
        x->x_table = fader_table_sqrt;
    else if(s == gensym("sin"))
        x->x_table = fader_table_sin;
    else if(s == gensym("hannsin"))
        x->x_table = fader_table_hannsin;
    else if(s == gensym("hann"))
        x->x_table = fader_table_hann;
    else if(s == gensym("quartic"))
        x->x_table = fader_table_quartic;
    else
      x->x_table = fader_table_quartic; // default
  return (x);
}

static t_int *fader_tilde_perform(t_int *w){
  t_float *in = (t_float *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  t_fader_tilde *x = (t_fader_tilde *)(w[3]);
  int n = (int)(w[4]);
  t_float *tab = x->x_table, *addr, f1, f2, frac;
  double dphase;
  int normhipart;
  union tabfudge_d tf;
  
  tf.tf_d = UNITBIT32;
  normhipart = tf.tf_i[HIOFFSET];
  
  while (n--)
  {
    t_float input = *in++;
    if (input < 0)
        input = 0;
    if (input > 1)
        input = 1;
    dphase = (double)(input * (t_float)(COSTABSIZE) * 0.99999) + UNITBIT32;
    tf.tf_d = dphase;
    addr = tab + (tf.tf_i[HIOFFSET] & (COSTABSIZE-1));
    tf.tf_i[HIOFFSET] = normhipart;
    frac = tf.tf_d - UNITBIT32;
    f1 = addr[0];
    f2 = addr[1];
    *out++ = f1 + frac * (f2 - f1);
  }
  return (w+5);
}

static void fader_tilde_dsp(t_fader_tilde *x, t_signal **sp){
  dsp_add(fader_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void fader_tilde_maketable(void){
  int i;
  t_float *fp, phase1, phase2, fff;
  t_float phlinc = 1.0 / ((t_float)COSTABSIZE*0.99999);
  t_float phsinc = HALF_PI * phlinc;
  union tabfudge_d tf;
  
  if(!fader_table_sin)
  {
    fader_table_sin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_sin, phase1=0; i--; fp++, phase1+=phsinc)
      *fp = sin(phase1);
  }
  if(!fader_table_hannsin)
  {
    fader_table_hannsin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_hannsin, phase1=0; i--; fp++, phase1+=phsinc)
    {
      fff = sin(phase1);
      *fp = fff*sqrt(fff);
    }
  }
  if(!fader_table_hann)
  {
    fader_table_hann = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_hann, phase1=0; i--; fp++, phase1+=phsinc)
    {
      fff = sin(phase1);
      *fp = fff*fff;
    }
  }
  if(!fader_table_lin)
  {
    fader_table_lin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_lin, phase1=0; i--; fp++, phase1+=phlinc)
      *fp = phase1;
  }
  if(!fader_table_linsin)
  {
    fader_table_linsin = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_linsin, phase1=phase2=0; i--; fp++, phase1+=phlinc, phase2+=phsinc)
      *fp = sqrt(phase1 * sin(phase2));
  }
  if(!fader_table_quartic)
  {
    fader_table_quartic = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_quartic, phase1=0; i--; fp++, phase1+=phlinc)
        *fp = pow(phase1, 4);
  }
  if(!fader_table_sqrt)
  {
    fader_table_sqrt = (t_float *)getbytes(sizeof(t_float) * (COSTABSIZE+1));
    for(i=COSTABSIZE+1, fp=fader_table_sqrt, phase1=0; i--; fp++, phase1+=phlinc)
      *fp = sqrt(phase1);
  }
  tf.tf_d = UNITBIT32 + 0.5;
  if((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
    bug("fader~: unexpected machine alignment");
}

void fader_tilde_setup(void){
  fader_tilde_class = class_new(gensym("fader~"), (t_newmethod)fader_tilde_new, 0,
    sizeof(t_fader_tilde), 0, A_DEFSYM, 0);
  CLASS_MAINSIGNALIN(fader_tilde_class, t_fader_tilde, x_f);
  class_addmethod(fader_tilde_class, (t_method)fader_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(fader_tilde_class, (t_method)fader_lin, gensym("lin"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_linsin, gensym("linsin"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_sqrt, gensym("sqrt"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_sin, gensym("sin"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_hannsin, gensym("hannsin"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_hann, gensym("hann"), 0);
    class_addmethod(fader_tilde_class, (t_method)fader_quartic, gensym("quartic"), 0);
  fader_tilde_maketable();
}
