/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*2016 - making COLLTHREAD,COLLEMBED, coll_embed, attribute parsing,
redundant insert2, edit to not autosort in collcommon_tonumkey,
adding coll_renumber2 and collcommon_renumber2
before used to always bang on callback, now only bangs now due to new x->x_filebang
(which i'm not sure works with panel_open....)
- Derek Kwan
*/

#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "g_canvas.h"
#include "common/file.h"

#include <pthread.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* LATER profile for the bottlenecks of insertion and sorting */
/* LATER make sure that ``reentrancy protection hack'' is really working... */

/* #ifdef KRZYSZCZ
//#define COLL_DEBUG
#endif */


#define COLLTHREAD 1 //default is threaded
#define COLLEMBED 0 //default for save in patch
#define COLL_ALLBANG 1 //bang all when read instead of specific object
#define COLL_MAXEXTLEN 4 //maximum file extension length

enum{COLL_HEADRESET, COLL_HEADNEXT, COLL_HEADPREV,  // distinction not used, currently
    COLL_HEADDELETED };

typedef struct _collelem{
    int                e_hasnumkey;
    int                e_numkey;
    t_symbol          *e_symkey;
    struct _collelem  *e_prev;
    struct _collelem  *e_next;
    int                e_size;
    t_atom            *e_data;
}t_collelem;

typedef struct _collcommon{
    t_pd           c_pd;
    struct _coll  *c_refs;      /* used in read-banging and dirty flag handling */
    int            c_increation;
    int            c_volatile;
    int            c_selfmodified;
    int            c_fileoninit; //if successfully loading a file on init
    int            c_entered;    /* a counter, LATER rethink */
    int            c_embedflag;  /* common field (CHECKED in 'TEXT' files) */
    t_symbol      *c_filename;   /* CHECKED common for all, read and write */
    t_canvas      *c_lastcanvas;
    t_file  *c_filehandle;
    t_collelem    *c_first;
    t_collelem    *c_last;
    t_collelem    *c_head;
    int            c_headstate;
}t_collcommon;

typedef struct _coll_q{    		/* element in a linked list of stored messages waiting to be sent out */
    struct _coll_q *q_next; 	/* next in list */
    char *q_s;            		/* the string */
}t_coll_q;

typedef struct _coll{
  t_object       x_ob;
  t_canvas      *x_canvas;
  t_symbol      *x_name;
  t_collcommon  *x_common;
  t_file  *x_filehandle;
  t_outlet      *x_keyout;
  t_outlet      *x_filebangout;
  t_outlet      *x_dumpbangout;
  t_symbol      *x_bindsym;
  int           x_is_opened;
  int           x_threaded;
  int           x_nosearch;
  int           x_initread; //if we're reading a file for the first time
  int           x_filebang; //if we're expecting to bang out 3rd outlet
  struct _coll  *x_next;

// for thread-unsafe file i/o operations added by Ivica Ico Bukvic <ico@vt.edu> 9-24-2010
// http://disis.music.vt.edu http://l2ork.music.vt.edu
  t_clock *x_clock;
  pthread_t unsafe_t;
  pthread_mutex_t unsafe_mutex;
  pthread_cond_t unsafe_cond;
  t_symbol *x_s;
  t_symbol      *x_fileext; 
  t_int unsafe;
  t_int init; //used to make sure that the secondary thread is ready to go
  t_int threaded; //used to decide whether this should be a threaded instance
  t_coll_q *x_q; //a list of error messages to be processed
}t_coll;

typedef struct _msg{
	int m_flag;
	int m_line;
}t_msg;

typedef struct _threadedFunctionParams{
	t_coll *x;
}t_threadedFunctionParams;

static t_class *coll_class;
static t_class *collcommon_class;

// if fileext is set, append it
static t_symbol * coll_fullfilename(t_coll *x, t_symbol * filename){
    char buf[MAXPDSTRING];
    if(x->x_fileext != &s_){
        strncpy(buf, filename->s_name, MAXPDSTRING - COLL_MAXEXTLEN - 1); //need space for period
        strcat(buf, ".");
        strcat(buf, x->x_fileext->s_name);
        return (gensym(buf));
    }
    else
        return (filename);
}

/// Porres: de-louding in a lazy way
void coll_messarg(t_pd *x, t_symbol *s){
    pd_error(x, "[coll]: bad arguments for message \"%s\"", s->s_name);
}

int coll_checkint(t_pd *x, t_float f, int *valuep, t_symbol *mess){
    if((*valuep = (int)f) == f)
        return (1);
    else{
        if(mess == &s_float)
            pd_error(x, "[coll]: doesn't understand \"noninteger float\"");
        else if(mess)
            pd_error(x, "[coll]: \"noninteger float\" argument invalid for message \"%s\"",
                     mess->s_name);
        return (0);
    }
}
///

static void coll_q_free(t_coll *x){
    t_coll_q *q2;
    while(x->x_q){
		q2 = x->x_q->q_next;
		t_freebytes(x->x_q->q_s, strlen(x->x_q->q_s) + 1);
		t_freebytes(x->x_q, sizeof(*x->x_q));
		x->x_q = q2;
    }
	x->x_q = NULL;
}

static void coll_q_post(t_coll_q *q){
    t_coll_q *qtmp;
    for(qtmp = q; qtmp; qtmp = qtmp->q_next)
		post("%s", qtmp->q_s);
}

static void coll_q_enqueue(t_coll *x, const char *s){
	t_coll_q *q, *q2 = NULL;
	q = (t_coll_q *)(getbytes(sizeof(*q)));
	q->q_next = NULL;
	q->q_s = (char *)getbytes(strlen(s) + 1);
	strcpy(q->q_s, s);
	if(!x->x_q)
		x->x_q = q;
	else{
		q2 = x->x_q;
		while(q2->q_next)
			q2 = q2->q_next;
		q2->q_next = q;
	}
}

static void coll_enqueue_threaded_msgs(t_coll *x, t_msg *m){
	char s[MAXPDSTRING];
	if(m->m_flag & 1)
		coll_q_enqueue(x, s);
	if(m->m_flag & 2){
        sprintf(s, "coll: can't find file '%s'", x->x_s->s_name);
        coll_q_enqueue(x, s);
	}
	if(m->m_flag & 4){
		//sprintf(s, "coll: finished reading %d lines from text file '%s'", m->m_line, x->x_s->s_name);
		//coll_q_enqueue(x, s);
	}
	if(m->m_flag & 8) {
		sprintf(s, "coll: error in line %d of text file '%s'", m->m_line, x->x_s->s_name);
		coll_q_enqueue(x, s);
	}
	if(m->m_flag & 16){
		sprintf(s, "coll: can't find file '%s'", x->x_s->s_name);
		coll_q_enqueue(x, s);
	}
	if(m->m_flag & 32) {
		sprintf(s, "coll: error writing text file '%s'", x->x_s->s_name);
		coll_q_enqueue(x, s);
	}
}

static void coll_tick(t_coll *x){
	if(x->x_q){
		coll_q_post(x->x_q);
		coll_q_free(x);
	};
    if(x->x_filebang && (!COLL_ALLBANG || x->x_initread)){
        x->x_initread = 0;
        outlet_bang(x->x_filebangout);
        x->x_filebang = 0;
    };
}

static t_collelem *collelem_new(int ac, t_atom *av, int *np, t_symbol *s){
    t_collelem *ep = (t_collelem *)getbytes(sizeof(*ep));
    if((ep->e_hasnumkey = (np != 0)))
        ep->e_numkey = *np;
    ep->e_symkey = s;
    ep->e_prev = ep->e_next = 0;
    if((ep->e_size = ac)){
        t_atom *ap = getbytes(ac * sizeof(*ap));
        ep->e_data = ap;
        if(av) while(ac--)
            *ap++ = *av++;
        else while(ac--){
            SETFLOAT(ap, 0);
            ap++;
        }
    }
    else
        ep->e_data = 0;
    return (ep);
}

static void collelem_free(t_collelem *ep){
    if(ep->e_data)
        freebytes(ep->e_data, ep->e_size * sizeof(*ep->e_data));
    freebytes(ep, sizeof(*ep));
}

/* CHECKME again... apparently c74 is not able to fix this for good */
/* result: 1 for ep1 < ep2, 0 for ep1 >= ep2, all symbols are < any float */
static int collelem_less(t_collelem *ep1, t_collelem *ep2, int ndx, int swap){
    int isless;
    if(swap){
        t_collelem *ep = ep1;
        ep1 = ep2;
        ep2 = ep;
    }
    if(ndx < 0){
        const char *st1 = ep1->e_symkey->s_name;
        const char *st2 = ep2->e_symkey->s_name;
        if(ep1->e_symkey)  // CHECKED incompatible with 4.07, but consistent
            isless = (ep2->e_symkey ? strcmp(st1, st2) < 0 : 1);
        else if(ep2->e_symkey)
            isless = 0;  // CHECKED incompatible with 4.07, but consistent
        else
            isless = (ep1->e_numkey < ep2->e_numkey);  // CHECKED in 4.07
    }
    else{
        t_atom *ap1 = (ndx < ep1->e_size ?
            ep1->e_data + ndx : ep1->e_data + ep1->e_size - 1);
        t_atom *ap2 = (ndx < ep2->e_size ?
            ep2->e_data + ndx : ep2->e_data + ep2->e_size - 1);
        if(ap1->a_type == A_FLOAT){
            if(ap2->a_type == A_FLOAT)
                isless = (ap1->a_w.w_float < ap2->a_w.w_float);
            else if(ap2->a_type == A_SYMBOL)
                isless = 0;
            else
                isless = 1;
        }
        else if(ap1->a_type == A_SYMBOL){
            if(ap2->a_type == A_FLOAT)
                isless = 1;
            else if(ap2->a_type == A_SYMBOL)
                isless = (strcmp(ap1->a_w.w_symbol->s_name, ap2->a_w.w_symbol->s_name) < 0);
            else
                isless = 1;
        }
        else
            isless = 0;
    }
    return(isless);
}

static t_collelem *collcommon_numkey(t_collcommon *cc, int numkey){
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next)
	if(ep->e_hasnumkey && ep->e_numkey == numkey)
	    return(ep);
    return(0);
}

