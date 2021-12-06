#include "m_pd.h"
#include <string.h>
#include <stdlib.h>

#define suspedal_NPCH  128 // number of midi pitches
#define suspedal_NORD  500 // size of incoming order array on stack
#define suspedal_ADDSZ 100 // size to make x_ord bigger by

static t_class *suspedal_class;

typedef struct _suspedal{
	t_object             x_obj;
	t_float              x_vel; // velocity
        int             *x_ord; // pointer to order array, points to x_ordst at first
        int             x_ordst[suspedal_NORD]; // order of incoming notes on the stack
        unsigned int    x_array_size; // size of order array
        unsigned int    x_nord; // actual number of entries in x_ord
        int             x_heaped; // if x_ord points to heap
        int             x_retrig;
        int             x_tonal;
        t_int           x_status;
        t_outlet       *x_velout; // velocity outlet
        int             x_noteon[suspedal_NPCH]; // for tonal mode and retrig modes
}t_suspedal;

static int suspedal_check_noteoff(t_suspedal *x, int pitch){
    unsigned int i;
    for(i = 0; i < x->x_nord; i++){ // find pitch in x->x_ord
        if(pitch == x->x_ord[i])
            return 1;
    };
    return 0;
}

static void suspedal_check_arrsz(t_suspedal *x, int sz){
    int cursz = x->x_array_size; // current size
    int heaped = x->x_heaped; // if x_ord points to heap
    if(heaped && sz > cursz){
        // if heaped and we need more space, make more space
        int newsz = cursz + suspedal_ADDSZ; //proposed new size
        while(newsz < sz)
            newsz += suspedal_ADDSZ; //tack on more chunks until we have the size we need
        // reallocate and we're good to go
        x->x_ord = (int *)realloc(x->x_ord, sizeof(int)*newsz);
        x->x_array_size = newsz;
    }
    else if(!heaped && sz > suspedal_NORD){
        //if not heaped and can't fit in stack var, allocate on heap
        int newsz = suspedal_NORD + suspedal_ADDSZ; //proposed new size
        // add more space if needed
        while(newsz < sz)
            newsz += suspedal_ADDSZ;
        //allocate on heap
        int * newarr = (int *)malloc(sizeof(int)*newsz);
        //copy stack contents to heap
        memcpy(newarr, x->x_ord, sizeof(int)*suspedal_NORD);
        //point to heap
        x->x_ord = newarr;
        x->x_array_size = newsz;
        x->x_heaped = 1;
    }
    else if(heaped && sz < suspedal_NORD){
        //if we're heaped and don't need to be anymore
        //copy heap contents to stack
        memcpy(x->x_ordst, x->x_ord, sizeof(int)*suspedal_NORD);
        //free and repoint to stack
        free(x->x_ord);
        x->x_ord = x->x_ordst;
        x->x_array_size = suspedal_NORD;
        x->x_heaped = 0;
    };
    // don't worry about other cases:
    // if not heaped and doesn't need to be, if heaped and size reduces
}

