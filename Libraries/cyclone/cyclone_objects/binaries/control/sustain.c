//redone 2016 by Derek Kwan (with some inspiration from the old code by krzYszcz (2002-2003)

#include "m_pd.h"
#include <common/api.h>
#include <string.h>
#include <stdlib.h>

#define SUSTAIN_NPCH  128 // number of midi pitches
#define SUSTAIN_NORD 500 //size of incoming order array on stack
#define SUSTAIN_ADDSZ 100 //size to make x_ord bigger by

static t_class *sustain_class;

typedef struct _sustain
{
	t_object        x_obj;
	t_float         x_vel; //velocity
        int             *x_ord; //pointer to order array, points to x_ordst at first
        int             x_ordst[SUSTAIN_NORD]; //order of incoming notes on the stack
        unsigned int    x_arrsz; //size of order array
        unsigned int    x_nord; //actual number of entries in x_ord
        int             x_heaped; //if x_ord points to heap
        int             x_mode;
        t_float         x_onoff;
        t_outlet         *x_velout; //velocity outlet

        int             x_noteon[SUSTAIN_NPCH]; //for mode 1, if we've gotten a note on
} t_sustain;


static int sustain_check_noteoff(t_sustain *x, int pitch){
    //finds existence of pitch in x->x_ord;
    unsigned int i;
    for(i=0; i<x->x_nord; i++){
        if(pitch == x->x_ord[i]){
            return 1;
        };
    };
    return 0;
}

static void sustain_check_arrsz(t_sustain *x, int sz){
    int cursz = x->x_arrsz; //current size
    int heaped = x->x_heaped; //if x_ord points to heap
    if(heaped && sz > cursz){
        //if heaped and we need more space, make more space
        int newsz = cursz + SUSTAIN_ADDSZ; //proposed new size
        while(newsz < sz){
            newsz += SUSTAIN_ADDSZ; //tack on more chunks until we have the size we need
        };
        //reallocate and we're good to go
        x->x_ord = (int *)realloc(x->x_ord, sizeof(int)*newsz);
        x->x_arrsz = newsz;
    }
    else if(!heaped && sz > SUSTAIN_NORD){
        //if not heaped and can't fit in stack var, allocate on heap
        int newsz = SUSTAIN_NORD + SUSTAIN_ADDSZ; //proposed new size
        //add more space if needed
        while(newsz < sz){
            newsz += SUSTAIN_ADDSZ;
        };
        //allocate on heap
        int * newarr = (int *)malloc(sizeof(int)*newsz);
        //copy stack contents to heap
        memcpy(newarr, x->x_ord, sizeof(int)*SUSTAIN_NORD);
        //point to heap
        x->x_ord = newarr;
        x->x_arrsz = newsz;
        x->x_heaped = 1;
    }
    else if(heaped && sz < SUSTAIN_NORD){
        //if we're heaped and don't need to be anymore
        //copy heap contents to stack
        memcpy(x->x_ordst, x->x_ord, sizeof(int)*SUSTAIN_NORD);
        //free and repoint to stack
        free(x->x_ord);
        x->x_ord = x->x_ordst;
        x->x_arrsz = SUSTAIN_NORD;
        x->x_heaped = 0;
    };
    //don't have to worry about other cases:
    //(if not heaped and doesn't need to be, if heaped and size reduces


}

static void sustain_mode12_in(t_sustain *x, int pitch){
    //insert for mode 1 and 2
    //if not already in x_ord, add it
    int found = sustain_check_noteoff(x, pitch);
    if(!found){
        int nord = x->x_nord; //current number of entries
        sustain_check_arrsz(x,nord+1); //check if we have room for new entry
        x->x_ord[nord] = pitch;
        x->x_nord = nord + 1;
    };
    
}

static void sustain_mode0_in(t_sustain *x, int pitch){
    //insert for mode 0
    //no need for x_pcount, just keep track of order in x_ord
    //first check size
    int nord = x->x_nord; //current number of entries
    sustain_check_arrsz(x, nord+1); //check if we have room for new entry
    x->x_ord[nord] = pitch;
    x->x_nord = nord + 1; 
}