static t_collelem *collcommon_symkey(t_collcommon *cc, t_symbol *symkey){
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next)
	if(ep->e_symkey == symkey)
        return(ep);
    return(0);
}

static void collcommon_takeout(t_collcommon *cc, t_collelem *ep){
    if(ep->e_prev)
        ep->e_prev->e_next = ep->e_next;
    else
        cc->c_first = ep->e_next;
    if(ep->e_next)
        ep->e_next->e_prev = ep->e_prev;
    else
        cc->c_last = ep->e_prev;
    if(cc->c_head == ep){
        cc->c_head = ep->e_next;  /* asymmetric, LATER rethink */
        cc->c_headstate = COLL_HEADDELETED;
    }
}

static void collcommon_modified(t_collcommon *cc, int relinked){
    if(cc->c_increation)
        return;
    if(relinked)
        cc->c_volatile = 1;
    if(cc->c_embedflag){
        t_coll *x;
        for(x = cc->c_refs; x; x = x->x_next)
            if(x->x_canvas && glist_isvisible(x->x_canvas))
                canvas_dirty(x->x_canvas, 1);
    }
}

/* atomic collcommon modifiers:  clearall, remove, replace,
   putbefore, putafter, swaplinks, swapkeys, changesymkey, renumber, sort */

static void collcommon_clearall(t_collcommon *cc){
    if(cc->c_first){
        t_collelem *ep1 = cc->c_first, *ep2;
        do{
            ep2 = ep1->e_next;
            collelem_free(ep1);
        }
        while((ep1 = ep2));
            cc->c_first = cc->c_last = 0;
        cc->c_head = 0;
        cc->c_headstate = COLL_HEADRESET;
        collcommon_modified(cc, 1);
    }
}

static void collcommon_remove(t_collcommon *cc, t_collelem *ep){
    collcommon_takeout(cc, ep);
    collelem_free(ep);
    collcommon_modified(cc, 1);
}

static void collcommon_replace(t_collcommon *cc, t_collelem *ep, int ac, t_atom *av, int *np, t_symbol *s){
    if((ep->e_hasnumkey = (np != 0)))
	ep->e_numkey = *np;
    ep->e_symkey = s;
    if(ac){
        int i = ac;
        t_atom *ap;
        if(ep->e_data){
            if(ep->e_size != ac)
                ap = resizebytes(ep->e_data, ep->e_size * sizeof(*ap), ac * sizeof(*ap));
            else ap = ep->e_data;
        }
        else
            ap = getbytes(ac * sizeof(*ap));
        ep->e_data = ap;
        if(av) while(i --)
            *ap++ = *av++;
        else while(i --){
            SETFLOAT(ap, 0);
            ap++;
        }
    }
    else{
        if(ep->e_data)
            freebytes(ep->e_data, ep->e_size * sizeof(*ep->e_data));
        ep->e_data = 0;
    }
    ep->e_size = ac;
    collcommon_modified(cc, 0);
}

static void collcommon_putbefore(t_collcommon *cc, t_collelem *ep, t_collelem *next){
    if(next){
        ep->e_next = next;
        if((ep->e_prev = next->e_prev))
            ep->e_prev->e_next = ep;
        else
            cc->c_first = ep;
        next->e_prev = ep;
    }
    else if(cc->c_first || cc->c_last)
        bug("collcommon_putbefore");
    else
        cc->c_first = cc->c_last = ep;
    collcommon_modified(cc, 1);
}

static void collcommon_putafter(t_collcommon *cc, t_collelem *ep, t_collelem *prev){
    if(prev){
        ep->e_prev = prev;
        if((ep->e_next = prev->e_next))
            ep->e_next->e_prev = ep;
        else
            cc->c_last = ep;
        prev->e_next = ep;
    }
    else if(cc->c_first || cc->c_last)
        bug("collcommon_putafter");
    else
        cc->c_first = cc->c_last = ep;
    collcommon_modified(cc, 1);
}

/* LATER consider making it faster, if there is a real need.
   Now called only in the sort routine, once per sort. */
static void collcommon_swaplinks(t_collcommon *cc, t_collelem *ep1, t_collelem *ep2){
    if(ep1 != ep2){
        t_collelem *prev1 = ep1->e_prev, *prev2 = ep2->e_prev;
        if(prev1 == ep2){
            collcommon_takeout(cc, ep2);
            collcommon_putafter(cc, ep2, ep1);
        }
        else if(prev2 == ep1){
            collcommon_takeout(cc, ep1);
            collcommon_putafter(cc, ep1, ep2);
        }
        else if(prev1){
            if(prev2){
                collcommon_takeout(cc, ep1);
                collcommon_takeout(cc, ep2);
                collcommon_putafter(cc, ep1, prev2);
                collcommon_putafter(cc, ep2, prev1);
            }
            else{
                t_collelem *next2 = ep2->e_next;
                collcommon_takeout(cc, ep1);
                collcommon_takeout(cc, ep2);
                collcommon_putbefore(cc, ep1, next2);
                collcommon_putafter(cc, ep2, prev1);
            }
        }
        else if(prev2){
            t_collelem *next1 = ep1->e_next;
            collcommon_takeout(cc, ep1);
            collcommon_takeout(cc, ep2);
            collcommon_putafter(cc, ep1, prev2);
            collcommon_putbefore(cc, ep2, next1);
        }
        else
            bug("collcommon_swaplinks");
    }
}

static void collcommon_swapkeys(t_collcommon *cc, t_collelem *ep1, t_collelem *ep2){
    int hasnumkey = ep2->e_hasnumkey, numkey = ep2->e_numkey;
    t_symbol *symkey = ep2->e_symkey;
    ep2->e_hasnumkey = ep1->e_hasnumkey;
    ep2->e_numkey = ep1->e_numkey;
    ep2->e_symkey = ep1->e_symkey;
    ep1->e_hasnumkey = hasnumkey;
    ep1->e_numkey = numkey;
    ep1->e_symkey = symkey;
    collcommon_modified(cc, 0);
}

static void collcommon_changesymkey(t_collcommon *cc, t_collelem *ep, t_symbol *s){
    ep->e_symkey = s;
    collcommon_modified(cc, 0);
}

static void collcommon_renumber2(t_collcommon *cc, int startkey){
 //entries with keys >= startkey get incremented by one	    
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next){
        if(ep->e_hasnumkey){
            if(ep->e_numkey >= startkey){
                ep->e_numkey = ep->e_numkey + 1;
            };
        };
    };
    //i have no idea what this does but renumber does it so i'm doing it too - DK
    collcommon_modified(cc, 0);
}

static void collcommon_renumber(t_collcommon *cc, int startkey){
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next)
        if(ep->e_hasnumkey)
            ep->e_numkey = startkey++;
    collcommon_modified(cc, 0);
}

/* LATER choose a better algo, after coll's storage structures stabilize.
   Note, that even the simple insertion sort below (n-square) might prove better
   for bi-directional lists, than theoretically efficient algo (nlogn) requiring
   random access emulation.  Avoiding recursion is not a bad idea, too. */
static void collcommon_sort(t_collcommon *cc, int descending, int ndx){
    t_collelem *min = cc->c_first;
    t_collelem *ep;
    if(min && (ep = min->e_next)){
        cc->c_increation = 1;
        // search for a sentinel element
        do if(collelem_less(ep, min, ndx, descending))
            min = ep;
        while((ep = ep->e_next));
        // prepend it
            collcommon_swaplinks(cc, cc->c_first, min);
        // sort
        ep = min->e_next->e_next;
        while(ep){
            t_collelem *next = ep->e_next;
            for(min = ep->e_prev; min && collelem_less(ep, min, ndx, descending);
                 min = min->e_prev);
            if(!min)  /* LATER remove */
                bug("collcommon_sort");
            else if(ep != min->e_next){
                collcommon_takeout(cc, ep);
                collcommon_putafter(cc, ep, min);
            }
            ep = next;
        }
        cc->c_increation = 0;
        collcommon_modified(cc, 1);
    }
}

