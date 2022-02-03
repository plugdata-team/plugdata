/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define FUNNEL_MINSLOTS   2
#define FUNNEL_INISIZE   32  /* LATER rethink */
#define FUNNEL_MAXSIZE  4096

/*
 FROM READING THE OLD CODE: there are the same number of proxies as inlets!
 the main inlet redirects to proxy[0], all other proxies have their own inlets


removing grow dependency
   IDEA:for each proxy, if the incoming message is beyond the size - 1, allocate a new t_atom on the heap,
 -routing everything through funnel_proxy_anything but setting list in funnel_msg_set first
 --offset just changed one proxy's offset, needed to change them all

before: p_value just stored one float which bang output, this was wrong. now outputs entire list
PROXY:
    removing    p_value;
    renaming     p_size to p_allocsz
    adding      p_outsz
    renaming  *p_message to *p_msg
    renaming  p_messini to p_mstack
    removing   p_entered
    adding     p_heaped
    adding    p_owner

EDITED:
    funnel_new
    funnel_set

REDONE:
    funnel_proxy_anything
    funnel_set
    funnel_proxy_symbol
    funnel_proxy_float
    funnel_proxy_bang
    funnel_offset
    funnel_proxy_offset

NEW:
    funnel_msg_set
    funnel_set
    funnel_proxy_set

REMOVED:
    funnel_list
 -Derek Kwan 2016 */


typedef struct _funnel
{
    t_object    x_ob;
    int         x_numactual; //number of actual successful proxies (when pd_new for proxy works)
    int         x_numprox;  //number of requested proxies
    t_pd      **x_proxies;
} t_funnel;

typedef struct _funnel_proxy
{
    t_object   p_ob;
    t_outlet  *p_out;   /* master outlet (the same value for each slot) */
    int        p_id;
    int        p_offset; /* to adjust id value */
    t_float    p_outsz; //size of outgoing message
    int        p_allocsz;  /* allocated size of message*/
    t_atom    *p_msg;  //point this to p_mstack if < FUNNEL_INISIZE, else allocate on heap - DK
    int        p_heaped; //adding this to indicate if using heap or not -DK
    t_atom     p_mstack[FUNNEL_INISIZE]; //the stack for the message -DK
    t_funnel  *p_owner;
} t_funnel_proxy;

static t_class *funnel_class;
static t_class *funnel_proxy_class;

