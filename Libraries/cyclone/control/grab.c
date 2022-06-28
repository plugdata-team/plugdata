/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Matt Barber added wizadry that makes grab talk to built-in receive names (2021)

#include "m_pd.h"
#include <common/api.h>
#include "m_imp.h"
#include "common/magicbit.h"

// LATER handle canvas grabbing (bypass)
// LATER check self-grabbing

// It would be nice to have write access to o_connections field...

struct _outlet{
    t_object        *o_owner;
    struct _outlet  *o_next;
    t_outconnect    *o_connections;
    t_symbol        *o_sym;
};

// ...and to have bindlist traversal routines in Pd API.

static t_class *bindlist_class = 0; // global variable

typedef struct _bindelem{
    t_pd                *e_who;
    struct _bindelem    *e_next;
}t_bindelem;

typedef struct _bindlist{
    t_pd b_pd;
    t_bindelem          *b_list;
}t_bindlist;

typedef struct _grab{
    t_object        x_obj;
    t_symbol       *x_rcv_name;     // the receive name to grab from
    int             x_noutlets;     // number of outlets to grab from
    t_outlet       *x_rightout;     // rightmost outlet (if there's no receive name)
    t_object	   *x_receiver;     // object containing the receiver
    int				x_maxcons;	    // maximum number of connections from receiver
    t_object      **x_grabbed;      // array of grabbed objects
    t_outconnect  **x_grabcons;     // array of grabbed connections
    int            *x_ngrabout;     // array; number of grabbed object's outlets
// traversal helpers:
    t_outconnect   *x_tograbbedcon; // a connection to grabbed object
    t_bindelem     *x_bindelem;
    int             x_nonreceive;   // flag to signal whether processed non-[receive] object
}t_grab;

static t_class *grab_class;

// assign the receiver and init buffers for storing objects & connections
static int grab_prep(t_grab *x, t_object *obj){
	t_outlet *op;       // outlet pointer
	t_outconnect *ocp;  // outlet connection pointer
	t_object *dummy;
	int ncons, inno;
	if(x->x_rcv_name){
		x->x_receiver = obj;
		op = obj->ob_outlet;
	}
	else
        op = x->x_rightout;
    /* if receiver isn't a [receive] object, we want to grab its outlet 
       so there's only one "connection." Otherwise we need to count the
       connections to the objects we're going to grab. If there aren't 
       any, there's nothing left to do. */
	if(x->x_receiver && (x->x_receiver->te_g.g_pd->c_name != gensym("receive")))
		ncons = 1;
	else{ // not a [receive] object, grab connections
        if(!(x->x_tograbbedcon = ocp = magic_outlet_connections(op)))
            return(0);
        for(ncons = 0; ocp ; ++ncons)
            ocp = magic_outlet_nextconnection(ocp, &dummy, &inno);
	}
	/* initialize three buffers.
	   x_grabbed stores pointers to the objects we're going to grab.
	   x_ngrabout stores the number of outlets we're grabbing per grabbed object.
	      this isn't strictly necessary since you can always count the outlets,
	      but it saves a potentially costly extra traversal of outlets at the
	      grab_restore step to save the count the first time they're traversed
	   x_grabcons stores pointers to the t_outconnect connection lists for each
	      grabbed outlet. The max we would need to store is number of grabbed
	      objects times the number of [grab]'s outlets.*/
	if(!x->x_grabbed){
		if(!((x->x_grabbed = getbytes(ncons * sizeof(*x->x_grabbed))) &&
			 (x->x_ngrabout = getbytes(ncons * sizeof(*x->x_ngrabout))) &&
			(x->x_grabcons = getbytes(ncons * x->x_noutlets * sizeof(*x->x_grabcons)))))
		{
			pd_error(x, "grab: error allocating memory");
			return(0);
		}
		x->x_maxcons = ncons;
	}
	// get more memory if we need to because of the number of connecctions
	else if(ncons > x->x_maxcons){
		if(!((x->x_grabbed = resizebytes(x->x_grabbed,
			x->x_maxcons * sizeof(*x->x_grabbed),
			ncons * sizeof(*x->x_grabbed))) &&

			(x->x_ngrabout = resizebytes(x->x_ngrabout,
			x->x_maxcons * sizeof(*x->x_ngrabout),
			ncons * sizeof(*x->x_ngrabout))) &&
			
			(x->x_grabcons = resizebytes(x->x_grabcons,
			x->x_maxcons * x->x_noutlets * sizeof(*x->x_grabcons),
			ncons * x->x_noutlets * sizeof(*x->x_grabcons)))))
		{
			pd_error(x, "grab: error allocating memory");
			return(0);
		}
		x->x_maxcons = ncons;
	}
	return(1);
}

