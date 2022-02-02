// Porres 2017 from mask~

#include "m_pd.h"
#include <string.h>
#include <stdlib.h> /* BSDs for example */

static t_class *impseq_class;

#define MAXLEN 1024
#define MAXimpseqS 1024
#define MAXSEQ 1024

typedef struct{
    float *pat; // impseq pattern
    int length;// length of pattern
} t_impseqpat;

typedef struct{
    int *seq; // impseq pattern
    int length;// length of pattern
    int phase; // keep track of where we are in sequence
} t_sequence;

typedef struct _impseq{
    t_object x_obj;
    float x_f;
    float x_lastin;
    int x_bang;
    short mute; // stops all computation (try z-disable)
    short gate; // continues impseqing but inhibits all output
    short phaselock; // indicates all patterns are the same size and use the same phase count
    short indexmode; //special mode where input clicks are also impseq indicies (+ 1)
    int phase; //phase of current pattern
    int current_impseq; // currently selected pattern
    t_impseqpat *impseqs; // contains the impseq patterns
    t_sequence sequence; // contains an optional impseq sequence
    int *stored_impseqs; // a list of patterns stored
    int pattern_count; //how many patterns are stored
    short noloop; // flag to play pattern only once
    float *in_vec; // copy space for input to avoid dreaded vector sharing override
} t_impseq;

static void impseq_bang(t_impseq *x){
    x->x_bang = 1;
}

void impseq_float(t_impseq *x, t_floatarg f){
        x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
        x->impseqs[0].length = 1;
        x->impseqs[0].pat[0] = f;
        x->current_impseq = 0; // now we use the impseq we read from the arguments
        x->stored_impseqs[0] = 0;
        x->pattern_count = 1;
        x->x_bang = 1;
}

static void impseq_list(t_impseq *x, t_symbol *s, int argc, t_atom * argv){
    int i;
    x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
    x->impseqs[0].length = argc;
    for(i = 0; i < argc; i++){
        x->impseqs[0].pat[i] = atom_getfloatarg(i,argc,argv);
    }
    x->current_impseq = 0; // now we use the impseq we read from the arguments
    x->stored_impseqs[0] = 0;
    x->pattern_count = 1;
    x->phase = 0;
    x->x_bang = 1;
}

static void impseq_set(t_impseq *x, t_symbol *s, int argc, t_atom * argv){
    x->phase = 0;
    if(!argc){
        x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
        x->impseqs[0].length = 1;
        x->impseqs[0].pat[0] = 1;
        x->current_impseq = 0; // now we use the impseq we read from the arguments
        x->stored_impseqs[0] = 0;
        x->pattern_count = 1;
    }
    else {
        int i;
        x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
        x->impseqs[0].length = argc;
        for(i = 0; i < argc; i++){
            x->impseqs[0].pat[i] = atom_getfloatarg(i,argc,argv);
        }
        x->current_impseq = 0; // now we use the impseq we read from the arguments
        x->stored_impseqs[0] = 0;
        x->pattern_count = 1;
    }
}

void impseq_indexmode(t_impseq *x, t_floatarg t){
	x->indexmode = (short)t;
}

void impseq_goto(t_impseq *x, t_floatarg f){
    x->phase = (int)f - 1;
}

void impseq_mute(t_impseq *x, t_floatarg f){
    x->mute = (short)f;
}

void impseq_noloop(t_impseq *x, t_floatarg f){
    x->noloop = (short)f;
}

void impseq_phaselock(t_impseq *x, t_floatarg f){
    x->phaselock = (short)f;
}

void impseq_gate(t_impseq *x, t_floatarg f){
    x->gate = (short)f;
}

void impseq_showimpseq(t_impseq *x, t_floatarg p){
    int location = p;
    short found = 0;
    int i;
    int len;
    
    for(i = 0; i<x->pattern_count; i++){
        if(location == x->stored_impseqs[i]){
            found = 1;
            break;
        }
    }
    if(found){
        len = x->impseqs[location].length;
        post("pattern length is %d",len);
        for(i = 0; i < len; i++){
            post("%d: %f",i,x->impseqs[location].pat[i]);
        }
        
    }
    else {
        pd_error(x, "no pattern stored at location %d",location);
    }
}