static void funnel_msg_set(t_funnel_proxy *x, t_symbol *s, int argc, t_atom * argv){
    //new method, sets the message but doesn't output - Derek Kwan 2016
    int cursize = x->p_allocsz;
    int heaped = x->p_heaped;


    //comparing to argc sizes-1 because need one more slot for inlet number
    if(heaped && argc <= (FUNNEL_INISIZE - 1)){
        //if p_msg is pointing to a heap and incoming list can fit into FUNNEL_INISIZE-1
        //deallocate p_msg and point it to stack, update status
        freebytes((x->p_msg), (cursize) * sizeof(t_atom));
        x->p_msg = x->p_mstack;
        x->p_heaped = 0;
        x->p_allocsz = FUNNEL_INISIZE;
        }
    else if(heaped && argc > (FUNNEL_INISIZE-1) && argc > (cursize-1)){
        //if already heaped, incoming list can't fit into FUNNEL_INISIZE-1 and can't fit into allocated t_atom
        //reallocate to accomodate larger list, update status
        
        int toalloc = argc + 1; //size to allocate
        //bounds checking for maxsize
        if(toalloc > FUNNEL_MAXSIZE){
            toalloc = FUNNEL_MAXSIZE;
        };
        x->p_msg = (t_atom *)resizebytes(x->p_msg, cursize * sizeof(t_atom), toalloc * sizeof(t_atom));
        x->p_allocsz = toalloc;
    }
    else if(!heaped && argc > (FUNNEL_INISIZE-1)){
        //if not heaped and incoming list can't fit into FUNNEL_INISIZE-1
        //allocate and update status

        int toalloc = argc + 1; //size to allocate
        //bounds checking for maxsize
        if(toalloc > FUNNEL_MAXSIZE){
            toalloc = FUNNEL_MAXSIZE;
        };
        x->p_msg = (t_atom *)getbytes(toalloc * sizeof(t_atom));
        x->p_heaped = 1;
        x->p_allocsz = toalloc;
    };

    //if we're not dealing with these conditions, don't have to worry about it 
    //now to construct our list! first element is the inlet number
   int lpos = 0;//our current position in the constructed list
    t_float outid = (t_float)x->p_id + (t_float)x->p_offset; //the inlet it's coming from
    SETFLOAT(&x->p_msg[lpos],outid);
    lpos++;
    //we want to include the "selector" if it isn't "list" or "symbol" or "set" or "offset" 
    if(s){
         if(strcmp(s->s_name, "list") != 0 && strcmp(s->s_name, "symbol") != 0 && strcmp(s->s_name, "set") != 0  && strcmp(s->s_name, "offset") != 0) {
             SETSYMBOL(&x->p_msg[lpos],s);
             lpos++;
         };
    };

    //now copy the rest of the messages up to FUNNEL_MAXSIZE
    int mpos = 0;//position within incoming message
    while(lpos < FUNNEL_MAXSIZE && mpos < argc){
        if((argv+mpos)->a_type == A_FLOAT){
            //if current elt is a float
            t_float curfloat= atom_getfloatarg(mpos,argc, argv);;
            SETFLOAT(&x->p_msg[lpos],curfloat);
        }
        else{
            //else count it as a symbol
            t_symbol * cursym = atom_getsymbolarg(mpos, argc, argv);
            SETSYMBOL(&x->p_msg[lpos],cursym);
        };
        //increment
        mpos++;
        lpos++;
    };

    x->p_outsz = lpos;


}
static void funnel_proxy_anything(t_funnel_proxy *x,
			      t_symbol *s, int argc, t_atom *argv){


    funnel_msg_set(x, s, argc, argv);
    outlet_list(x->p_out, &s_list, x->p_outsz, x->p_msg);

}

static void funnel_proxy_set(t_funnel_proxy *x, t_symbol* s, int argc, t_atom* argv)
{

     funnel_msg_set(x,s,argc,argv);
}

static void funnel_proxy_float(t_funnel_proxy *x, t_float f)
{
    //since p_value changed, need to redo, use funnel_proxy_anything now - Derek Kwan 
    t_atom newatom[1];
    SETFLOAT(newatom, f);
    funnel_proxy_anything(x,0,1,newatom);
}


static void funnel_proxy_symbol(t_funnel_proxy *x, t_symbol *s)
{

    funnel_proxy_anything(x,s, 0, 0);
}

static void funnel_float(t_funnel *x, t_float f)
{
    funnel_proxy_float((t_funnel_proxy *)x->x_proxies[0], f);
}

static void funnel_symbol(t_funnel *x, t_symbol *s)
{
    funnel_proxy_symbol((t_funnel_proxy *)x->x_proxies[0], s);
}

static void funnel_set(t_funnel *x, t_symbol* s, int argc, t_atom* argv)
{

    funnel_proxy_set((t_funnel_proxy *)x->x_proxies[0], s, argc, argv);
}

static void funnel_anything(t_funnel *x, t_symbol *s, int ac, t_atom *av)
{
    funnel_proxy_anything((t_funnel_proxy *)x->x_proxies[0], s, ac, av);
}


static void funnel_proxy_bang(t_funnel_proxy *x)
{
 
    outlet_list(x->p_out, &s_list, x->p_outsz, x->p_msg);
    
}

static void funnel_bang(t_funnel *x)
{
    funnel_proxy_bang((t_funnel_proxy *)x->x_proxies[0]);
}


static void funnel_offset(t_funnel *x, t_floatarg f)
{
    int i;
    for(i=0;i<x->x_numactual; i++){
        t_funnel_proxy *p = (t_funnel_proxy *)x->x_proxies[i];
        p->p_offset = f;
    };

}

static void funnel_proxy_offset(t_funnel_proxy *x, t_floatarg f)
{
    t_funnel *y = x->p_owner;
    funnel_offset(y, f);
}