// run prep routines and find bindlist for the send/receive symbol
static void grab_start(t_grab *x){
    x->x_tograbbedcon = 0;
    x->x_bindelem = 0;
    x->x_receiver = 0;
    x->x_nonreceive = 0;
    if(x->x_rcv_name){ // if we're looking for a receive name
     // Two things can be bound to a symbol and stored in s_thing. If it's only one object, it's
     // stored as an object. If there's more, then it makes a bindlist, (a linked list of objects)
        t_pd *proxy = x->x_rcv_name->s_thing;
        t_object *obj;
        if(proxy && bindlist_class){
            if(*proxy == bindlist_class){ // if proxy is a bindlist
                // traverse the list until we find a patchable
                // object ([receive], gatom, etc) with "pd_checkobject"
                x->x_bindelem = ((t_bindlist *)proxy)->b_list;
                while(x->x_bindelem){
                	// once we find it, prep it and go from there
                    if((obj = pd_checkobject(x->x_bindelem->e_who)))
                        if(grab_prep(x, obj)) // why 'if'???
                        	return;
                    x->x_bindelem = x->x_bindelem->e_next;
                }
            }
            // it's a single object otherwise. If patchable, prep it
            else if((obj = pd_checkobject(proxy)))
                grab_prep(x, obj);
        }
    }
    else // not looking for receive name
        grab_prep(x, &x->x_obj); // prep [grab] and assign x_rightout as the proxy
}

/* grab_next is where all the work gets done. For reference, obj_starttraverseoutlet
   takes three arguments: an object, an address to store a pointer to one of its outlets
   and the outlet number, indexed from zero (left to right). It looks in the object, finds
   the n'th outlet, puts a pointer to it in the address, and then returns that outlet's
   connection list. */