void impseq_recall(t_impseq *x, t_floatarg p)
{
    int i;
    int location = p;
    short found = 0;
    for(i = 0; i < x->pattern_count; i++){
        if(location == x->stored_impseqs[i]){
            found = 1;
            break;
        }
    }
    if(found){
        x->current_impseq = location;
        if(! x->phaselock){
            x->phase = 0;
        }
    } else {
        pd_error(x, "no pattern stored at location %d",location);
    }
}

void impseq_playonce(t_impseq *x, t_floatarg pnum){
    x->noloop = 1;
    x->mute = 0;
    impseq_recall(x,pnum);
}

// initiate impseq recall sequence
void impseq_sequence(t_impseq *x, t_symbol *msg, short argc, t_atom *argv){
    int i;
	if(argc > MAXSEQ){
		pd_error(x, "%d exceeds possible length for a sequence",argc);
		return;
	}
	if(argc < 1){
		pd_error(x, "you must sequence at least 1 impseq");
		return;
	}
	for(i = 0; i < argc; i++){
		x->sequence.seq[i] = atom_getfloatarg(i,argc,argv);
	}
	if(x->sequence.seq[0] < 0){
		post("sequencing turned off");
		x->sequence.length = 0;
		return;
	}
	x->sequence.phase = 0;
	x->sequence.length = argc;
	// now load in first impseq of sequence
	impseq_recall(x, (t_floatarg)x->sequence.seq[x->sequence.phase++]);
	
	// ideally would check that each sequence number is a valid stored location
}

void impseq_addimpseq(t_impseq *x, t_symbol *msg, short argc, t_atom *argv){
    int location;
    int i;
    if(argc < 2){
        pd_error(x, "must specify location and impseq");
        return;
    }
    if(argc > MAXLEN){
        pd_error(x, "impseq is limited to length %d",MAXLEN);
        return;
    }
    location = atom_getintarg(0,argc,argv);
    if(location < 0 || location > MAXimpseqS - 1){
        pd_error(x, "illegal location");
        return;
    }
    if(x->impseqs[location].pat == NULL){
        x->impseqs[location].pat = (float *) malloc(MAXLEN * sizeof(float));
        x->stored_impseqs[x->pattern_count++] = location;
    }
    else {
    //    post("replacing pattern stored at location %d", location);
    }
    //  post("reading new impseq from argument list, with %d members",argc-1);
    x->impseqs[location].length = argc-1;
    for(i=1; i<argc; i++){
        x->impseqs[location].pat[i-1] = atom_getfloatarg(i,argc,argv);
    }
    //  post("there are currently %d patterns stored",x->pattern_count);
}

t_int *impseq_perform(t_int *w){
    t_impseq *x = (t_impseq *) (w[1]);
    float *inlet = (t_float *) (w[2]);
    float *outlet = (t_float *) (w[3]);
    int nblock = (int) w[4];
    t_float lastin = x->x_lastin;
    int phase = x->phase;
    short gate = x->gate;
    short indexmode = x->indexmode;
    short noloop = x->noloop;
    int current_impseq = x->current_impseq;
    t_impseqpat *impseqs = x->impseqs;
    t_sequence sequence = x->sequence;
    while (nblock--){
        float input = *inlet++;
        float output;
/*        if(x->mute || current_impseq < 0)
            *outlet++ = 0;
          else{ */
            if((input != 0 && lastin == 0) || x->x_bang){ // trigger
/*              if(indexmode){ // input controls the phase
                    phase = input - 1;
                    if(phase < 0 || phase >= impseqs[current_impseq].length)
                        phase %= impseqs[current_impseq].length;
                } */
                output = impseqs[current_impseq].pat[phase];
                phase++; // next phase
                if(phase >= impseqs[current_impseq].length){
                    phase = 0;
                    if(sequence.length){ // if a sequence is active, reset the current impseq too
                        impseq_recall(x, (t_floatarg)sequence.seq[sequence.phase++]);
                        current_impseq = x->current_impseq; // this was reset internally!
                        if(sequence.phase >= sequence.length)
                            sequence.phase = 0;
                    }
                }
            x->x_bang = 0;
            }
        else output = 0;
//        }
        *outlet++ = output;
        lastin = input;
    }
    x->x_lastin = lastin;
    x->phase = phase;
    x->sequence.phase = sequence.phase;
    return (w+5);
}