//mode 0: keep all noteoffs and repetitions in order
//mode 1: retrigger for repetitions (retriggers need no prev note off), keep original order
//mode 2: never mind retriggers or repetitions, just keep one note off per pitch
static void sustain_float(t_sustain *x, t_float f){
    int p = (int)f; //incoming pitch
    //bounds checking

    if(p >= 0 && p < SUSTAIN_NPCH){
        //x->x_vel simulated by list
        if(x->x_vel || !x->x_onoff){
            //if nonzero velocity or sustain is off, let pass
            
            if(x->x_mode == 1){
                //if mode 1, pitch already in x_nord (ie we got a note off message for it), send note off
                //ADDING, also allow if we've received a note on too - DK

                int prevnoteoff = sustain_check_noteoff(x, p);
                int prevnoteon = x->x_noteon[p]; //flag for previous note on


                if(prevnoteoff || prevnoteon){
	            outlet_float(x->x_velout, 0);
	            outlet_float(((t_object *)x)->ob_outlet, p);
                };
            };


	    outlet_float(x->x_velout, x->x_vel);
	    outlet_float(((t_object *)x)->ob_outlet, p);
            x->x_noteon[p] = 1; //note on flag

        }
        else{
            //if not, store
            if(x->x_mode == 0){
                sustain_mode0_in(x,p);
            }
            else{
                sustain_mode12_in(x,p);
            };

        };

    };
}

static void sustain_clear(t_sustain *x){
    if(x->x_heaped == 1){
        //if heaped, free heap and set pointer to stack
        free(x->x_ord);
        x->x_ord = x->x_ordst;
        x->x_heaped = 0;
    };
    //zero array and reset sizes
    x->x_arrsz = SUSTAIN_NORD;
    x->x_nord = 0;
    int i;
    for(i=0;i<SUSTAIN_NORD;i++){
        x->x_ord[i] = 0;
    };
    //clear out note on array
    for(i=0; i<SUSTAIN_NPCH; i++){
        x->x_noteon[i] = 0;
    };
}

static void sustain_flush(t_sustain *x){
    unsigned int i;
    for(i=0; i<x->x_nord; i++){
        int p = x->x_ord[i]; 
        outlet_float(x->x_velout, 0);
	outlet_float(((t_object *)x)->ob_outlet, p);
    };
    sustain_clear(x);
}


static void * sustain_free(t_sustain *x){

    if(x->x_heaped == 1){
        free(x->x_ord);
    };
    outlet_free(x->x_velout);
    return (void *)x;
}

static void sustain_mode(t_sustain *x, t_float _mode){
    int mode = (int)_mode;
    if(mode < 0){
        mode = 0;    
    }
    else if(mode > 2){
        mode = 2;
    };
    x->x_mode = mode;

}

static void sustain_onoff(t_sustain *x, t_float _onoff){
    if(_onoff > 0){
        x->x_onoff = 1;
    }
    else{
        x->x_onoff = 0;
    };

    if(x->x_onoff == 0 && x->x_nord > 0){
        sustain_flush(x);
    };
}


static void *sustain_new(t_symbol *s, int argc, t_atom *argv)
{ 
	t_sustain *x = (t_sustain *)pd_new(sustain_class);
        //set defaults
        t_float mode = 0; 
        t_float onoff = 0;
        while(argc){
            if(argv->a_type == A_SYMBOL){
                if(argc >= 2){
                    t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
                    t_float curfloat = atom_getfloatarg(1, argc, argv);
                    if(strcmp(cursym->s_name, "@repeatmode") == 0){
                        mode = curfloat;
                    }
                    else if(strcmp(cursym->s_name, "@sustain") == 0){
                        onoff = curfloat;
                    }
                    else{
                        goto errstate;
                    };
                    argc -=2;
                    argv += 2;
                }
                else{
                    goto errstate;
                };
            }
            else{
                goto errstate;
            };

        };
        x->x_ord = x->x_ordst;
        x->x_heaped = 0;
        x->x_vel = 0;
        sustain_clear(x);
        sustain_mode(x, mode);
        sustain_onoff(x, onoff);
        floatinlet_new((t_object *)x, &x->x_vel);
        inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("sustain"));
        outlet_new((t_object *)x, &s_float);
        x->x_velout = outlet_new((t_object *)x, &s_float);
	return (x);
	errstate:
		pd_error(x, "sustain: improper args");
		return NULL;
	
}

CYCLONE_OBJ_API void sustain_setup(void)
{
	sustain_class = class_new(gensym("sustain"), (t_newmethod)sustain_new, (t_method)sustain_free,
	sizeof(t_sustain), 0, A_GIMME, 0);
	class_addfloat(sustain_class, (t_method)sustain_float);
	class_addmethod(sustain_class, (t_method)sustain_mode,  gensym("repeatmode"), A_FLOAT, 0);
    class_addmethod(sustain_class, (t_method)sustain_onoff,  gensym("sustain"), A_FLOAT, 0);
    class_addmethod(sustain_class, (t_method)sustain_clear,  gensym("clear"), 0);
    class_addmethod(sustain_class, (t_method)sustain_flush,  gensym("flush"), 0);
}