static void collcommon_adddata(t_collcommon *cc, t_collelem *ep, int ac, t_atom *av){
    if(ac){
        t_atom *ap;
        int newsize = ep->e_size + ac;
        if(ep->e_data)
            ap = resizebytes(ep->e_data, ep->e_size * sizeof(*ap), newsize * sizeof(*ap));
        else{
            ep->e_size = 0;  /* redundant, hopefully */
            ap = getbytes(newsize * sizeof(*ap));
        }
        ep->e_data = ap;
        ap += ep->e_size;
        if(av) while(ac--)
            *ap++ = *av++;
        else while(ac--){
            SETFLOAT(ap, 0);
            ap++;
        }
        ep->e_size = newsize;
        collcommon_modified(cc, 0);
    }
}

/* 
   //old autosorting collcommon_tonumkey which INSERTS

static t_collelem *collcommon_tonumkey(t_collcommon *cc, int numkey,
				       int ac, t_atom *av, int replace)
{
    t_collelem *old = collcommon_numkey(cc, numkey), *new;
    if(old && replace)
	collcommon_replace(cc, new = old, ac, av, &numkey, 0);
    else
    {
	new = collelem_new(ac, av, &numkey, 0);
	if(old)
	{
	    collcommon_putbefore(cc, new, old);
	    do
		if(old->e_hasnumkey)
		    //CHECKED incremented up to the last one; incompatible:
		     //  elements with numkey == 0 not incremented (a bug?) 
		    old->e_numkey++;
	    while(old = old->e_next);
	}
	else
	{
	    // CHECKED negative numkey always put before the last element,
	     //  zero numkey always becomes the new head 
	    int closestkey = 0;
	    t_collelem *closest = 0, *ep;
	    for(ep = cc->c_first; ep; ep = ep->e_next)
	    {
		if(ep->e_hasnumkey)
		{
		    if(numkey >= closestkey && numkey <= ep->e_numkey)
		    {
			collcommon_putbefore(cc, new, ep);
			break;
		    }
		    closestkey = ep->e_numkey;
		}
		closest = ep;
	    }
	    if(!ep)
	    {
		if(numkey <= closestkey)
		    collcommon_putbefore(cc, new, closest);
		else
		    collcommon_putafter(cc, new, closest);
	    }
	}
    }
    return (new);
}
*/

static t_collelem *collcommon_tonumkey(t_collcommon *cc, int numkey, int ac, t_atom *av, int replace){
    //collcommon_numkey checks existence of key
    t_collelem *old = collcommon_numkey(cc, numkey), *new; 
    if(old && replace)
        collcommon_replace(cc, new = old, ac, av, &numkey, 0);
    else{
        new = collelem_new(ac, av, &numkey, 0);
        if(old){
            do
                if(old->e_hasnumkey)
                    //CHECKED incremented up to the last one; incompatible:
                    //  elements with numkey == 0 not incremented (a bug?)
                    old->e_numkey++;
                while((old = old->e_next));
        };
        // CHECKED negative numkey always put before the last element,
        //  zero numkey always becomes the new head
        t_collelem *closest = 0, *ep;
        for(ep = cc->c_first; ep; ep = ep->e_next)
            closest = ep;
        collcommon_putafter(cc, new, closest);
	}
    return(new);
}

static t_collelem *collcommon_tosymkey(t_collcommon *cc, t_symbol *symkey, int ac, t_atom *av, int replace){
    t_collelem *old = collcommon_symkey(cc, symkey), *new;
    if(old && replace)
        collcommon_replace(cc, new = old, ac, av, 0, symkey);
    else
        collcommon_putafter(cc, new = collelem_new(ac, av, 0, symkey), cc->c_last);
    return (new);
}

static int collcommon_fromatoms(t_collcommon *cc, int ac, t_atom *av){
    int hasnumkey = 0, numkey;
    t_symbol *symkey = 0;
    int size = 0;
    t_atom *data = 0;
    int nlines = 0;
    cc->c_increation = 1;
    collcommon_clearall(cc);
    while(ac--){
        if(data){
            if(av->a_type == A_SEMI){
                t_collelem *ep = collelem_new(size, data, hasnumkey ? &numkey : 0, symkey);
                collcommon_putafter(cc, ep, cc->c_last);
                hasnumkey = 0;
                symkey = 0;
                data = 0;
                nlines++;
            }
            if(av->a_type == A_COMMA){
                /* CHECKED rejecting a comma */
                collcommon_clearall(cc);  /* LATER rethink */
                cc->c_increation = 0;
                return (-nlines);
            }
            else
                size++;
        }
        else if(av->a_type == A_COMMA){
            size = 0;
            data = av + 1;
        }
        else if(av->a_type == A_SYMBOL)
            symkey = av->a_w.w_symbol;
        else if(av->a_type == A_FLOAT &&
            coll_checkint(0, av->a_w.w_float, &numkey, 0))
	    hasnumkey = 1;
        else{
            post("coll: bad atom");
            collcommon_clearall(cc);  /* LATER rethink */
            cc->c_increation = 0;
            return (-nlines);
        }
        av++;
    }
    if(data){
        post("coll: incomplete");
        collcommon_clearall(cc);  /* LATER rethink */
        cc->c_increation = 0;
        return (-nlines);
    }
    cc->c_increation = 0;
    return (nlines);
}

static int collcommon_frombinbuf(t_collcommon *cc, t_binbuf *bb){
    return (collcommon_fromatoms(cc, binbuf_getnatom(bb), binbuf_getvec(bb)));
}

static t_msg *collcommon_doread(t_collcommon *cc, t_symbol *fn, t_canvas *cv, int threaded){
    t_binbuf *bb;
	t_msg *m = (t_msg *)(getbytes(sizeof(*m)));
	m->m_flag = 0;
	m->m_line = 0;
    if(!fn && !(fn = cc->c_filename))  // !fn: 'readagain'
        return(m);
    char buf[MAXPDSTRING];
    /* FIXME use open_via_path()
    if(cv || (cv = cc->c_lastcanvas))  // !cv: 'read' w/o arg, 'readagain'
		canvas_makefilename(cv, fn->s_name, buf, MAXPDSTRING);
    else{
    	strncpy(buf, fn->s_name, MAXPDSTRING);
    	buf[MAXPDSTRING-1] = 0;
    }*/
    char *bufptr;
    int fd = canvas_open(cv, fn->s_name, "", buf, &bufptr, MAXPDSTRING, 1);
    if(fd > 0){
        buf[strlen(buf)]='/';
        sys_close(fd);
    }
    else{
        post("[coll] file '%s' not found", fn->s_name);
        return(m);
    }
    if(!cc->c_refs){
		/* loading during object creation --
		   avoid binbuf_read()'s complaints, LATER rethink */
		FILE *fp;
		char fname[MAXPDSTRING];
		sys_bashfilename(buf, fname);
		if(!(fp = fopen(fname, "r"))){
			m->m_flag |= 0x01;
			return(m);
		};
		fclose(fp);
    }
    bb = binbuf_new();
    if(binbuf_read(bb, buf, "", 0)){
		m->m_flag |= 0x02;
		if(!threaded)
			post("coll: can't find file '%s'", fn->s_name);
	}
    else if(!binbuf_read(bb, buf, "", 0)){
		int nlines = collcommon_frombinbuf(cc, bb);
		if(nlines > 0){
			t_coll *x;
			/* LATER consider making this more robust
             //now taken care of by coll_read for obj specificity
            //leaving here so i remember how to do this o/wise - DK */
            if(COLL_ALLBANG){
                for(x = cc->c_refs; x; x = x->x_next){
                    outlet_bang(x->x_filebangout);
                };
            };
			cc->c_lastcanvas = cv;
			cc->c_filename = fn;
			m->m_flag |= 0x04;
			m->m_line = nlines;
			/*
			if(!threaded)
				post("coll: finished reading %d lines from text file '%s'",
					nlines, fn->s_name);
			*/
		}
		else if(nlines < 0){
			m->m_flag |= 0x08;
			m->m_line = 1 - nlines;
			if(!threaded)
			    post("coll: error in line %d of text file '%s'", 1 - nlines, fn->s_name);
		}
		else{
			m->m_flag |= 0x16;
			if(!threaded)
				post("coll: can't find file '%s'", fn->s_name);
		}
		if(cc->c_refs)
			collcommon_modified(cc, 1);
	}
    binbuf_free(bb);
	return(m);
}

static void collcommon_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    collcommon_doread((t_collcommon *)z, fn, ((t_coll *)z)->x_canvas, 0);
}

static void collcommon_tobinbuf(t_collcommon *cc, t_binbuf *bb){
    t_collelem *ep;
    t_atom at[3];
    for(ep = cc->c_first; ep; ep = ep->e_next){
        t_atom *ap = at;
        int cnt = 1;
        if(ep->e_hasnumkey){
            SETFLOAT(ap, ep->e_numkey);
            ap++; cnt++;
        }
        if(ep->e_symkey){
            SETSYMBOL(ap, ep->e_symkey);
            ap++; cnt++;
        }
        SETCOMMA(ap);
        binbuf_add(bb, cnt, at);
        binbuf_add(bb, ep->e_size, ep->e_data);
        binbuf_addsemi(bb);
    }
}

