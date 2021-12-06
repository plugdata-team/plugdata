#include "m_pd.h"

#define COEFFS 5   // number of coeffs per filter stage
#define MAX_COEFFS 250 // defining max number of coeffs to take
#define STAGES 50 // number of stages = MAX_COEFFS/COEFFS

static t_class *biquads_class;

typedef struct _biquads{
    t_object  x_obj;
    t_inlet  *x_coefflet;
    t_outlet *x_outlet;
    double    x_xnm1[STAGES];
    double    x_xnm2[STAGES];
    double    x_ynm1[STAGES];
    double    x_ynm2[STAGES];
    t_int     x_bypass;
    int 	  x_numfilt; // number of biquad filters
	double 	  x_coeff[MAX_COEFFS]; // array of coeffs
/* the coeff array is ane asy/cheap way of doing this
 without malloc/calloc-ing - maybe worth changing in the future */
} t_biquads;

void *biquads_new(void);

void biquads_clear(t_biquads *x){
  int i;
  for(i = 0; i < x->x_numfilt; i++)
	x->x_xnm1[i] = x->x_xnm2[i] = x->x_ynm1[i] = x->x_ynm2[i] = 0.f;
}

void biquads_bypass(t_biquads *x, t_floatarg f){
    x->x_bypass = f != 0;
}

static void biquads_list(t_biquads *x, t_symbol *s, int argc, t_atom *argv){
	int numfilt = (int)(argc/COEFFS); // nearest multiple - anything over is ignored
	if(numfilt > STAGES)
		numfilt = STAGES;
	x->x_numfilt = numfilt;
	int curfilt = 0; // filter counter
	while(curfilt < numfilt){
        int curidx = COEFFS*curfilt; // current starting index
        t_float b1 = atom_getfloatarg(curidx, argc, argv);
        t_float b2 = atom_getfloatarg(curidx+1, argc, argv);
		t_float a0 = atom_getfloatarg(curidx+2, argc, argv);
		t_float a1 = atom_getfloatarg(curidx+3, argc, argv);
		t_float a2 = atom_getfloatarg(curidx+4, argc, argv);
		x->x_coeff[curidx] = (double)b1;
		x->x_coeff[curidx+1] = (double)b2;
		x->x_coeff[curidx+2] = (double)a0;
		x->x_coeff[curidx+3] = (double)a1;
		x->x_coeff[curidx+4] = (double)a2;
		curfilt++;
	};
}

static t_int * biquads_perform(t_int *w){
    t_biquads *x = (t_biquads *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_int bypass = x->x_bypass;
    int numfilt = x->x_numfilt;
    while(nblock--){
      if(bypass)
          *out++ = *in++;
      else{
          double xn = *in++;
          int curfilt = 0; // filter section counter
          while(curfilt < numfilt){
              int curidx = COEFFS*curfilt; // current starting index
              double b1 = x->x_coeff[curidx];
              double b2 = x->x_coeff[curidx+1];
              double a0 = x->x_coeff[curidx+2];
              double a1 = x->x_coeff[curidx+3];
              double a2 = x->x_coeff[curidx+4];
              double xnm1 = x->x_xnm1[curfilt];
              double xnm2 = x->x_xnm2[curfilt];
              double ynm1 = x->x_ynm1[curfilt];
              double ynm2 = x->x_ynm2[curfilt];
              double yn = a0*xn + a1*xnm1 + a2*xnm2 + b1*ynm1 + b2*ynm2; // biquad section
              x->x_xnm2[curfilt] = xnm1;
              x->x_xnm1[curfilt] = xn;
              x->x_ynm2[curfilt] = ynm1;
              x->x_ynm1[curfilt] = yn;
              xn = yn; // next stage's xn is previous yn!
              curfilt++; // iterate the filter section counter
        };
        *out++ = xn; // xn is the cascaded output
      };
  };
  return(w + 5);
}

static void biquads_dsp(t_biquads *x, t_signal **sp){
  dsp_add(biquads_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *biquads_free(t_biquads *x){
	outlet_free(x->x_outlet);
	return (void *)x;
}

void *biquads_new(void){
  t_biquads *x = (t_biquads *)pd_new(biquads_class);
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  x->x_bypass = 0;
  x->x_numfilt = 0; // setting number of filters to 0 initially bc no coeffs
  int i;
  for(i = 0; i < MAX_COEFFS; i++) // zeroing out coeff array
	x->x_coeff[i] = 0.f;
  for(i = 0; i < STAGES; i++) // zeroing out filter's memory
	x->x_xnm1[i] = x->x_xnm2[i] = x->x_ynm1[i] = x->x_ynm2[i] = 0.f;
  return(x);
}

void biquads_tilde_setup(void){
    biquads_class = class_new(gensym("biquads~"), (t_newmethod) biquads_new,
            CLASS_DEFAULT, sizeof (t_biquads), CLASS_DEFAULT, 0);
    class_addmethod(biquads_class, nullfn, gensym("signal"), 0);
    class_addmethod(biquads_class, (t_method) biquads_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(biquads_class, (t_method) biquads_clear, gensym("clear"), 0);
    class_addmethod(biquads_class, (t_method) biquads_bypass, gensym("bypass"), A_DEFFLOAT, 0);
    class_addlist(biquads_class,(t_method)biquads_list);
}
