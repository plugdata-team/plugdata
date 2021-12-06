#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *spread_class;

#define HALF_PI (3.14159265358979323846 * 0.5)

typedef struct _spread{
    t_object    x_obj;
    long        inChans;
    long        outChans;
    t_float    *inarr;
    t_float   **loc_invecs;
    t_float    *outs;
    double      advFrac;
    double     *pangains1;
    double     *pangains2;
    long       *indexList;
}t_spread;

t_int *spread_perform(t_int *w){
    t_spread *x = (t_spread*) w[1];
	long inChans = x->inChans;
    long outChans = x->outChans;
    t_float *inarr = x->inarr;
    t_float **loc_invecs = x->loc_invecs;
    t_float *invec;
    t_float **outs = x->outs;
    double *pangains1 = x->pangains1;
    double *pangains2 = x->pangains2;
	int chan,i, j;
    long outIndex;
    long *indexList = x->indexList;
    
    int n = (int) w[inChans + outChans + 2]; // assign output vector pointers
    
    for(i = 0; i < outChans; i++)
        outs[i] = (t_float *) w[2 + inChans + i];

    for(i = 0; i < inChans; i++){ // copy inputs to local 2D array
        invec = (t_float *) w[2 + i];
        for(j = 0; j < n; j++)
            loc_invecs[i][j] = invec[j];
    }
   
	for( j = 0; j < n; j++){
        for(chan = 0; chan < inChans; chan++) // copy local input sample frame
            inarr[chan] = loc_invecs[chan][j];
        
        for(chan = 1; chan < outChans - 1; chan++) // zero out output channels
            outs[chan][j] = 0.0;
        
        // copy outer channel samples directly
        outs[0][j] = inarr[0];
        outs[outChans - 1][j] = inarr[inChans - 1];
        
        // spread internal input channels to respective output channels
        for(chan = 1; chan < inChans - 1; chan++){
            outIndex = indexList[chan];
            outs[outIndex][j] += pangains1[chan] * inarr[chan];
            outs[outIndex+1][j] += pangains2[chan] * inarr[chan];
        }
	}
    return(w + inChans + outChans + 3);
}

void spread_dsp(t_spread *x, t_signal **sp){
	long i;
    t_int **sigvec;
    int pointer_count = x->inChans + x->outChans + 2;
    sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));
	for(i = 0; i < pointer_count; i++)
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	sigvec[0] = (t_int *)x; // first pointer is to the object
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	for(i = 1; i < pointer_count - 1; i++) // attach inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
    dsp_addv(spread_perform, pointer_count, (t_int *)sigvec);
    free(sigvec);
}

void spread_free(t_spread *x){
    int i;
    free(x->inarr);
    free(x->pangains1);
    free(x->pangains2);
    free(x->indexList);
    for(i = 0; i < x->inChans; i++)
        free(x->loc_invecs[i]);
    free(x->loc_invecs);
    free(x->outs);
}

void *spread_new(t_symbol *s, int ac, t_atom *av){
    t_spread *x = (t_spread *)pd_new(spread_class);
    t_symbol *dummy = s;
    dummy = NULL;
    int i;
    double fullFrac, thisFrac, panloc;
    long outIndex;
    x->inChans = (long)atom_getfloatarg(0, ac, av);
    x->outChans = (long)atom_getfloatarg(1, ac, av);
    if(x->inChans < 2)
        x->inChans = 2;
    if(x->outChans < 2)
        x->outChans = 2;
    if(x->inChans > 512)
        x->inChans = 512;
    if(x->outChans > 512)
        x->outChans = 512;
    for(i = 0; i < x->inChans - 1; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"),gensym("signal"));
    for(i=0; i < x->outChans; i++)
        outlet_new(&x->x_obj, gensym("signal"));
    
    x->inarr = (t_float *) malloc(x->inChans * sizeof(t_float));
    x->loc_invecs = (t_float **) malloc(x->inChans * sizeof(t_float *));
    for(i = 0; i < x->inChans; i++){
        x->loc_invecs[i] = (t_float *) malloc(8192 * sizeof(t_float));
    }
    x->pangains1 = (double *) malloc(x->inChans * sizeof(double));
    x->pangains2 = (double *) malloc(x->inChans * sizeof(double));
    x->indexList = (long *) malloc(x->inChans * sizeof(long));
    x->advFrac = (double)(x->outChans - 1)/(double)(x->inChans - 1);
    x->outs = (t_float **) malloc(x->outChans * sizeof(t_float *)); // temporary holding for output vectors
    
    for(i = 1; i < x->inChans - 1; i++){
        fullFrac = i * x->advFrac;
        outIndex = floor(fullFrac);
        thisFrac = fullFrac - outIndex;
        panloc = thisFrac * HALF_PI;
        x->indexList[i] = outIndex;
        x->pangains1[i] = cos(panloc);
        x->pangains2[i] = sin(panloc);
    }
    return x;
}

void spread_tilde_setup(void){
    spread_class = class_new(gensym("spread~"), (t_newmethod)spread_new,
            (t_method)spread_free, sizeof(t_spread), 0, A_GIMME, 0);
    class_addmethod(spread_class, nullfn, gensym("signal"), 0);
    class_addmethod(spread_class, (t_method)spread_dsp, gensym("dsp"),0);
}
