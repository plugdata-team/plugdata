/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Derek Kwan 2017 - pretty much entirely redone

// Old version was quite buggy, and some of the bugs were replicated from max,
// even though they ruined the purpose of the object in some cases with list input.
// This version fixes it (porres)

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define PAST_STACK 128 // check
#define PAST_MAX 512 // check
typedef struct _past
{
    t_object   x_obj;
    t_float *   x_arg;
    t_float    x_argstack[PAST_STACK];
    int        x_allocsz; //size of allocated (or not) arglist
    int        x_argsz; //size of stored arg list
    int        x_heaped; //if x_arg is allocated
    int        x_past; //if past threshold

} t_past;

static t_class *past_class;

//value allocation helper function
static void past_alloc(t_past *x, int argc){
   int cursize = x->x_allocsz;
    int heaped = x->x_heaped;


    if(heaped && argc <= PAST_STACK){
        //if x_arg is pointing to a heap and incoming list can fit into PAST_STACK
        //deallocate x_arg and point it to stack, update status
        freebytes((x->x_arg), (cursize) * sizeof(t_float));
        x->x_arg = x->x_argstack;
        x->x_heaped = 0;
        x->x_allocsz = PAST_STACK;
        }
    else if(heaped && argc > PAST_STACK && argc > cursize){
        //if already heaped, incoming list can't fit into PAST_STACK and can't fit into allocated t_atom
        //reallocate to accomodate larger list, update status
        
        int toalloc = argc; //size to allocate
        //bounds checking for maxsize
        if(toalloc > PAST_MAX){
            toalloc = PAST_MAX;
        };
        x->x_arg = (t_float *)resizebytes(x->x_arg, cursize * sizeof(t_float), toalloc * sizeof(t_float));
        x->x_allocsz = toalloc;
    }
    else if(!heaped && argc > PAST_STACK){
        //if not heaped and incoming list can't fit into PAST_STACK
        //allocate and update status

        int toalloc = argc; //size to allocate
        //bounds checking for maxsize
        if(toalloc > PAST_MAX){
            toalloc = PAST_MAX;
        };
        x->x_arg = (t_float *)getbytes(toalloc * sizeof(t_float));
        x->x_heaped = 1;
        x->x_allocsz = toalloc;
    };


}

static int past_set_helper(t_past * x, int argc, t_atom * argv)
{
    t_float* parse = t_getbytes(argc * sizeof(*parse));
    int i, errors = 0;
    for(i=0; i < argc; i++){
        if((argv+i)->a_type == A_FLOAT){
            parse[i] = atom_getfloatarg(i, argc, argv);
        }
        else{
            errors = 1;
            break;
        };
    };

    if(errors){
        //not a legit list, error out
        return 1;
    }
    else{
        past_alloc(x, argc);
        if(argc > PAST_MAX){
            argc = PAST_MAX;
        };
       memcpy(x->x_arg, parse, sizeof(t_float) * argc);
        x->x_argsz = argc;
        return 0;
    };
    t_freebytes(parse, argc * sizeof(*parse));
}

static void past_set(t_past *x, t_symbol *s, int argc, t_atom * argv)
{
    int failset = past_set_helper(x, argc, argv);
    if(failset)
        {
        pd_error(x, "past: improper args");
        return;
        }
}

static void past_list(t_past *x, t_symbol *s, int argc, t_atom * argv)
{
    int argsz = x->x_argsz;
    if(argc < argsz){
        //not enough input args to compare to compare args, don't bother
        x->x_past = 0;
        return;
    }
    else{
        int i, past = 1;
        t_float inputf, cmpf;
        for(i=0; i < argsz; i++){
            if((argv+i) -> a_type == A_FLOAT)
            {
                inputf = atom_getfloatarg(i, argc, argv);
                cmpf = x->x_arg[i]; //float to compare input to
                if(inputf <= cmpf){
                    //condition not satisfied, break
                    past = 0;
                    break;
                };
            }
            else
            {
                //incoming not a float, break
                past = 0;
                break;
            };

        };

        //if below thresh before and now above, bang
        if(past && x->x_past == 0)
        {
	    outlet_bang(x->x_obj.ob_outlet);
        };

        x->x_past = past;
    };
}

static void past_float(t_past *x, t_float f)
{
    t_atom fltlist[1];
    SETFLOAT(fltlist, f);
    past_list(x, 0, 1, fltlist);
}

static void past_clear(t_past *x)
{
    x->x_past = 0;
}

static void *past_new(t_symbol *s, int argc, t_atom *argv)
{
    t_past *x = (t_past *)pd_new(past_class);
    int failset = 0; //if setting args failed
    x->x_past = 0;
    x->x_allocsz = PAST_STACK;
    x->x_arg = x->x_argstack;
    x->x_heaped = 0;
    x->x_argsz = 0;
    if(argc){
        failset = past_set_helper(x, argc, argv);
    };
    if(!failset){
        outlet_new((t_object *)x, &s_bang);
        return (x);
    }
    else{
	pd_error(x, "past: improper args");
        return NULL; 
    };
}			

static void *past_free(t_past *x)
{
    if(x->x_heaped)
    {
        freebytes(x->x_arg, x->x_allocsz * sizeof(t_float));
    };
    return (void *)x;
}

CYCLONE_OBJ_API void past_setup(void)
{
    past_class = class_new(gensym("past"),
			   (t_newmethod)past_new,
			   (t_method)past_free,
			   sizeof(t_past), 0, A_GIMME, 0);
    class_addfloat(past_class, past_float);
    class_addlist(past_class, past_list);
    class_addmethod(past_class, (t_method)past_clear, gensym("clear"), 0);
    class_addmethod(past_class, (t_method)past_set, gensym("set"), A_GIMME, 0);
}