static int grab_next(t_grab *x){
    // check if buffers are available and bail if they aren't
	if(!(x->x_grabbed && x->x_grabcons && x->x_ngrabout))
		return(0);
	// pointers to traverse the buffers
	t_object **grabbedp = x->x_grabbed;
	t_outconnect **grabconsp = x->x_grabcons;
	int *ngraboutp = x->x_ngrabout;
	t_object *gr; // the grabbed object below the [receive].
	int inno;
	t_outlet *op; // dummy outlet pointer required for obj_starttraverseoutlet
	t_outlet *goutp; // the outlet pointer; we need to get its connections
nextremote:
	// if the receiver is not a [receive] (gatom, nbx, etc.), grab directly from it
	if(x->x_receiver && !(x->x_nonreceive) && (x->x_receiver->te_g.g_pd->c_name != gensym("receive"))){
		*grabbedp = x->x_receiver; // store the receiver itself in the object buffer
		*ngraboutp = 1; // they only have one outlet (it could be more flexible for externals)
		// store the connection list for that outlet
		*grabconsp = obj_starttraverseoutlet(x->x_receiver, &goutp, 0);
		/* now, we overwrite the outlet's connection list with the connection list
		   from our [grab]'s 0th outlet. This essentially connects the grabbed receiver
		   to whatever [grab] is connected to */
		goutp->o_connections = obj_starttraverseoutlet((t_object *)x, &op, 0);
		/* this is a flag to let us know we've done the work for this object; otherwise
		   we get an infinite loop in the grab_bang, grab_float, etc. routines. There's
		   probably a better way to do this... */
		x->x_nonreceive = 1;
		/* return the number of objects we stored */
		return(1);
	}
	// otherwise, it's a [receive] or [grab]'s right outlet
    else if(x->x_tograbbedcon){
    	/* traverse the connections from x_receiver or x_rightout, one by one */
		while(x->x_tograbbedcon){
			/* get the next connection and the object it's connected to. This
			   will break the loop once there aren't any new connections stored,
			   and also it will signal the else if directly above to jump down
			   the next time grab_next is called */
			x->x_tograbbedcon = magic_outlet_nextconnection(x->x_tograbbedcon, &gr, &inno);
			if(gr){ // if it's an object?
				// if not connected to 0th inlet, don't store the object or its connections
				if(inno){
					if(x->x_rightout)
						pd_error(x, "[grab]: right outlet must feed leftmost inlet");
					else
						pd_error(x, "[grab]: remote receive object must feed leftmost inlet");
				}
				else{
					// store the object
					*grabbedp++ = gr;
					/* store the number of outlets (pd has to traverse the outlet list
					   in order to do this, which is costly for a lot of outlets).*/
					int goutno = obj_noutlets(gr);
					/* but we won't be grabbing more outlets than we have outlets in [grab] */
					if(goutno > x->x_noutlets) goutno = x->x_noutlets;
					/* store the number of outlet's we're grabbing. It's <= x_noutlets */
					*ngraboutp++ = goutno;
					/* now we get the connection lists for each outlet in turn. */
					for(int i = 0; i < goutno; i++){
						// put i'th outlet in goutp, return its connection list and store it
						*grabconsp++ = obj_starttraverseoutlet(gr, &goutp, i);
						/* now replace its connection list with the connection list from our
						   [grab]'s ith outlet. This essentially connects the grabbed object's
                           ith outlet to whatever [grab]'s ith outlet  is connected to */
						goutp->o_connections = obj_starttraverseoutlet((t_object *)x, &op, i);
					}
				}
			}
		}
		return(grabbedp-x->x_grabbed); // return number of objects stored
    }
    // once we've processed and done all of one [receive]'s objects, it's on to
    // the next receiver in the bindlist. Run prep again and then goto the top.
    if(x->x_bindelem){
        while((x->x_bindelem = x->x_bindelem->e_next)){
            t_object *obj;
            if((obj = pd_checkobject(x->x_bindelem->e_who))){
                x->x_nonreceive = 0;
                grab_prep(x, obj);
                grabbedp = x->x_grabbed;
                grabconsp = x->x_grabcons;
                ngraboutp = x->x_ngrabout;
                goto
                    nextremote;
            }
        }
    }
    return(0);
}

// restore's all grabbed objects' connection lists to their outlets
static void grab_restore(t_grab *x, int nobs){
	t_object **grabbedp = x->x_grabbed;
	t_outconnect **grabconsp = x->x_grabcons;
	int *ngraboutp = x->x_ngrabout;
	int goutno;
	t_object *gr;
	t_outlet *goutp;
	// grab_next returns the number of objects it's stored, so we can pass it to
	// grab_restore in order to tell it how many objects to restore from the buffers
	while(nobs--){
		gr = *grabbedp++; // get the object
		goutno = *ngraboutp++; // get the number of outlets (it is <= x_noutlets)
		for(int i = 0; i < goutno ; i++){ // restore the connection lists to each outlet in turn
			obj_starttraverseoutlet(gr, &goutp, i);
			goutp->o_connections = *grabconsp++;
		}
	}
}

// All methods are pretty much like this
static void grab_float(t_grab *x, t_float f){
	int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
    	/* we're grabbing from a receiver, then we've already rewired all of the connections
    	   from the grabbed objects to [grab]. Then we call that receiver's float method,
    	   and the output from the grabbed objects comes through [grab] instead. */
    	if(x->x_receiver)
        	pd_float(&x->x_receiver->ob_pd, f);
        else  // otherwise, send the float out the right outlet
        	outlet_float(x->x_rightout, f);
        grab_restore(x, nobs);
    }
}

static void grab_bang(t_grab *x){
    int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
        if(x->x_receiver)
            pd_bang(&x->x_receiver->ob_pd);
        else
            outlet_bang(x->x_rightout);
        grab_restore(x, nobs);
    }
}

