/* code generated thanks to Schiavoni's Pure Data external Generator */
#include "m_pd.h"
#include <common/api.h>
#include "math.h"

// ---------------------------------------------------
// Class definition
// ---------------------------------------------------
static t_class *trunc_class;

// ---------------------------------------------------
// Data structure definition
// ---------------------------------------------------
typedef struct _trunc {
   t_object x_obj;
   t_outlet * x_outlet_dsp_0;
} t_trunc;

// ---------------------------------------------------
// Functions signature
// ---------------------------------------------------
void * trunc_new(void);// Constructor
void trunc_destroy(t_trunc *x); //Destructor
static t_int * trunc_perform(t_int *w); //Perform function
static void trunc_dsp(t_trunc *x, t_signal **sp); //DSP function

// ---------------------------------------------------
// Perform
// ---------------------------------------------------
static t_int * trunc_perform(t_int *w)
{
    t_trunc *x = (t_trunc *)(w[1]); // Seu objeto
    int n = (int)(w[2]); // Número de samples no bloco
    t_float *in1 = (t_float *)(w[3]); // bloco de entrada
    t_float *out1 = (t_float *)(w[4]); // bloco de saida

    //interagir com todas as amostras do bloco (de 0 a n)
    int i = 0;
    for (i = 0 ; i < n ; i++){
        out1[i] = trunc(in1[i]); // trunc~
    }
    return (w + 5); // proximo bloco
}

// ---------------------------------------------------
// DSP Function
// ---------------------------------------------------
static void trunc_dsp(t_trunc *x, t_signal **sp)
{
   dsp_add(trunc_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

// ---------------------------------------------------
// Constructor of the class
// ---------------------------------------------------
void * trunc_new(void)
{
   t_trunc *x = (t_trunc *) pd_new(trunc_class);
   x->x_outlet_dsp_0 = outlet_new(&x->x_obj, &s_signal);
   return (void *) x;
}

// ---------------------------------------------------
// Destroy the class
// ---------------------------------------------------
void trunc_destroy(t_trunc *x)
{
   outlet_free(x->x_outlet_dsp_0);
}

// ---------------------------------------------------
// Setup
// ---------------------------------------------------
CYCLONE_OBJ_API void trunc_tilde_setup(void) {
   trunc_class = class_new(gensym("trunc~"),
      (t_newmethod) trunc_new, // Constructor
      (t_method) trunc_destroy, // Destructor
      sizeof (t_trunc),
      CLASS_DEFAULT,
      0);//Must always ends with a zero
    
   class_addmethod(trunc_class, nullfn, gensym("signal"), 0);
   class_addmethod(trunc_class, (t_method) trunc_dsp, gensym("dsp"), A_CANT, 0);
}
// EOF---------------------------------------------------