static t_msg *collcommon_dowrite(t_collcommon *cc, t_symbol *fn, t_canvas *cv, int threaded){
    t_binbuf *bb;
	t_msg *m = (t_msg *)(getbytes(sizeof(*m)));
	m->m_flag = 0;
	m->m_line = 0;
    char buf[MAXPDSTRING];
    if(!fn && !(fn = cc->c_filename))  /* !fn: 'writeagain' */
		return(0);
    if(cv || (cv = cc->c_lastcanvas))  /* !cv: 'write' w/o arg, 'writeagain' */
        canvas_makefilename(cv, fn->s_name, buf, MAXPDSTRING);
    else{
    	strncpy(buf, fn->s_name, MAXPDSTRING);
    	buf[MAXPDSTRING-1] = 0;
    }
    bb = binbuf_new();
    collcommon_tobinbuf(cc, bb);
    if(binbuf_write(bb, buf, "", 0)) {
		m->m_flag |= 0x32;
		if(!threaded)
			post("coll: error writing text file '%s'", fn->s_name);
	}
    else if(!binbuf_write(bb, buf, "", 0)){
        cc->c_lastcanvas = cv;
        cc->c_filename = fn;
    }
    binbuf_free(bb);
	return(m);
}

static void collcommon_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0, av = NULL;
    collcommon_dowrite((t_collcommon *)z, fn, 0, 0);
}

static void coll_embedhook(t_pd *z, t_binbuf *bb, t_symbol *bindsym){
    t_coll *x = (t_coll *)z;
    t_collcommon *cc = x->x_common;
    if(cc->c_embedflag){
        t_collelem *ep;
        t_atom at[6];
        binbuf_addv(bb, "ssii;", bindsym, gensym("flags"), 1, 0);
        SETSYMBOL(at, bindsym);
        for(ep = cc->c_first; ep; ep = ep->e_next){
            t_atom *ap = at + 1;
            int cnt;
            if(ep->e_hasnumkey && ep->e_symkey){
                SETSYMBOL(ap, gensym("nstore"));
                ap++;
                SETSYMBOL(ap, ep->e_symkey);
                ap++;
                SETFLOAT(ap, ep->e_numkey);
                cnt = 4;
            }
            else if(ep->e_symkey){
                SETSYMBOL(ap, gensym("store"));
                ap++;
                SETSYMBOL(ap, ep->e_symkey);
                cnt = 3;
            }
            else{
                SETFLOAT(ap, ep->e_numkey);
                cnt = 2;
            }
            binbuf_add(bb, cnt, at);
            binbuf_add(bb, ep->e_size, ep->e_data);
            binbuf_addsemi(bb);
        };
    };
    obj_saveformat((t_object *)x, bb);
}

static void collcommon_editorhook(t_pd *z, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int nlines = collcommon_fromatoms((t_collcommon *)z, ac, av);
    if(nlines < 0)
        post("coll: editing error in line %d", 1 - nlines);
}

static void collcommon_free(t_collcommon *cc){
    t_collelem *ep1, *ep2 = cc->c_first;
    while((ep1 = ep2)){
        ep2 = ep1->e_next;
        collelem_free(ep1);
    }
}

static void *collcommon_new(void){
    t_collcommon *cc = (t_collcommon *)pd_new(collcommon_class);
    cc->c_embedflag = 0;
    cc->c_first = cc->c_last = 0;
    cc->c_head = 0;
    cc->c_headstate = COLL_HEADRESET;
    cc->c_fileoninit = 0; //loaded file on init, change when successful loading
    return (cc);
}

/*static t_collcommon *coll_checkcommon(t_coll *x)
{ // unused
    if(x->x_name &&
	x->x_common != (t_collcommon *)pd_findbyclass(x->x_name,
						      collcommon_class))
    {
	bug("coll_checkcommon");
	return (0);
    }
    return (x->x_common);
}*/

static void coll_unbind(t_coll *x){
    /* LATER consider calling coll_checkcommon(x) */
    t_collcommon *cc = x->x_common;
    t_coll *prev, *next;
    if((prev = cc->c_refs) == x){
        if(!(cc->c_refs = x->x_next)){
            file_free(cc->c_filehandle);
            collcommon_free(cc);
            if(x->x_name) pd_unbind(&cc->c_pd, x->x_name);
                pd_free(&cc->c_pd);
        }
    }
    else if(prev){
        while((next = prev->x_next)){
            if(next == x){
                prev->x_next = next->x_next;
                break;
            }
	    prev = next;
        }
    }
    x->x_common = 0;
    x->x_name = 0;
    x->x_next = 0;
}

static void coll_bind(t_coll *x, t_symbol *name){
    int no_search = x->x_nosearch;
    t_collcommon *cc = 0;
    if(name == &s_)
        name = 0;
    else if(name)
        cc = (t_collcommon *)pd_findbyclass(name, collcommon_class);
    if(!cc){
        cc = (t_collcommon *)collcommon_new();
        cc->c_refs = 0;
        cc->c_increation = 0;
        //x->x_common = cc;
        //x->x_s = name;
        if(name){
            pd_bind(&cc->c_pd, name);
            /* LATER rethink canvas unpredictability */
            //x->unsafe = 1;
            //pthread_mutex_lock(&x->unsafe_mutex);
            //pthread_cond_signal(&x->unsafe_cond);
            //pthread_mutex_unlock(&x->unsafe_mutex);
            if(!no_search){
                t_msg * msg = collcommon_doread(cc, name, x->x_canvas, 0);
                if(msg->m_line > 0){
                    //bang if file read successful
                    //need to use clock bc x not returned yet - DK
                    cc->c_fileoninit = 1;
                    x->x_filebang = 1;
                    x->x_initread = 1;
                    clock_delay(x->x_clock, 0);
                };
            };
        }
        else{
            cc->c_filename = 0;
            cc->c_lastcanvas = 0;
        }
        cc->c_filehandle = file_new((t_pd *)cc, 0, collcommon_readhook,
            collcommon_writehook, collcommon_editorhook);
    }
    else{
        //bang if you find collcommon existing already,
        //but shouldn't be for no search
        if(!no_search && cc->c_filename != 0 && cc->c_fileoninit){
            x->x_filebang = 1;
            x->x_initread = 1;
            clock_delay(x->x_clock, 0);
        };
    };
    x->x_common = cc;
    x->x_name = name;
    x->x_next = cc->c_refs;
    cc->c_refs = x;
}

static int coll_rebind(t_coll *x, t_symbol *name){
    t_collcommon *cc;
    if(name && name != &s_ && (cc = (t_collcommon *)pd_findbyclass(name, collcommon_class))){
        coll_unbind(x);
        x->x_common = cc;
        x->x_name = name;
        x->x_next = cc->c_refs;
        cc->c_refs = x;
        return(1);
    }
    else
        return(0);
}

static void coll_dooutput(t_coll *x, int ac, t_atom *av){
    if(ac > 1){
        if(av->a_type == A_FLOAT)
            outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
        else if(av->a_type == A_SYMBOL)
            outlet_anything(((t_object *)x)->ob_outlet, av->a_w.w_symbol, ac-1, av+1);
    }
    else if(ac){
        if(av->a_type == A_FLOAT)
            outlet_float(((t_object *)x)->ob_outlet, av->a_w.w_float);
        else if(av->a_type == A_SYMBOL)
            outlet_symbol(((t_object *)x)->ob_outlet, av->a_w.w_symbol);
    }
}

static void coll_keyoutput(t_coll *x, t_collelem *ep){
    t_collcommon *cc = x->x_common;
    if(!cc->c_entered++) cc->c_selfmodified = 0;
        cc->c_volatile = 0;
    if(ep->e_hasnumkey)
        outlet_float(x->x_keyout, ep->e_numkey);
    else if(ep->e_symkey)
        outlet_symbol(x->x_keyout, ep->e_symkey);
    else
        outlet_float(x->x_keyout, 0);
    if(cc->c_volatile) cc->c_selfmodified = 1;
        cc->c_entered--;
}

static t_collelem *coll_findkey(t_coll *x, t_atom *key, t_symbol *mess){
    t_collcommon *cc = x->x_common;
    t_collelem *ep = 0;
    if(key->a_type == A_FLOAT){
        int numkey;
        if(coll_checkint((t_pd *)x, key->a_w.w_float, &numkey, mess))
            ep = collcommon_numkey(cc, numkey);
        else
            mess = 0;
    }
    else if(key->a_type == A_SYMBOL)
        ep = collcommon_symkey(cc, key->a_w.w_symbol);
    else if(mess){
        coll_messarg((t_pd *)x, mess);
        mess = 0;
    }
    if(!ep && mess)
        pd_error((t_pd *)x, "no such key");
    return(ep);
}