static void grab_symbol(t_grab *x, t_symbol *s){
    int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
    	if(x->x_receiver)
        	pd_symbol(&x->x_receiver->ob_pd, s);
        else
        	outlet_symbol(x->x_rightout, s);
        grab_restore(x, nobs);
    }
}

static void grab_pointer(t_grab *x, t_gpointer *gp){
    int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
    	if(x->x_receiver)
        	pd_pointer(&x->x_receiver->ob_pd, gp);
        else
        	outlet_pointer(x->x_rightout, gp);
        grab_restore(x, nobs);
    }
}

static void grab_list(t_grab *x, t_symbol *s, int ac, t_atom *av){
    int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
    	if(x->x_receiver)
        	pd_list(&x->x_receiver->ob_pd, s, ac, av);
        else
        	outlet_list(x->x_rightout, s, ac, av);
       grab_restore(x, nobs);
    }
}

static void grab_anything(t_grab *x, t_symbol *s, int ac, t_atom *av){
    int nobs;
    grab_start(x);
    while((nobs = grab_next(x))){
    	if(x->x_receiver)
        	pd_anything(&x->x_receiver->ob_pd, s, ac, av);
        else
        	outlet_anything(x->x_rightout, s, ac, av);
       grab_restore(x, nobs);
    }
}

// set receive name
static void grab_set(t_grab *x, t_symbol *s){
    if(x->x_rcv_name && s && s != &s_)
        x->x_rcv_name = s;
}

static void *grab_new(t_symbol *s, int ac, t_atom *av){
    t_grab *x = (t_grab *)pd_new(grab_class);
    int noutlets = 1;
    int rightout = 1;
    x->x_rcv_name = 0;
    if(ac){
        if(av->a_type != A_FLOAT)
            goto errstate;
        else{
            noutlets = av->a_w.w_float < 1 ? 1 : (int)(av->a_w.w_float);
            ac--;
            av++;
        }
        if(ac){
            if(av->a_type != A_SYMBOL)
                goto errstate;
            else{
                x->x_rcv_name = av->a_w.w_symbol;
                rightout = 0;
                ac--;
                av++;
            }
        }
        if(ac)
            goto errstate;
    }
    x->x_noutlets = noutlets;
    x->x_maxcons = 0;
    while(noutlets--)
        outlet_new((t_object *)x, &s_anything);
    if(rightout)
        x->x_rightout = outlet_new((t_object *)x, &s_anything);
    return(x);
    errstate:
    pd_error(x, "[grab]: improper creation arguments");
    return(NULL);
}

static void grab_free(t_grab *x){
    if(x->x_grabbed)
        freebytes(x->x_grabbed, x->x_maxcons * sizeof(*x->x_grabbed));
    if(x->x_ngrabout)
    	freebytes(x->x_ngrabout, x->x_maxcons * sizeof(*x->x_ngrabout));
    if(x->x_grabcons)
    	freebytes(x->x_grabcons, x->x_maxcons * x->x_noutlets * sizeof(*x->x_grabcons));
}

CYCLONE_OBJ_API void grab_setup(void){
    t_symbol *s = gensym("grab");
    grab_class = class_new(s, (t_newmethod)grab_new, (t_method)grab_free,
        sizeof(t_grab), 0, A_GIMME, 0);
    class_addfloat(grab_class, grab_float);
    class_addbang(grab_class, grab_bang);
    class_addsymbol(grab_class, grab_symbol);
    class_addpointer(grab_class, grab_pointer);
    class_addlist(grab_class, grab_list);
    class_addanything(grab_class, grab_anything);
    class_addmethod(grab_class, (t_method)grab_set, gensym("set"), A_SYMBOL, 0);
    if(!bindlist_class){
        t_class *c = grab_class;
        pd_bind(&grab_class, s);
        pd_bind(&c, s);
        if(!s->s_thing || !(bindlist_class = *s->s_thing)
	    || bindlist_class->c_name != gensym("bindlist"))
            pd_error(grab_class, "grab: failure to initialize remote grabbing feature");
	pd_unbind(&c, s);
	pd_unbind(&grab_class, s);
    }
}