static void funnel_free(t_funnel *x)
{
    if (x->x_proxies)
    {
	int i = x->x_numactual;
	while (i--)
	{
	    t_funnel_proxy *y = (t_funnel_proxy *)x->x_proxies[i];
            //if we've allocated heaps, we wanna free those - Derek Kwan
            if(y->p_heaped){
                freebytes((y->p_msg), (y->p_allocsz) * sizeof(t_atom));
            };
	    pd_free((t_pd *)y);
	}
	freebytes(x->x_proxies, x->x_numprox * sizeof(*x->x_proxies));
    }
}

static void *funnel_new(t_floatarg f1, t_floatarg f2)
{
    t_funnel *x;
    int i, numactual, numprox = (int)f1;
    int id = 0;
    int offset = (int)f2;
    t_outlet *out;
    t_pd **proxies;
    t_atom * p_msg_def = getbytes(sizeof(*p_msg_def)); //default value for p_msg for inlets
    
    SETFLOAT(p_msg_def, 0);
    
    if (numprox < 1)  // can create single inlet funnel, but default to 2
	numprox = FUNNEL_MINSLOTS;
    if (!(proxies = (t_pd **)getbytes(numprox * sizeof(*proxies))))
	return (0);
    for (numactual = 0; numactual < numprox; numactual++)
	if (!(proxies[numactual] = pd_new(funnel_proxy_class))) break;
    if (!numactual)
    {
	freebytes(proxies, numprox * sizeof(*proxies));
	return (0);
    }
    x = (t_funnel *)pd_new(funnel_class);
    x->x_numactual = numactual;
    x->x_numprox = numprox;
    x->x_proxies = proxies;
    out = outlet_new((t_object *)x, &s_list);
    for (i = 0; i < numactual; i++)
    {
	t_funnel_proxy *y = (t_funnel_proxy *)proxies[i];
	y->p_out = out;
    y->p_offset = offset;
	y->p_id = id++;
	y->p_outsz = 0;
        y->p_allocsz = FUNNEL_INISIZE;
        y->p_heaped = 0;
        y->p_msg = y->p_mstack; //initially point pointer to heaped t_atom
        //don't make new inlet for proxy[0], make for others
        y->p_owner = x;
	funnel_proxy_set(y,0,1,p_msg_def); //set default p_msg to 0
	if (i) inlet_new((t_object *)x, (t_pd *)y, 0, 0);
    };

    freebytes(p_msg_def, sizeof(*p_msg_def));
    return (x);
}

CYCLONE_OBJ_API void funnel_setup(void)
{
    funnel_class = class_new(gensym("funnel"), (t_newmethod)funnel_new,
        (t_method)funnel_free, sizeof(t_funnel), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(funnel_class, funnel_bang);
    class_addfloat(funnel_class, funnel_float);
    class_addsymbol(funnel_class, funnel_symbol);
    class_addlist(funnel_class, funnel_anything);
    class_addanything(funnel_class, funnel_anything);
    class_addmethod(funnel_class, (t_method)funnel_set, gensym("set"), A_GIMME, 0);
    class_addmethod(funnel_class, (t_method)funnel_offset, gensym("offset"), A_FLOAT, 0);
// proxy
    funnel_proxy_class = class_new(gensym("_funnel_proxy"), 0, 0,
                                   sizeof(t_funnel_proxy),CLASS_PD | CLASS_NOINLET, 0);
    // proxy methods
    class_addbang(funnel_proxy_class, funnel_proxy_bang);
    class_addfloat(funnel_proxy_class, funnel_proxy_float);
    
// new proxy methods // add symbol, anything, offset, set
    class_addsymbol(funnel_proxy_class, funnel_proxy_symbol);
    class_addlist(funnel_proxy_class, funnel_proxy_anything);
    class_addanything(funnel_proxy_class, funnel_proxy_anything);
    class_addmethod(funnel_proxy_class, (t_method)funnel_proxy_set, gensym("set"), A_GIMME, 0);
    class_addmethod(funnel_proxy_class, (t_method)funnel_proxy_offset, gensym("offset"), A_FLOAT, 0);
}