static t_collelem *coll_tokey(t_coll *x, t_atom *key, int ac, t_atom *av, int replace, t_symbol *mess){
    t_collcommon *cc = x->x_common;
    t_collelem *ep = 0;
    if(key->a_type == A_FLOAT){
        int numkey;
        if(coll_checkint((t_pd *)x, key->a_w.w_float, &numkey, mess))
            ep = collcommon_tonumkey(cc, numkey, ac, av, replace);
    }
    else if(key->a_type == A_SYMBOL)
        ep = collcommon_tosymkey(cc, key->a_w.w_symbol, ac, av, replace);
    else if(mess)
        coll_messarg((t_pd *)x, mess);
    return(ep);
}

static t_collelem *coll_firsttyped(t_coll *x, int ndx, t_atomtype type){
    t_collcommon *cc = x->x_common;
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next)
        if(ep->e_size > ndx && ep->e_data[ndx].a_type == type)
            return (ep);
    return (0);
}

static void coll_do_update(t_coll *x){
    if(x->x_is_opened){
        t_collcommon *cc = x->x_common;
        t_binbuf *bb = binbuf_new();
        int natoms, newline;
        t_atom *ap;
        char buf[MAXPDSTRING];
        collcommon_tobinbuf(cc, bb);
        natoms = binbuf_getnatom(bb);
        ap = binbuf_getvec(bb);
        newline = 1;
        sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)cc->c_filehandle);
        sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)cc->c_filehandle);
        sys_gui(" }\n");
        while(natoms--){
            char *ptr = buf;
            if(ap->a_type != A_SEMI && ap->a_type != A_COMMA && !newline)
                *ptr++ = ' ';
            atom_string(ap, ptr, MAXPDSTRING);
            if(ap->a_type == A_SEMI){
                strcat(buf, "\n");
                newline = 1;
            }
            else
                newline = 0;
            editor_append(cc->c_filehandle, buf);
            ap++;
        }
        editor_setdirty(cc->c_filehandle, 0);
        binbuf_free(bb);
    }
}

static void coll_do_open(t_coll *x){
    t_collcommon *cc = x->x_common;
    if(x->x_is_opened){
        unsigned long wname = (unsigned long)cc->c_filehandle;
        sys_vgui("wm deiconify .%lx\n", wname);
        sys_vgui("raise .%lx\n", wname);
        sys_vgui("focus .%lx.text\n", wname);
    }
    else{
        char buf[MAXPDSTRING];
        char *name = (char *)(x->x_name ? x->x_name->s_name : "Untitled");
        editor_open(cc->c_filehandle, name, "coll");
        t_binbuf *bb = binbuf_new();
        collcommon_tobinbuf(cc, bb);
        int natoms = binbuf_getnatom(bb);
        t_atom *ap = binbuf_getvec(bb);
        int newline = 1;
        while(natoms--){
            char *ptr = buf;
            if(ap->a_type != A_SEMI && ap->a_type != A_COMMA && !newline)
                *ptr++ = ' ';
            atom_string(ap, ptr, MAXPDSTRING);
            if(ap->a_type == A_SEMI){
                strcat(buf, "\n");
                newline = 1;
            }
            else
                newline = 0;
            editor_append(cc->c_filehandle, buf);
            ap++;
        }
        editor_setdirty(cc->c_filehandle, 0);
        binbuf_free(bb);
        x->x_is_opened = 1;
    }
}

static void coll_is_opened(t_coll *x, t_floatarg f, t_floatarg open){
    x->x_is_opened = (int)f;
    open ? coll_do_open(x) : coll_do_update(x);
}

static void check_open(t_coll *x, int open){
    sys_vgui("if {[winfo exists .%lx]} {\n", (unsigned long)x->x_common->c_filehandle);
    sys_vgui("pdsend \"%s _is_opened 1 %d\"\n", x->x_bindsym->s_name, open);
    sys_vgui("} else {pdsend \"%s _is_opened 0 %d\"\n", x->x_bindsym->s_name, open);
    sys_gui(" }\n");
}

static void coll_update(t_coll *x){
    check_open(x, 0);
}

// methods -------------------------------------------------------------------------------------
static void coll_wclose(t_coll *x){ // if edited, closing window asks and replace the contents
    editor_close(x->x_common->c_filehandle, 1);
}

static void coll_open(t_coll *x){
    check_open(x, 1);
}

static void coll_click(t_coll *x, t_floatarg xpos, t_floatarg ypos,
t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    xpos = ypos = shift = ctrl = alt = 0;
    check_open(x, 1);
}

static void coll_float(t_coll *x, t_float f){
    t_collcommon *cc = x->x_common;
    t_collelem *ep;
    int numkey;
    if(coll_checkint((t_pd *)x, f, &numkey, &s_float) && (ep = collcommon_numkey(cc, numkey))){
		coll_keyoutput(x, ep);
		if(!cc->c_selfmodified || (ep = collcommon_numkey(cc, numkey)))
			coll_dooutput(x, ep->e_size, ep->e_data);
    }
}

static void coll_symbol(t_coll *x, t_symbol *s){
    t_collcommon *cc = x->x_common;
    t_collelem *ep;
    if((ep = collcommon_symkey(cc, s))){
		coll_keyoutput(x, ep);
		if(!cc->c_selfmodified || (ep = collcommon_symkey(cc, s)))
			coll_dooutput(x, ep->e_size, ep->e_data);
    }
}

static void coll_list(t_coll *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac >= 2 && av->a_type == A_FLOAT){
        coll_tokey(x, av, ac-1, av+1, 1, &s_list);
        coll_update(x);
    }
    else
		coll_messarg((t_pd *)x, &s_list);
}

static void coll_anything(t_coll *x, t_symbol *s, int ac, t_atom *av){
    ac = 0, av = NULL;
    coll_symbol(x, s);
}