static void suspedal_float(t_suspedal *x, t_float f){
    int pitch = (int)f;
    if(pitch >= 0 && pitch < suspedal_NPCH){ // bounds checking
//        post("x->x_vel = %f / x->x_status = %d", x->x_vel, x->x_status);
        if(!x->x_status){ // suspedal Off => Pass and register note-ons
            outlet_float(x->x_velout, x->x_vel);
            outlet_float(((t_object *)x)->ob_outlet, pitch);
            x->x_noteon[pitch] = x->x_vel != 0; // note on flag
        }
        else{ // suspedal On
            if(x->x_tonal){ // Tonal mode
                if(x->x_vel){ // If Note-On
                    if(x->x_noteon[pitch] > 0){ // note is being sustained
                        x->x_noteon[pitch] = 1;
                        if(x->x_retrig == 0){ // don't retrigger (ignore repeated)
                            if(!suspedal_check_noteoff(x, pitch)){ // pass only if pitch is not in x->x_ord
                                outlet_float(x->x_velout, x->x_vel);
                                outlet_float(((t_object *)x)->ob_outlet, pitch);
                            }
                        }
                        else if(x->x_retrig == 1 || x->x_retrig == 3){ // (retrigger/free later) - just pass
                            outlet_float(x->x_velout, x->x_vel);
                            outlet_float(((t_object *)x)->ob_outlet, pitch);
                        }
                        else if(x->x_retrig == 2){ // 'free first'
                            if(suspedal_check_noteoff(x, pitch)){ // if pitch is in x->x_ord, retrigger
                                outlet_float(x->x_velout, 0);
                                outlet_float(((t_object *)x)->ob_outlet, pitch);
                            }
                            outlet_float(x->x_velout, x->x_vel);
                            outlet_float(((t_object *)x)->ob_outlet, pitch);
                        }
                    }
                    else{ // note is not being sustained: just pass
                        outlet_float(x->x_velout, x->x_vel);
                        outlet_float(((t_object *)x)->ob_outlet, pitch);
                    }
                }
                else{ // Note-Off => ADD
                    if(x->x_noteon[pitch] > 0){
                        if(x->x_retrig == 3){ // mode 3 - 'free later': always add
                            suspedal_check_arrsz(x, x->x_nord+1); // check if we have room for new entry
                            x->x_ord[x->x_nord] = pitch;
                            x->x_nord++;
                        }
                        else{ // mode 0, 1, or 2
                            if(!suspedal_check_noteoff(x, pitch)){ // only add if not found in "x->x_ord"
                                suspedal_check_arrsz(x, x->x_nord+1); // check if we have room for new entry
                                x->x_ord[x->x_nord] = pitch;
                                x->x_nord++;
                            }
                            x->x_noteon[pitch] = 2; // note on flag
                        }
                    }
                    else{ // just pass
                        outlet_float(x->x_velout, x->x_vel);
                        outlet_float(((t_object *)x)->ob_outlet, pitch);
                    }
                }
            }
            else{ // regular sustain mode
                if(x->x_vel){ // If Note-On
                    if(x->x_retrig == 0){ // Mode 0: don't retrigger (ignore repeated)
                        if(!suspedal_check_noteoff(x, pitch)){ // pass only if pitch is not in x->x_ord
                            outlet_float(x->x_velout, x->x_vel);
                            outlet_float(((t_object *)x)->ob_outlet, pitch);
                            x->x_noteon[pitch] = 1; // note on flag
                        }
                        x->x_noteon[pitch] = 1; // note on flag
                    }
                    else if(x->x_retrig == 1 || x->x_retrig == 3){ //  mode 1/3 - just pass
                        outlet_float(x->x_velout, x->x_vel);
                        outlet_float(((t_object *)x)->ob_outlet, pitch);
                        x->x_noteon[pitch] = 1; // note on flag
                    }
                    else if(x->x_retrig == 2){ // Mode 2: 'free first'
                        if(suspedal_check_noteoff(x, pitch)){ // if pitch is in x->x_ord, retrigger
                            outlet_float(x->x_velout, 0);
                            outlet_float(((t_object *)x)->ob_outlet, pitch);
                        }
                        outlet_float(x->x_velout, x->x_vel);
                        outlet_float(((t_object *)x)->ob_outlet, pitch);
                        x->x_noteon[pitch] = 1; // note on flag
                    }
                }
                else{ // Note-Off => ADD
                    if(x->x_retrig == 3){ // mode 3 - 'free later': always add
                        suspedal_check_arrsz(x, x->x_nord+1); // check if we have room for new entry
                        x->x_ord[x->x_nord] = pitch;
                        x->x_nord++;
                    }
                    else{ // mode 0, 1, or 2
                        if(!suspedal_check_noteoff(x, pitch)){ // only add if not found in "x->x_ord"
                            suspedal_check_arrsz(x, x->x_nord+1); // check if we have room for new entry
                            x->x_ord[x->x_nord] = pitch;
                            x->x_nord++;
                        }
                        x->x_noteon[pitch] = 0;
                    }
                }
            }
        }
    }
}