void impseq_dsp(t_impseq *x, t_signal **sp){
    dsp_add(impseq_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void impseq_free(t_impseq *x){
    int i;
    for(i=0;i<x->pattern_count;i++)
        free(x->impseqs[i].pat);
    free(x->impseqs);
    free(x->stored_impseqs);
    free(x->sequence.seq);
    free(x->in_vec);
}

void *impseq_new(t_symbol *msg, short argc, t_atom *argv)
{
    int i;
    t_impseq *x = (t_impseq *)pd_new(impseq_class);
    outlet_new(&x->x_obj, gensym("signal"));
    
    x->impseqs = (t_impseqpat *) malloc(MAXimpseqS * sizeof(t_impseqpat));
    x->stored_impseqs = (int *) malloc(MAXimpseqS * sizeof(int));
    
    x->sequence.seq = (int *) malloc(MAXSEQ * sizeof(int));
    
    x->sequence.length = 0; // no sequence by default
    x->sequence.phase = 0; //
    
    //	post("allocated %d bytes for basic impseq holder",MAXimpseqS * sizeof(t_impseqpat));
    
    /*    x->current_impseq = -1; // by default no impseq is selected
     for(i = 0; i < MAXimpseqS; i++){
     x->stored_impseqs[i] = -1; // indicates no pattern stored
     x->impseqs[i].pat = NULL;
     } */
    if(argc == 0){
        x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
        x->impseqs[0].length = 1;
        x->impseqs[0].pat[0] = 1;
        x->current_impseq = 0; // now we use the impseq we read from the arguments
        x->stored_impseqs[0] = 0;
        x->pattern_count = 1;
    }
    else{ // post("reading initial impseq from argument list, with %d members",argc);
        x->impseqs[0].pat = (float *) malloc(MAXLEN * sizeof(float));
        // post("allocated %d bytes for this pattern", MAXLEN * sizeof(float));
        x->impseqs[0].length = argc;
        for(i = 0; i < argc; i++){
            x->impseqs[0].pat[i] = atom_getfloatarg(i,argc,argv);
        }
        x->current_impseq = 0; // now we use the impseq we read from the arguments
        x->stored_impseqs[0] = 0;
        x->pattern_count = 1;
    }
    x->indexmode = 0;
    x->mute = 0;
    x->gate = 1; //by default gate is on, and the pattern goes out (zero gate turns it off)
    x->phaselock = 0;// by default do NOT use a common phase for all patterns
    x->phase = 0;
    x->noloop = 0;
    
    return x;
}

void impseq_tilde_setup(void){
    impseq_class = class_new(gensym("impseq~"), (t_newmethod)impseq_new,
                             (t_method)impseq_free ,sizeof(t_impseq), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(impseq_class, t_impseq, x_f);
    class_addmethod(impseq_class,(t_method)impseq_dsp,gensym("dsp"), A_CANT, 0);
    class_addmethod(impseq_class,(t_method)impseq_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_phaselock,gensym("phaselock"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_gate,gensym("gate"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_addimpseq,gensym("addimpseq"),A_GIMME,0);
    class_addmethod(impseq_class,(t_method)impseq_sequence,gensym("sequence"),A_GIMME,0);
    class_addmethod(impseq_class,(t_method)impseq_recall,gensym("recall"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_showimpseq,gensym("showimpseq"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_indexmode,gensym("indexmode"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_playonce,gensym("playonce"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_noloop,gensym("noloop"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_goto,gensym("goto"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_set, gensym("set"),A_GIMME,0);
    class_addbang(impseq_class, (t_method)impseq_bang);
    class_addfloat(impseq_class, (t_method)impseq_float);
    class_addlist(impseq_class, (t_method)impseq_list);
}