static void coll_store(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac >= 2){
		coll_tokey(x, av, ac-1, av+1, 1, s);
        coll_update(x);
    }
    else
		pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_nstore(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac >= 3){
		t_collcommon *cc = x->x_common;
		t_collelem *ep;
		int numkey;
		if(av->a_type == A_FLOAT && av[1].a_type == A_SYMBOL){
			if(coll_checkint((t_pd *)x, av->a_w.w_float, &numkey, s)){
                if((ep = collcommon_symkey(cc, av[1].a_w.w_symbol)))
                    collcommon_remove(cc, ep);
                ep = collcommon_tonumkey(cc, numkey, ac-2, av+2, 1);
                ep->e_symkey = av[1].a_w.w_symbol;
			}
            coll_update(x);
		}
		else if(av->a_type == A_SYMBOL && av[1].a_type == A_FLOAT){
			if(coll_checkint((t_pd *)x, av[1].a_w.w_float, &numkey, s)){
                if((ep = collcommon_numkey(cc, numkey)))
                    collcommon_remove(cc, ep);
                ep = collcommon_tosymkey(cc, av->a_w.w_symbol, ac-2, av+2, 1);
                ep->e_hasnumkey = 1;
                ep->e_numkey = numkey;
			}
            coll_update(x);
		}
		else
            pd_error(x, "bad arguments for message '%s'", s->s_name);
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_insert2(t_coll *x, t_symbol *s, int ac, t_atom *av){
    t_collcommon  *cc = x->x_common;
    t_collelem *epnew = NULL;
    t_collelem *ep;
    int numkey;
    if(ac >= 2 && av->a_type == A_FLOAT){
        // get the int from the first elt if exists and store in numkey
        coll_checkint((t_pd *) x, av->a_w.w_float, &numkey, 0);
        // retrieve elem with numkey if it exists
        ep = collcommon_numkey(cc, numkey);
        if(ep){
            epnew = collelem_new(ac-1, av+1, &numkey, 0);
            // put new element before the one we just found
            collcommon_putbefore(cc, epnew, ep);
            // increment all keys which are >= the numkey - aside from the one we just made
            for(ep = cc->c_first; ep; ep = ep->e_next){
                if(ep->e_hasnumkey){
                    if((ep->e_numkey >= numkey) && (ep != epnew)){
                        ep->e_numkey = ep->e_numkey + 1;
                    };
                };
            };
            //it looks like you use this when you don't change data, just keys? -DK
            collcommon_modified(cc, 0);
        }
        else{
            coll_tokey(x, av, ac-1, av+1, 0, s);
            coll_update(x);
        }
    }
    else
		pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_insert(t_coll *x, t_symbol *s, int ac, t_atom *av){
    t_collcommon  *cc = x->x_common;
    t_collelem *epnew = NULL;
    t_collelem *ep;
    int numkey;
    if(ac >= 2 && av->a_type == A_FLOAT){
        // get the int from the first elt if exists and store in numkey
        coll_checkint((t_pd *) x, av->a_w.w_float, &numkey, 0);
        // retrieve elem with numkey if it exists
        ep = collcommon_numkey(cc, numkey);
        if(ep){
            epnew = collelem_new(ac-1, av+1, &numkey, 0);
            // put new element before the one we just found
            collcommon_putbefore(cc, epnew, ep);
        }
        else{ // we didn't find it, put the new elem last
            ep = cc->c_last;
            epnew = collelem_new(ac-1, av+1, &numkey, 0);
            collcommon_putafter(cc, epnew, ep);
        };
        // increment all keys which are >= the numkey - aside from the one we just made
        for(ep = cc->c_first; ep; ep = ep->e_next){
            if(ep->e_hasnumkey){
                if((ep->e_numkey >= numkey) && (ep != epnew)){
                    ep->e_numkey = ep->e_numkey + 1;
                };
            };
        };
        // it looks like you use this when you don't change data, just keys? -DK
        collcommon_modified(cc, 0);
        coll_update(x);
    }
    else
		pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_remove(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac){
		t_collelem *ep;
		if((ep = coll_findkey(x, av, s)))
			collcommon_remove(x->x_common, ep);
        coll_update(x);
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_delete(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac){
        t_collelem *ep;
        if((ep = coll_findkey(x, av, s))){
            if(av->a_type == A_FLOAT){
				int numkey = ep->e_numkey;
				t_collelem *next;
				for(next = ep->e_next; next; next = next->e_next)
					if(next->e_hasnumkey && next->e_numkey > numkey)
                        next->e_numkey--;
            }
            collcommon_remove(x->x_common, ep);
            coll_update(x);
        }
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_assoc(t_coll *x, t_symbol *s, t_floatarg f){
    int numkey;
    if(coll_checkint((t_pd *)x, f, &numkey, gensym("assoc"))){
        t_collcommon *cc = x->x_common;
        t_collelem *ep1, *ep2;
        if((ep1 = collcommon_numkey(cc, numkey)) && ep1->e_symkey != s){  // LATER rethink
            if((ep2 = collcommon_symkey(cc, s)))
                collcommon_remove(cc, ep2);
            collcommon_changesymkey(cc, ep1, s);
        }
        coll_update(x);
    }
}
		
static void coll_deassoc(t_coll *x, t_symbol *s, t_floatarg f){
    s = NULL;
    int numkey;
    if(coll_checkint((t_pd *)x, f, &numkey, gensym("deassoc"))){
        t_collcommon *cc = x->x_common;
        t_collelem *ep;
        if((ep = collcommon_numkey(cc, numkey)))
            collcommon_changesymkey(cc, ep, 0);
    }
    coll_update(x);
}

static void coll_subsym(t_coll *x, t_symbol *s1, t_symbol *s2){
    t_collelem *ep;
    if(s1 != s2 && (ep = collcommon_symkey(x->x_common, s2))){
		collcommon_changesymkey(x->x_common, ep, s1);
        coll_update(x);
    }
}

static void coll_renumber2(t_coll *x, t_floatarg f){
    int startkey;
    if(coll_checkint((t_pd *)x, f, &startkey, gensym("renumber"))){
		collcommon_renumber2(x->x_common, startkey);
        coll_update(x);
    }
}

static void coll_renumber(t_coll *x, t_floatarg f){
    int startkey;
    if(coll_checkint((t_pd *)x, f, &startkey, gensym("renumber"))){
		collcommon_renumber(x->x_common, startkey);
        coll_update(x);
    }
}

static void coll_merge(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac >= 2){
        t_collcommon *cc = x->x_common;
        t_collelem *ep;
        if(av->a_type == A_FLOAT){
            int numkey;
            if(coll_checkint((t_pd *)x, av->a_w.w_float, &numkey, s)){
				if((ep = collcommon_numkey(cc, numkey)))
					collcommon_adddata(cc, ep, ac-1, av+1);
				else  /* LATER consider defining collcommon_toclosest() */
					collcommon_tonumkey(cc, numkey, ac-1, av+1, 1);
            }
            coll_update(x);
        }
        else if(av->a_type == A_SYMBOL){
            if((ep = collcommon_symkey(cc, av->a_w.w_symbol)))
                collcommon_adddata(cc, ep, ac-1, av+1);
            else{
                ep = collelem_new(ac-1, av+1, 0, av->a_w.w_symbol);
                collcommon_putafter(cc, ep, cc->c_last);
            }
            coll_update(x);
        }
        else
            pd_error(x, "bad arguments for message '%s'", s->s_name);
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_sub(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac){
        t_collelem *ep;
        if((ep = coll_findkey(x, av, s))){
            t_collcommon *cc = x->x_common;
            t_atom *key = av++;
            ac--;
            while(ac >= 2){
                if(av->a_type == A_FLOAT){
                    int ndx;
                    if(coll_checkint((t_pd *)x, av->a_w.w_float, &ndx, 0)
                    && ndx >= 1 && ndx <= ep->e_size)
                    ep->e_data[ndx-1] = av[1];
                }
                ac -= 2;
                av += 2;
            }
            if(s == gensym("sub")){
				coll_keyoutput(x, ep);
				if(!cc->c_selfmodified || (ep = coll_findkey(x, key, 0)))
					coll_dooutput(x, ep->e_size, ep->e_data);
            }
        }
        coll_update(x);
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_sort(t_coll *x, t_floatarg f1, t_floatarg f2){
    int dir, ndx;
    if(coll_checkint((t_pd *)x, f1, &dir, gensym("sort")) &&
    coll_checkint((t_pd *)x, f2, &ndx, gensym("sort"))){
        collcommon_sort(x->x_common, (dir < 0 ? 0 : 1), (ndx < 0 ? -1 : (ndx ? ndx - 1 : 0)));
        coll_update(x);
    }
}

static void coll_clear(t_coll *x){
    t_collcommon *cc = x->x_common;
    collcommon_clearall(cc);
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)cc->c_filehandle);
    sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)cc->c_filehandle);
    sys_gui(" }\n");
}

/* According to the refman, the data should be swapped, rather than the keys
   -- easy here, but apparently c74 people have chosen to avoid some effort
   needed in case of their implementation... */
static void coll_swap(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac == 2){
		t_collelem *ep1, *ep2;
        if((ep1 = coll_findkey(x, av, s)) && (ep2 = coll_findkey(x, av + 1, s))){
			collcommon_swapkeys(x->x_common, ep1, ep2);
            coll_update(x);
        }
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

/* CHECKED traversal direction change is consistent with the general rule:
   'next' always outputs e_next of a previous output, and 'prev' always
   outputs e_prev, whether preceded by 'prev', or by 'next'.  This is
   currently implemented by pre-updating of the head (which is inhibited
   if there was no previous output, i.e. after 'goto', 'end', or collection
   initialization).  CHECKME again. */

static void coll_next(t_coll *x){
    t_collcommon *cc = x->x_common;
    if(cc->c_headstate != COLL_HEADRESET && cc->c_headstate != COLL_HEADDELETED){// asymmetric, LATER rethink
		if(cc->c_head)
			cc->c_head = cc->c_head->e_next;
		if(!cc->c_head && !(cc->c_head = cc->c_first))  /* CHECKED wrapping */
			return;
    }
    else if(!cc->c_head && !(cc->c_head = cc->c_first))
		return;
    cc->c_headstate = COLL_HEADNEXT;
    coll_keyoutput(x, cc->c_head);
    if(cc->c_head)
        coll_dooutput(x, cc->c_head->e_size, cc->c_head->e_data);
    else if(!cc->c_selfmodified)
		bug("coll_next");  /* LATER rethink */
}

static void coll_prev(t_coll *x){
    t_collcommon *cc = x->x_common;
    if(cc->c_headstate != COLL_HEADRESET){
        if(cc->c_head)
            cc->c_head = cc->c_head->e_prev;
        if(!cc->c_head && !(cc->c_head = cc->c_last))  /* CHECKED wrapping */
            return;
    }
    else if(!cc->c_head && !(cc->c_head = cc->c_first))
		return;
    cc->c_headstate = COLL_HEADPREV;
    coll_keyoutput(x, cc->c_head);
    if(cc->c_head)
		coll_dooutput(x, cc->c_head->e_size, cc->c_head->e_data);
    else if(!cc->c_selfmodified)
		bug("coll_prev");  /* LATER rethink */
}

static void coll_start(t_coll *x){
    t_collcommon *cc = x->x_common;
    cc->c_head = cc->c_first;
    cc->c_headstate = COLL_HEADRESET;
}

static void coll_end(t_coll *x){
    t_collcommon *cc = x->x_common;
    cc->c_head = cc->c_last;
    cc->c_headstate = COLL_HEADRESET;
}

static void coll_goto(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac){
        t_collelem *ep = coll_findkey(x, av, s);
        if(ep){
            t_collcommon *cc = x->x_common;
            cc->c_head = ep;
            cc->c_headstate = COLL_HEADRESET;
        }
    }
    else
        coll_start(x);
}

static void coll_nth(t_coll *x, t_symbol *s, int ac, t_atom *av){
    if(ac >= 2 && av[1].a_type == A_FLOAT){
        int ndx;
        t_collelem *ep;
        if(coll_checkint((t_pd *)x, av[1].a_w.w_float, &ndx, s) &&
        (ep = coll_findkey(x, av, s)) &&ep->e_size >= ndx){
            t_atom *ap = ep->e_data + --ndx;
            if(ap->a_type == A_FLOAT)
				outlet_float(((t_object *)x)->ob_outlet, ap->a_w.w_float);
            else if(ap->a_type == A_SYMBOL)
				outlet_symbol(((t_object *)x)->ob_outlet, ap->a_w.w_symbol);
        }
    }
    else
        pd_error(x, "bad arguments for message '%s'", s->s_name);
}

static void coll_length(t_coll *x){
    t_collcommon *cc = x->x_common;
    t_collelem *ep = cc->c_first;
    int result = 0;
    while(ep) result++, ep = ep->e_next;
    outlet_float(((t_object *)x)->ob_outlet, result);
}

static void coll_min(t_coll *x, t_floatarg f){
    int ndx;
    if(coll_checkint((t_pd *)x, f, &ndx, gensym("min"))){
        t_collelem *found;
        if(ndx > 0)
            ndx--;
        /* LATER consider complaining: */
        else if(ndx < 0)
            return;  /* CHECKED silently rejected */
        /* else CHECKED silently defaults to 1 */
        if((found = coll_firsttyped(x, ndx, A_FLOAT))){
            t_float result = found->e_data[ndx].a_w.w_float;
            t_collelem *ep;
            for(ep = found->e_next; ep; ep = ep->e_next){
				if(ep->e_size > ndx &&
                ep->e_data[ndx].a_type == A_FLOAT &&
                ep->e_data[ndx].a_w.w_float < result){
					found = ep;
					result = ep->e_data[ndx].a_w.w_float;
				}
            }
            coll_keyoutput(x, found);
            outlet_float(((t_object *)x)->ob_outlet, result);
        }
    }
}

static void coll_max(t_coll *x, t_floatarg f){
    int ndx;
    if(coll_checkint((t_pd *)x, f, &ndx, gensym("max"))){
        t_collelem *found;
        if(ndx > 0)
            ndx--;
        /* LATER consider complaining: */
        else if(ndx < 0)
            return;  /* CHECKED silently rejected */
        /* else CHECKED silently defaults to 1 */
        if((found = coll_firsttyped(x, ndx, A_FLOAT))){
            t_float result = found->e_data[ndx].a_w.w_float;
            t_collelem *ep;
            for(ep = found->e_next; ep; ep = ep->e_next){
				if(ep->e_size > ndx &&
					ep->e_data[ndx].a_type == A_FLOAT &&
					ep->e_data[ndx].a_w.w_float > result)
				{
					found = ep;
					result = ep->e_data[ndx].a_w.w_float;
				}
            }
            coll_keyoutput(x, found);
            outlet_float(((t_object *)x)->ob_outlet, result);
        }
    }
}

static void coll_refer(t_coll *x, t_symbol *s){
    coll_rebind(x, s);
}

static void coll_flags(t_coll *x, t_float f1, t_float f2){
    int i1;
    f2 = 0;
    if(coll_checkint((t_pd *)x, f1, &i1, gensym("flags"))){
        t_collcommon *cc = x->x_common;
        cc->c_embedflag = (i1 != 0);
    }
}

static void coll_embed(t_coll *x, t_float f){
    int embed = f == 0 ? 0 : 1;
    coll_flags(x, (t_float) embed, 0);
}

static void coll_read(t_coll *x, t_symbol *s){
	if(!x->unsafe) {
		t_collcommon *cc = x->x_common;
		if(s && s != &s_) {
			x->x_s = coll_fullfilename(x,s);
			if(x->x_threaded == 1) {
				x->unsafe = 1;
				pthread_mutex_lock(&x->unsafe_mutex);
				pthread_cond_signal(&x->unsafe_cond);
				pthread_mutex_unlock(&x->unsafe_mutex);
				//collcommon_doread(cc, s, x->x_canvas, 0);
			}
			else{
				t_msg * msg = collcommon_doread(cc, s, x->x_canvas, 0);
                if((!COLL_ALLBANG) && (msg->m_line > 0)){
                    x->x_filebang = 1;
                    clock_delay(x->x_clock, 0);
                };
			}
		}
		else
			panel_open(cc->c_filehandle, 0);
	}
}

static void coll_write(t_coll *x, t_symbol *s){
	if(!x->unsafe){
        t_collcommon *cc = x->x_common;
		if(s && s != &s_){
            x->x_s = coll_fullfilename(x,s);
            if(x->x_threaded == 1){
                x->unsafe = 10;
                pthread_mutex_lock(&x->unsafe_mutex);
                pthread_cond_signal(&x->unsafe_cond);
                pthread_mutex_unlock(&x->unsafe_mutex);
                //collcommon_dowrite(cc, s, x->x_canvas, 0);
            }
            else
                collcommon_dowrite(cc, s, x->x_canvas, 0);
		}
		else
            panel_save(cc->c_filehandle, 0, 0);  /* CHECKED no default name */
	}
}

static void coll_readagain(t_coll *x){
    t_collcommon *cc = x->x_common;
    if(cc->c_filename){
        if(x->x_threaded == 1){
            x->unsafe = 2;
            pthread_mutex_lock(&x->unsafe_mutex);
            pthread_cond_signal(&x->unsafe_cond);
            pthread_mutex_unlock(&x->unsafe_mutex);
            //collcommon_doread(cc, 0, 0, 0);
        }
        else {
            t_msg * msg = collcommon_doread(cc, 0, x->x_canvas, 0);
            if((!COLL_ALLBANG) && msg->m_line > 0){
                x->x_filebang = 1;
                clock_delay(x->x_clock, 0);
            };
        }
    }
    else
		panel_open(cc->c_filehandle, 0);
}

static void coll_writeagain(t_coll *x){
    t_collcommon *cc = x->x_common;
    if(cc->c_filename) {
        if(x->x_threaded == 1) {
            x->unsafe = 11;
            pthread_mutex_lock(&x->unsafe_mutex);
            pthread_cond_signal(&x->unsafe_cond);
            pthread_mutex_unlock(&x->unsafe_mutex);
            //collcommon_dowrite(cc, 0, 0, 0);
        }
        else
            collcommon_dowrite(cc, 0, 0, 0);
    }
    else
		panel_save(cc->c_filehandle, 0, 0);  /* CHECKED no default name */
}

static void coll_filetype(t_coll *x, t_symbol *s, int argc, t_atom * argv){
    s = NULL;
    int ext_set = 0;
    if(argc > 0){
        if(argv[0].a_type == A_SYMBOL){
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            if(cursym != &s_){
                if(strlen(cursym->s_name) <= COLL_MAXEXTLEN){
                    x->x_fileext = cursym;
                    ext_set = 1;
                };
            };
        };
    };
    if(ext_set <= 0)
        x->x_fileext = &s_;
}

static void coll_dump(t_coll *x){
    t_collcommon *cc = x->x_common;
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next){
		coll_keyoutput(x, ep);
		if(cc->c_selfmodified)
			break;
		coll_dooutput(x, ep->e_size, ep->e_data);
		/* FIXME dooutput() may invalidate ep as well as keyoutput()... */
    }
    outlet_bang(x->x_dumpbangout);
}

/* #ifdef COLL_DEBUG
static void collelem_post(t_collelem *ep)
{
		if(ep->e_hasnumkey && ep->e_symkey)
		loudbug_startpost("%d %s:", ep->e_numkey, ep->e_symkey->s_name);
		else if(ep->e_hasnumkey)
		loudbug_startpost("%d:", ep->e_numkey);
		else if(ep->e_symkey)
		loudbug_startpost("%s:", ep->e_symkey->s_name);
		else loudbug_bug("collcommon_post");
		loudbug_postatom(ep->e_size, ep->e_data);
		loudbug_endpost();
}


static void collcommon_post(t_collcommon *cc)
{
    t_collelem *ep;
    for(ep = cc->c_first; ep; ep = ep->e_next) collelem_post(ep);
}

static void coll_debug(t_coll *x, t_floatarg f)
{
		t_collcommon *cc = coll_checkcommon(x);
		if(cc)
		{
			t_coll *x1 = cc->c_refs;
			t_collelem *ep, *last;
			int i = 0;
			while(x1) i++, x1 = x1->x_next;
			bug("refcount %d", i);
			for(ep = cc->c_first, last = 0; ep; ep = ep->e_next) last = ep;
			if(last != cc->c_last) bug("coll_debug: last element");
			collcommon_post(cc);
		}
}
#endif */

static void *coll_threaded_fileio(void *ptr){
	t_threadedFunctionParams *rPars = (t_threadedFunctionParams*)ptr;
	t_coll *x = rPars->x;
	t_msg *m = NULL;
	while(x->unsafe > -1){
		pthread_mutex_lock(&x->unsafe_mutex);
		if(x->unsafe == 0)
			if(!x->init)
                x->init = 1;
        pthread_cond_wait(&x->unsafe_cond, &x->unsafe_mutex);
		if(x->unsafe == 1) { //read
			m = collcommon_doread(x->x_common, x->x_s, x->x_canvas, 1);
			if(m->m_flag)
				coll_enqueue_threaded_msgs(x, m);
            if(m->m_line > 0){
                x->x_filebang = 1;
            };
			clock_delay(x->x_clock, 0);
		}
		else if(x->unsafe == 2) { //read
			m = collcommon_doread(x->x_common, 0, x->x_canvas, 1);
			if(m->m_flag)
				coll_enqueue_threaded_msgs(x, m);
            if(m->m_line > 0){
                x->x_filebang = 1;
            };
			clock_delay(x->x_clock, 0);
		}
		else if(x->unsafe == 10) { //write
			m = collcommon_dowrite(x->x_common, x->x_s, x->x_canvas, 1);
			if(m->m_flag)
				coll_enqueue_threaded_msgs(x, m);
		}
		else if(x->unsafe == 11) { //write
			m = collcommon_dowrite(x->x_common, 0, 0, 1);
			if(m->m_flag)
				coll_enqueue_threaded_msgs(x, m);
		};
		if(m != NULL){
			t_freebytes(m, sizeof(*m));
			m = NULL;
		}
		if(x->unsafe != -1)
            x->unsafe = 0;
		pthread_mutex_unlock(&x->unsafe_mutex);
	}
	pthread_exit(0);
}

static void coll_separate(t_coll *x, t_floatarg f){
	int indx;
	t_collcommon *cc = x->x_common;
	if(coll_checkint((t_pd *)x, f, &indx, gensym("separate"))){
		t_collelem *ep;
		for(ep = cc->c_first; ep; ep = ep->e_next)
			if(ep->e_hasnumkey && ep->e_numkey >= indx)
				ep->e_numkey += 1;
		collcommon_modified(cc, 0);
        coll_update(x);
	}
}

static void coll_dothread(t_coll *x, int create){
    if(create){
        int ret;
        t_threadedFunctionParams rPars;
        x->unsafe = 0;
        rPars.x = x;
        pthread_mutex_init(&x->unsafe_mutex, NULL);
        pthread_cond_init(&x->unsafe_cond, NULL);
        ret = pthread_create( &x->unsafe_t, NULL, (void *) &coll_threaded_fileio, (void *) &rPars);
        while(!x->init)
            sched_yield();
    }
    else{
        x->unsafe = -1;
        pthread_mutex_lock(&x->unsafe_mutex);
		pthread_cond_signal(&x->unsafe_cond);
		pthread_mutex_unlock(&x->unsafe_mutex);
		pthread_join(x->unsafe_t, NULL);
		pthread_mutex_destroy(&x->unsafe_mutex);
		if(x->x_q)
			coll_q_free(x);
        x->unsafe = 0;
    };
}

static void coll_threaded(t_coll *x, t_float f){
    int th = f == 0 ? 0 : 1;
    if(th != x->x_threaded)
        coll_dothread(x, th);
    x->x_threaded = th;
}

static void coll_free(t_coll *x){
    coll_wclose(x);
	if(x->x_threaded == 1)
        coll_dothread(x, 0);
    pd_unbind(&x->x_ob.ob_pd, x->x_bindsym);
    clock_free(x->x_clock);
    file_free(x->x_filehandle);
    coll_unbind(x);
}

static void *coll_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_coll *x = (t_coll *)pd_new(coll_class);
    t_symbol *file = NULL;
    x->x_fileext = &s_;
    x->x_canvas = canvas_getcurrent();
    char buf[MAXPDSTRING];
    buf[MAXPDSTRING-1] = 0;
    sprintf(buf, "#%lx", (long)x);
    pd_bind(&x->x_ob.ob_pd, x->x_bindsym = gensym(buf));
    outlet_new((t_object *)x, &s_);
    x->x_keyout = outlet_new((t_object *)x, &s_);
    x->x_filebangout = outlet_new((t_object *)x, &s_bang);
    x->x_dumpbangout = outlet_new((t_object *)x, &s_bang);
    x->x_filehandle = file_new((t_pd *)x, coll_embedhook, 0, 0, 0);
    int no_search = 0;
    int embed = COLLEMBED;
    int threaded = COLLTHREAD;
    // check arguments for filename and threaded version
    // right now, don't care about arg order (mb we should?) - DK
    while(argc){
        if(argv -> a_type == A_SYMBOL){
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            argc--;
            argv++;
            if(strcmp(cursym -> s_name, "@embed") == 0){
                if(argc){
                    t_float curf = atom_getfloatarg(0, argc, argv);
                    argc--;
                    argv++;
                    embed = curf > 0 ? 1 : 0;
                };
            }
            else if(strcmp(cursym -> s_name, "@threaded") == 0){
                if(argc){
                    t_float curf = atom_getfloatarg(0, argc, argv);
                    argc--;
                    argv++;
                    threaded = curf != 0 ? 1 : 0;
                };
            }
            else
                file = cursym;
        }
        else if(argv -> a_type == A_FLOAT){
            t_float nosearch = atom_getfloatarg(0, argc, argv);
            no_search = nosearch != 0 ? 1 : 0;
            argc--;
            argv++;
        }
        else
            goto errstate;
    };
    x->x_is_opened = 0;
    x->x_threaded = 0; //changed from -1 which broke on windows
    x->x_nosearch = no_search;
    x->x_filebang = 0;
	// if no file name provided, associate with empty symbol
	if(file == NULL)
		file = &s_;
	x->unsafe = 0;
	x->init = 0;
    //lines below used to be only for threaded, but it's
    //needed for bang on the 3rd outlet - DK & Porres
    x->x_clock = clock_new(x, (t_method)coll_tick);
	coll_threaded(x, threaded);
    coll_bind(x, file);
    coll_flags(x, (int)embed, 0);
    return(x);
	errstate:
		pd_error(x, "coll: improper args");
		return NULL;
}

CYCLONE_OBJ_API void coll_setup(void){
    coll_class = class_new(gensym("coll"), (t_newmethod)coll_new,
        (t_method)coll_free, sizeof(t_coll), 0, A_GIMME, 0);
    class_addbang(coll_class, coll_next);
    class_addfloat(coll_class, coll_float);
    class_addsymbol(coll_class, coll_symbol);
    class_addlist(coll_class, coll_list);
    class_addanything(coll_class, coll_anything);
    class_addmethod(coll_class, (t_method)coll_store, gensym("store"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_nstore, gensym("nstore"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_insert, gensym("insert"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_insert2, gensym("insert2"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_remove, gensym("remove"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_delete, gensym("delete"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_assoc, gensym("assoc"), A_SYMBOL, A_FLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_deassoc, gensym("deassoc"), A_SYMBOL, A_FLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_subsym, gensym("subsym"), A_SYMBOL, A_SYMBOL, 0);
    class_addmethod(coll_class, (t_method)coll_renumber, gensym("renumber"), A_DEFFLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_renumber2, gensym("renumber2"), A_DEFFLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_merge, gensym("merge"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_sub, gensym("sub"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_sub, gensym("nsub"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_clear, gensym("clear"), 0);
    class_addmethod(coll_class, (t_method)coll_sort, gensym("sort"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_swap, gensym("swap"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_next, gensym("next"), 0);
    class_addmethod(coll_class, (t_method)coll_prev, gensym("prev"), 0);
    class_addmethod(coll_class, (t_method)coll_end, gensym("end"), 0);
    class_addmethod(coll_class, (t_method)coll_goto, gensym("goto"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_nth, gensym("nth"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_length, gensym("length"), 0);
    class_addmethod(coll_class, (t_method)coll_min, gensym("min"), A_DEFFLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_max, gensym("max"), A_DEFFLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_refer, gensym("refer"), A_SYMBOL, 0);
    class_addmethod(coll_class, (t_method)coll_flags, gensym("flags"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_embed, gensym("embed"), A_FLOAT,0);
    class_addmethod(coll_class, (t_method)coll_threaded, gensym("threaded"), A_FLOAT,0);
    class_addmethod(coll_class, (t_method)coll_read, gensym("read"), A_DEFSYM, 0);
    class_addmethod(coll_class, (t_method)coll_start, gensym("start"), 0);
    class_addmethod(coll_class, (t_method)coll_write, gensym("write"), A_DEFSYM, 0);
    class_addmethod(coll_class, (t_method)coll_readagain, gensym("readagain"), 0);
    class_addmethod(coll_class, (t_method)coll_writeagain, gensym("writeagain"), 0);
    class_addmethod(coll_class, (t_method)coll_filetype, gensym("filetype"), A_GIMME, 0);
    class_addmethod(coll_class, (t_method)coll_dump, gensym("dump"), 0);
    class_addmethod(coll_class, (t_method)coll_open, gensym("open"), 0);
    class_addmethod(coll_class, (t_method)coll_wclose, gensym("wclose"), 0);
    class_addmethod(coll_class, (t_method)coll_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_separate, gensym("separate"), A_FLOAT, 0);
    class_addmethod(coll_class, (t_method)coll_is_opened, gensym("_is_opened"), A_FLOAT , A_FLOAT, 0);
/* #ifdef COLL_DEBUG
    class_addmethod(coll_class, (t_method)coll_debug,
		    gensym("debug"), A_DEFFLOAT, 0);
#endif */
    file_setup(coll_class, 1);
    collcommon_class = class_new(gensym("coll"), 0, 0, sizeof(t_collcommon), CLASS_PD, 0);
    /* this call is a nop (collcommon does not embed, and the file
       class itself has been already set up above), but it is better to
       have it around, just in case... */
    file_setup(collcommon_class, 0);
}