static void suspedal_clear(t_suspedal *x){
    if(x->x_heaped == 1){ //if heaped, free heap and set pointer to stack
        free(x->x_ord);
        x->x_ord = x->x_ordst;
        x->x_heaped = 0;
    };
    x->x_array_size = suspedal_NORD;
    x->x_nord = 0;
    int i;
    for(i = 0; i < suspedal_NORD; i++)
        x->x_ord[i] = 0;
    for(i = 0; i < suspedal_NPCH; i++) // clear noteon array
        x->x_noteon[i] = 0;
}

static void suspedal_flush(t_suspedal *x){
    unsigned int i;
    for(i = 0; i < x->x_nord; i++){
        int pitch = x->x_ord[i];
        if(x->x_retrig == 3){
            outlet_float(x->x_velout, 0);
            outlet_float(((t_object *)x)->ob_outlet, pitch);
        }
        else{ // mode 0, 1 or 2
            if(x->x_noteon[pitch] != 1){ // ignore if note on
                outlet_float(x->x_velout, 0);
                outlet_float(((t_object *)x)->ob_outlet, pitch);
            }
        }
    };
    suspedal_clear(x);
}

static void * suspedal_free(t_suspedal *x){
    if(x->x_heaped == 1)
        free(x->x_ord);
    outlet_free(x->x_velout);
    return (void *)x;
}

static void suspedal_tonal(t_suspedal *x, t_float f){
    x->x_tonal = f != 0;
}

static void suspedal_retrig(t_suspedal *x, t_float f){
    int mode = (int)f;
    if(mode < 0)
        mode = 0;
    else if(mode > 3)
        mode = 3;
    x->x_retrig = mode;
}

static void suspedal_status(t_suspedal *x, t_float f){
    x->x_status = f != 0;
    if(!x->x_status && x->x_nord > 0)
        suspedal_flush(x);
}

static void *suspedal_new(t_symbol *s, int argc, t_atom *argv){
	t_suspedal *x = (t_suspedal *)pd_new(suspedal_class);
        t_float retrig = 0; 
        t_float status = 0;
        t_int floatarg = 0;
        x->x_tonal = 0;
        while(argc){
            if(argv->a_type == A_FLOAT && !floatarg){
                status = atom_getfloatarg(0, argc, argv) != 0;
                argc--;
                argv++;
                floatarg = 1;
            }
            else if(argv->a_type == A_SYMBOL){
                t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
                if(!strcmp(cursym->s_name, "-retrig")){
                    if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                        t_float curfloat = atom_getfloatarg(1, argc, argv);
                        retrig = curfloat;
                        argc -=2;
                        argv += 2;
                    }
                    else
                        goto errstate;
                }
                else if(!strcmp(cursym->s_name, "-tonal")){
                    x->x_tonal = 1;
                    argc--;
                    argv++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        };
        x->x_ord = x->x_ordst;
        x->x_heaped = x->x_vel = 0;
        suspedal_clear(x);
        suspedal_retrig(x, retrig);
        suspedal_status(x, status);
        floatinlet_new((t_object *)x, &x->x_vel);
        inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("sustain"));
        outlet_new((t_object *)x, &s_float);
        x->x_velout = outlet_new((t_object *)x, &s_float);
	return(x);
	errstate:
		pd_error(x, "suspedal: improper args");
		return NULL;
}

void suspedal_setup(void){
	suspedal_class = class_new(gensym("suspedal"), (t_newmethod)suspedal_new, (t_method)suspedal_free,
                sizeof(t_suspedal), 0, A_GIMME, 0);
    class_addfloat(suspedal_class, (t_method)suspedal_float);
    class_addmethod(suspedal_class, (t_method)suspedal_tonal, gensym("tonal"), A_FLOAT, 0);
	class_addmethod(suspedal_class, (t_method)suspedal_retrig, gensym("retrig"), A_FLOAT, 0);
    class_addmethod(suspedal_class, (t_method)suspedal_status, gensym("sustain"), A_FLOAT, 0);
    class_addmethod(suspedal_class, (t_method)suspedal_clear, gensym("clear"), 0);
    class_addmethod(suspedal_class, (t_method)suspedal_flush, gensym("flush"), 0);
}
