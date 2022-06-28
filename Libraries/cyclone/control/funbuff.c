/* Copyright (c) 2002-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <libgen.h>
#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "control/tree.h"
#include "common/file.h"


/*   FUNCTIONALITY CHANGES (Derek Kwan 2016)
    -   adding cut/copy/paste/select/undo functionality:
        fbuffcom, the clipboard, comes from ideas from value in pd's src/x_connective.c
        this will serve as funbuff's common clipboard
    -   funbuff_set no longer clears previous contents
    -   yvals now typecast to int (previously floats)
*/


//stack size
#define FBUFFATOM_STACK 256
//max alloc size
#define FBUFFATOM_MAX 1024


//history past event identifier 
typedef enum _pastaction
{
NOACT, //placeholder for action not cut or paste
CUT,
PASTE

} t_pastaction;

typedef struct _funbuffcom
{

    t_pd c_pd;
    t_atom * c_pairs; //xvals and yvals
    int     c_refcount; //how many funbuffs are active
    int     c_allocsz; //allocated size of list
    int     c_pairsz;  //size of xval/yval list
    t_atom  c_pairstack[FBUFFATOM_STACK]; //list stack
    int     c_heaped; //if heaped or not
} t_funbuffcom;

typedef struct _funbuff
{
    t_object   x_ob;
    t_canvas  *x_canvas;
    t_symbol  *x_defname;
    t_float    x_value;
    int        x_valueset;
    /* CHECKED filling with a large set, then sending 'goto', 'read', 'next'...
       outputs the previous, replaced contents (same with deletion)
       -- apparently a node pointer is stored, corrupt in these cases */
    t_hammernode  *x_pointer;
    int            x_pointerset;  /* set-with-goto flag */

    t_hammernode *x_selptr; //pointer for select
    int           x_selptrset; //set with select
    int            x_selstart; //select start index
    int           x_selrng; //select range
    t_funbuffcom   *x_clip; //clipboard
    int            x_lastdelta;
    int            x_embedflag;
    t_file  *x_filehandle;
    t_hammertree   x_tree;
    t_outlet      *x_deltaout;
    t_outlet      *x_bangout;

    //history tracking
    t_atom *        x_hist; //history pairs pointer
    t_atom          x_histstack[FBUFFATOM_STACK];
    int             x_allocsz; //allocated size of list
    int             x_histsz; //size of history list
    int             x_heaped; //if heaped or not
    t_pastaction    x_past; //last action affecting instance of funbuff


} t_funbuff;

static t_class *funbuff_class;
static t_class *funbuffcom_class;

static void fbuffmem_alloc(t_atom * arrayptr, t_atom * stackptr, int wantsz, int * allocsz, int * ifheaped){
     int cursize = *allocsz;
    int heaped = *ifheaped;


    if(heaped  &&  wantsz <= FBUFFATOM_STACK){
        //if x_value is pointing to a heap and incoming list can fit into FBUFFATOM_STACK
        //deallocate x_value and point it to stack, update status
        freebytes( arrayptr, (cursize) * sizeof(t_atom));
        arrayptr = stackptr;
        *ifheaped = 0;
        *allocsz = FBUFFATOM_STACK;
        }
    else if(heaped  && wantsz > FBUFFATOM_STACK && wantsz > cursize){
        //if already heaped, incoming list can't fit into FBUFFATOM_STACK and can't fit into allocated t_atom
        //reallocate to accommodate larger list, update status
        
        int toalloc = wantsz; //size to allocate
        //bounds checking for maxsize
        if(toalloc > FBUFFATOM_MAX){
            toalloc = FBUFFATOM_MAX;
        };
        arrayptr = (t_atom *)resizebytes(arrayptr, cursize * sizeof(t_atom), toalloc * sizeof(t_atom));
        *allocsz = toalloc;
    }
    else if(!(heaped) && wantsz > FBUFFATOM_STACK){
        //if not heaped and incoming list can't fit into FBUFFATOM_STACK
        //allocate and update status

        int toalloc = wantsz; //size to allocate
        //bounds checking for maxsize
        if(toalloc > FBUFFATOM_MAX){
            toalloc = FBUFFATOM_MAX;
        };
        arrayptr = (t_atom *)getbytes(toalloc * sizeof(t_atom));
        *ifheaped = 1;
        *allocsz = toalloc;
    };

}

//value allocation helper function
static void funbuffcom_alloc(t_funbuffcom *c, int argc){
    fbuffmem_alloc(c->c_pairs, c->c_pairstack, argc, &c->c_allocsz, &c->c_heaped);
}

static void funbuff_alloc(t_funbuff * x, int argc){
    fbuffmem_alloc(x->x_hist, x->x_histstack, argc, &x->x_allocsz, &x->x_heaped);
}

static void funbuff_histcopy(t_funbuff *x)
{
    //copy contents of clipboard to history
    int i =0;
    t_funbuffcom * c = x->x_clip;
    int clipsize = c->c_pairsz; //length of clipboard
    if(clipsize != (x->x_histsz)){
        funbuff_alloc(x, clipsize);
    };
    for(i=0; i<clipsize; i++){
        t_float f = atom_getfloatarg(i, clipsize, c->c_pairs);
        SETFLOAT(&x->x_hist[i], f);
    };
    x->x_histsz = clipsize;

}

void funbuffcom_release(void)
{
    t_symbol * clipname = gensym("cyfunbuffclip");
    t_funbuffcom *c = (t_funbuffcom *)pd_findbyclass(clipname, funbuffcom_class);
    if (c)
    {
        if (!--c->c_refcount)
        {
            if(c->c_heaped){

                freebytes((c->c_pairs), (c->c_allocsz) * sizeof(t_atom));
             };
            pd_unbind(&c->c_pd, clipname);
            pd_free(&c->c_pd);
        }
    }
    else bug("funbuffcom_release");
}


static void funbuff_dooutput(t_funbuff *x, float value, float delta)
{
    /* CHECKED lastdelta sent for 'next', 'float', 'min', 'max',
       'interp', 'find' */
    outlet_float(x->x_deltaout, delta);
    outlet_float(((t_object *)x)->ob_outlet, value);
}

static void funbuff_bang(t_funbuff *x)
{
    t_hammernode *np;
    int count = 0;
    int xmin = 0, xmax = 0;
    t_float ymin = 0, ymax = 0;
    if((np = x->x_tree.t_first))
    {
	/* LATER consider using extra fields, updated on the fly */
	count = 1;
	xmin = np->n_key;
	xmax = x->x_tree.t_last->n_key;
	ymin = ymax = HAMMERNODE_GETFLOAT(np);
	while((np = np->n_next))
	{
	    t_float f = HAMMERNODE_GETFLOAT(np);
	    if (f < ymin)
		ymin = f;
	    else if (f > ymax)
		ymax = f;
	    count++;
	}
    }
    /* format CHECKED */
    post("funbuff info:  %d elements long", count);  /* CHECKED 0 and 1 */
    if (count)
    {
	post(" -> minX= %d maxX= %d", xmin, xmax);
	post(" -> minY= %g maxY= %g", ymin, ymax);
	post(" -> domain= %d range= %g", xmax - xmin, ymax - ymin);
    }
}

static void funbuff_float(t_funbuff *x, t_float f)
{
    int ndx = (int)f;  /* CHECKED float is silently truncated */
    t_hammernode *np;
    if (x->x_valueset)
    {
	np = hammertree_insertfloat(&x->x_tree, ndx, x->x_value, 1);
	x->x_valueset = 0;
    }
    else if ((np = hammertree_closest(&x->x_tree, ndx, 0)))
	funbuff_dooutput(x, HAMMERNODE_GETFLOAT(np), x->x_lastdelta);
    /* CHECKED pointer is updated --
       'next' outputs np also in a !valueset case (it is sent twice) */
    x->x_pointer = np;  /* FIXME */
    x->x_pointerset = 0;
}

static void funbuff_ft1(t_funbuff *x, t_floatarg _f)
{
    /* this is incompatible -- CHECKED float is silently truncated */
    int f = (int) _f; //typecast to int 
    x->x_value = (t_float)f;
    x->x_valueset = 1;
}

static void funbuff_ptrreset(t_funbuff * x)
{

    /* CHECKED valueset is not cleared */
    x->x_pointer = 0;
    x->x_selptr = 0;
    x->x_selptrset = 0;
    x->x_past = NOACT;
}

static void funbuff_clear(t_funbuff *x)
{
    hammertree_clear(&x->x_tree, 0);
    funbuff_ptrreset(x);
}

/* LATER dirty flag handling */
static void funbuff_embed(t_funbuff *x, t_floatarg f)
{
    x->x_embedflag = (f != 0);
}

static void funbuff_goto(t_funbuff *x, t_floatarg f)
{
    /* CHECKED truncation */
    x->x_pointer = hammertree_closest(&x->x_tree, (int)f, 1);
    x->x_pointerset = 1;  /* CHECKED delta output by 'next' will be zero */
}

/* LATER consider using an extra field, updated on the fly */
static void funbuff_min(t_funbuff *x)
{
    t_hammernode *np;
    if ((np = x->x_tree.t_first))  /* CHECKED nop if empty */
    {
	t_float result = HAMMERNODE_GETFLOAT(np);
	while ((np = np->n_next))
	    if (HAMMERNODE_GETFLOAT(np) < result)
		result = HAMMERNODE_GETFLOAT(np);
	funbuff_dooutput(x, result, x->x_lastdelta);
	/* CHECKED pointer not updated */
    }
}

/* LATER consider using an extra field, updated on the fly */
static void funbuff_max(t_funbuff *x)
{
    t_hammernode *np;
    if ((np = x->x_tree.t_first))  /* CHECKED nop if empty */
    {
	t_float result = HAMMERNODE_GETFLOAT(np);
	while ((np = np->n_next))
	    if (HAMMERNODE_GETFLOAT(np) > result)
		result = HAMMERNODE_GETFLOAT(np);
	funbuff_dooutput(x, result, x->x_lastdelta);
	/* CHECKED pointer not updated */
    }
}

static void funbuff_next(t_funbuff *x)
{
    t_hammernode *np;
    if (!x->x_tree.t_root)
	return;
    if (!(np = x->x_pointer))
    {
	outlet_bang(x->x_bangout);
	/* CHECKED banging until reset */
	return;
    }
    if (x->x_pointerset)
	x->x_lastdelta = 0;
    else if (np->n_prev)
	x->x_lastdelta = np->n_key - np->n_prev->n_key;
    else
	x->x_lastdelta = 0;  /* CHECKED corrupt delta sent here... */
    funbuff_dooutput(x, HAMMERNODE_GETFLOAT(np), x->x_lastdelta);
    x->x_pointer = np->n_next;
    x->x_pointerset = 0;
}

static void funbuff_set(t_funbuff *x, t_symbol *s, int ac, t_atom *av)
{
    s = NULL;
    /* CHECKED symbols somehow bashed to zeros,
       decreasing x coords corrupt the funbuff -- not emulated here... */
    int i = ac;
    t_atom *ap = av;
    while (i--) if (ap++->a_type != A_FLOAT)
    {
	pd_error((t_pd *)x, "bad input (not a number) -- no data to set");
	return;
    }
    if (!ac || (ac % 2))
    {
	/* CHECKED odd/null ac loudly rejected, current contents preserved */
        pd_error((t_pd *)x, "bad input (%s) -- no data to set",
		   (ac ? "odd arg count" : "no input"));
	return;
    }
    funbuff_ptrreset(x);
    while (ac--)
    {
	int ndx = (int)av++->a_w.w_float;
        int yval = (int)av++->a_w.w_float; //typecast to int
	if (!hammertree_insertfloat(&x->x_tree, ndx, (t_float)yval, 1))
	    return;
	ac--;
    }
}

static void funbuff_doread(t_funbuff *x, t_symbol *fn){
    t_binbuf *bb = binbuf_new();
    char path[MAXPDSTRING];
//    canvas_makefilename(x->x_canvas, fn->s_name, path, MAXPDSTRING);
    char *bufptr;
    int fd = canvas_open(x->x_canvas, fn->s_name, "", path, &bufptr, MAXPDSTRING, 1);
    if(fd > 0){
        path[strlen(path)]='/';
        sys_close(fd);
    }
    else{
        post("[funbuff] file '%s' not found", fn->s_name);
        return;
    }
    binbuf_read(bb, path, "", 0);
    int ac = binbuf_getnatom(bb);
    t_atom *av = binbuf_getvec(bb);
    if(ac && av && av->a_type == A_SYMBOL && av->a_w.w_symbol == gensym("funbuff")){
        post("funbuff: %s read successful", fn->s_name);  // CHECKED
        funbuff_set(x, 0, ac-1, av+1);
    }
    else  // CHECKED no complaints...
        pd_error((t_pd *)x, "invalid file %s", fn->s_name);
    binbuf_free(bb);
}

static void funbuff_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av)
{
    ac = 0;
    av = NULL;
    funbuff_doread((t_funbuff *)z, fn);
}

static void funbuff_dowrite(t_funbuff *x, t_symbol *fn){
    t_binbuf *bb = binbuf_new();
    char buf[MAXPDSTRING];
    t_hammernode *np;
    /* specifying the object as cyclone/funbuff breaks the file writing/
     * reading as the it doesn't start with 'funbuff'. A call to 
     * libgen/basename fixes this.  fjk, 2015-01-24 */
    t_symbol *objName = atom_getsymbol(binbuf_getvec(x->x_ob.te_binbuf));
    objName->s_name = basename((char *)objName->s_name);
    binbuf_addv(bb, "s", objName);
    for(np = x->x_tree.t_first; np; np = np->n_next)
        binbuf_addv(bb, "if", np->n_key, HAMMERNODE_GETFLOAT(np));
    canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    binbuf_write(bb, buf, "", 0);
    binbuf_free(bb);
}

static void funbuff_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av)
{
    ac = 0;
    av = NULL;
    funbuff_dowrite((t_funbuff *)z, fn);
}

static void funbuff_embedhook(t_pd *z, t_binbuf *bb, t_symbol *bindsym)
{
    t_funbuff *x = (t_funbuff *)z;
    if (x->x_embedflag)
    {
        t_hammernode *np;
        binbuf_addv(bb, "ssi;", bindsym, gensym("embed"), 1);
        if ((np = x->x_tree.t_first))
        {
            binbuf_addv(bb, "ss", bindsym, gensym("set"));
            for (; np; np = np->n_next)
            binbuf_addv(bb, "if", np->n_key, HAMMERNODE_GETFLOAT(np));
            binbuf_addsemi(bb);
        };

    };
    obj_saveformat((t_object *)x, bb);
}

/* CHECKED symbol arg ok */
static void funbuff_read(t_funbuff *x, t_symbol *s)
{
    if (s && s != &s_)
	funbuff_doread(x, s);
    else
	panel_open(x->x_filehandle, 0);
}

/* CHECKED symbol arg not allowed --
   a bug? but CHECKME other classes (cf seq's filetype dilemma) */
static void funbuff_write(t_funbuff *x, t_symbol *s)
{
    if (s && s != &s_) 
	funbuff_dowrite(x, s);
    else  /* CHECKME default name */
	panel_save(x->x_filehandle,
			 canvas_getdir(x->x_canvas), x->x_defname);
}

static void funbuff_delete(t_funbuff *x, t_symbol *s, int ac, t_atom *av)
{
    if (ac && av->a_type == A_FLOAT &&
	(ac == 1 || (ac == 2 && av[1].a_type == A_FLOAT)))
    {
	/* CHECKED float is silently truncated */
	int ndx = (int)av->a_w.w_float;
	t_hammernode *np;
	if ((np = hammertree_search(&x->x_tree, ndx)) &&
	    (ac == 1 || HAMMERNODE_GETFLOAT(np) == av[1].a_w.w_float))
	{
	    if (np == x->x_pointer)
		x->x_pointer = 0;  /* CHECKED corrupt pointer left here... */
            if (np == x->x_selptr){
                x->x_selptr = 0;
                x->x_selptrset = 0;
            };
	    hammertree_delete(&x->x_tree, np);  /* FIXME */
	}
	/* CHECKED mismatch silently ignored */

        x->x_past = NOACT; //clear undo option
    }
    else  pd_error((t_pd *)x, "bad arguments for message \"%s\"", s->s_name);  /* CHECKED */

}

static void funbuff_find(t_funbuff *x, t_floatarg f)
{
    t_hammernode *np;
    if ((np = x->x_tree.t_first))
    {
	do
	{
	    /* CHECKED lastdelta preserved */
	    if (HAMMERNODE_GETFLOAT(np) == f)
//		funbuff_dooutput(x, np->n_key, x->x_lastdelta);
 outlet_float(((t_object *)x)->ob_outlet, np->n_key);
	}
	while ((np = np->n_next));
	/* CHECKED no bangout, no complaint if nothing found */
    }
    else pd_error((t_pd *)x, "nothing to find");  /* CHECKED */
}

static void funbuff_dump(t_funbuff *x){
    t_hammernode *np;
    if((np = x->x_tree.t_first)){
        do{
            x->x_lastdelta = HAMMERNODE_GETFLOAT(np);  /* CHECKED */
            /* float value preserved (this is incompatible) */
            funbuff_dooutput(x, np->n_key, x->x_lastdelta);
        }
        while((np = np->n_next));
	/* CHECKED no bangout */
    }
    else
        pd_error((t_pd *)x, "nothing to dump");  /* CHECKED */
}

/* CHECKME if pointer is updated */
static void funbuff_dointerp(t_funbuff *x, t_floatarg f, int vsz, t_word *vec)
{
    t_hammernode *np1;
    int trunc = (int)f;
    if (trunc > f) trunc--;  /* CHECKME negative floats */
    if ((np1 = hammertree_closest(&x->x_tree, trunc, 0)))
    {
	float value = HAMMERNODE_GETFLOAT(np1);
	t_hammernode *np2 = np1->n_next;
	if (np2)
	{
	    float delta = (float)(np2->n_key - np1->n_key);
	    /* this is incompatible -- CHECKED float argument is silently
	       truncated (which does not make much sense here), CHECKME again */
	    float frac = f - np1->n_key;
	    if (frac < 0 || frac >= delta)
	    {
		bug("funbuff_dointerp");
		return;
	    }
	    frac /= delta;
	    if (vec)
	    {
		/* CHECKME */
		float vpos = (vsz - 1) * frac;
		int vndx = (int)vpos;
		float vfrac = vpos - vndx;
		if (vndx < 0 || vndx >= vsz - 1)
		{
		    bug("funbuff_dointerp redundant test...");
		    return;
		}
		vec += vndx;
		frac = vec[0].w_float + (vec[1].w_float - vec[0].w_float) * vfrac;
	    }
	    value +=
		(HAMMERNODE_GETFLOAT(np2) - HAMMERNODE_GETFLOAT(np1)) * frac;
	}
	funbuff_dooutput(x, value, x->x_lastdelta);  /* CHECKME !np2 */
    }
    else if ((np1 = hammertree_closest(&x->x_tree, trunc, 1)))
	/* CHECKME */
	funbuff_dooutput(x, HAMMERNODE_GETFLOAT(np1), x->x_lastdelta);
}

static void funbuff_interp(t_funbuff *x, t_floatarg f)
{
    funbuff_dointerp(x, f, 0, 0);
}



static void funbuff_interptab(t_funbuff *x, t_symbol *s, t_floatarg f)
{
    int vsz = 0;
    t_word *vec;

    if (s && s != &s_)
    {
	t_garray *ap = (t_garray *)pd_findbyclass(s, garray_class);
	if (ap)
	{
	    if (!garray_getfloatwords(ap, &vsz, &vec))
	    {
                pd_error(x,"bad template of array '%s'", s->s_name);
            };
	}
	else {
	    pd_error(x, "no such array '%s'", s->s_name);
	};
    };


    if (vsz > 2)
	funbuff_dointerp(x, f, vsz, vec);
    else
	funbuff_dointerp(x, f, 0, 0);
}


static void funbuff_select(t_funbuff *x, t_floatarg f1, t_floatarg f2)
{
    int startidx = f1 < 0 ? 0 : (int) f1;
    x->x_selptr = hammertree_closest(&x->x_tree, startidx, 1);
    int rng = f2 < 0 ? 0 : (int) f2;
    if(rng){
        x->x_selstart = startidx;
        x->x_selrng = rng;
        x->x_selptrset = 1;
    };
}

/* CHECKED copy entire contents if no selection */
static void funbuff_copy(t_funbuff *x)
{

    int rng = x->x_selrng;
    if(x->x_selptrset){
        int idx = 0; //index into c_pairs
        t_funbuffcom * c = x->x_clip;
        int cursize = c->c_pairsz;
        //rng * 2 should be good but rng * 3 just to be safe?
        //depends on what rng actually means
        if(cursize != (rng *2)){
            funbuffcom_alloc(c, rng*2);
        };
        int allocsz = c->c_allocsz;
        t_hammernode * np = x->x_selptr;
        while(np && c && idx < allocsz){
	    int xval = np->n_key;
            if(xval >= x->x_selstart + rng){
                break;
            };
	    t_float yval = HAMMERNODE_GETFLOAT(np);
            //post("%d %f", xval, yval);
            SETFLOAT(&c->c_pairs[idx], (t_float)xval);
            idx++;
            SETFLOAT(&c->c_pairs[idx], (t_float)yval);
            idx++;
            np = np->n_next;
        };
        c->c_pairsz = idx;
    }
    else{
        pd_error(x, "funbuff: no data selected");
    };
}

static void funbuff_paste(t_funbuff *x)
{
    t_funbuffcom * c = x->x_clip;
    int clipsize = c->c_pairsz; //length of clipboard
    if(clipsize){
        //record in history
        funbuff_histcopy(x);
        //now insert in to tree
        funbuff_set(x, 0, clipsize, c->c_pairs); 
        x->x_past = PASTE;
    }
    else{
        pd_error(x, "funbuff: clipboard empty");
    };
}

static void funbuff_undo(t_funbuff *x)
{
    /* CHECKED apparently not working in 4.07 */
    if(x->x_past == CUT){
        //opposite of cut = insert
        funbuff_set(x, 0, x->x_histsz, x->x_hist); 
        x->x_past = NOACT;
    }
    else if(x->x_past == PASTE){
        //opposite of paste = delete
        int i = 0;
        for(i=0; i < x->x_histsz; i += 2){
            funbuff_delete(x, gensym("undo"), 2, x->x_hist+i); 
        };
        x->x_past = NOACT;
    };
}

/* CHECKED (sub)buffer's copy is stored, as expected --
   'delete' does not modify the clipboard */
/* CHECKED cut entire contents if no selection */
static void funbuff_cut(t_funbuff *x)
{
    funbuff_copy(x);
    if(x->x_selptrset){
        int i =0;
        t_funbuffcom * c = x->x_clip;
        int clipsize = c->c_pairsz;
        //record into history 
        funbuff_histcopy(x); 
        //do the deletion
        //have to go by pair since delete only goes by pair
        for(i=0; i < clipsize; i += 2){
            funbuff_delete(x, gensym("cut"), 2, c->c_pairs+i); 
        };
        x->x_past = CUT;
    };
    

}

#ifdef HAMMERTREE_DEBUG
static void funbuff_debug(t_funbuff *x, t_floatarg f)
{
    hammertree_debug(&x->x_tree, (int)f, 0);
}
#endif

static void funbuff_free(t_funbuff *x)
{
    file_free(x->x_filehandle);
    hammertree_clear(&x->x_tree, 0);
    funbuffcom_release();
    if(x->x_heaped){
        freebytes(x->x_hist, (x->x_allocsz) * sizeof(t_atom));
    };
}

static void *funbuff_new(t_symbol *s)
{
    t_funbuff *x = (t_funbuff *)pd_new(funbuff_class);
    x->x_canvas = canvas_getcurrent();
    x->x_valueset = 0;
    x->x_pointer = 0;
    x->x_pointerset = 0;  /* CHECKME, rename to intraversal? */
    x->x_selptr = 0;
    x->x_selptrset = 0;
    x->x_lastdelta = 0;
    x->x_embedflag = 0;
    hammertree_inittyped(&x->x_tree, HAMMERTYPE_FLOAT, 0);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_deltaout = outlet_new((t_object *)x, &s_float);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    if (s && s != &s_)
    {
	x->x_defname = s;  /* CHECKME if 'read' changes this */
	funbuff_doread(x, s);
    }
    else x->x_defname = &s_;
    x->x_filehandle = file_new((t_pd *)x, funbuff_embedhook,
				     funbuff_readhook, funbuff_writehook, 0);

    //init history
    

    x->x_histsz = 0;
    x->x_hist = x->x_histstack;
    x->x_heaped = 0;
    x->x_allocsz = FBUFFATOM_STACK;
    x->x_past = NOACT;
    //init funbuff clipboard

    //give it a long symbol name to make sure no overlaps
    t_symbol * clipname = gensym("cyfunbuffclip");
    t_funbuffcom *c = (t_funbuffcom *)pd_findbyclass(clipname, funbuffcom_class);
    if(!c){

        c = (t_funbuffcom *)pd_new(funbuffcom_class);
        pd_bind(&c->c_pd, clipname);
        c->c_refcount = 1;
        c->c_pairsz = 0;
        c->c_pairs = c->c_pairstack;
        c->c_heaped = 0;
        c->c_allocsz = FBUFFATOM_STACK;
    }
    else{
        c->c_refcount = c->c_refcount + 1;
    };
    
    x->x_clip = c;
    return (x);
}

CYCLONE_OBJ_API void funbuff_setup(void)
{
    funbuff_class = class_new(gensym("funbuff"),
			      (t_newmethod)funbuff_new,
			      (t_method)funbuff_free,
			      sizeof(t_funbuff), 0, A_DEFSYM, 0);
    class_addbang(funbuff_class, funbuff_bang);
    class_addfloat(funbuff_class, funbuff_float);
    class_addmethod(funbuff_class, (t_method)funbuff_ft1,
		    gensym("ft1"), A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_clear,
		    gensym("clear"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_goto,
		    gensym("goto"), A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_min,
		    gensym("min"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_max,
		    gensym("max"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_next,
		    gensym("next"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_embed,
		    gensym("embed"), A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_read,
		    gensym("read"), A_DEFSYM, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_write,
		    gensym("write"), A_DEFSYM, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_set,
		    gensym("set"), A_GIMME, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_delete,
		    gensym("delete"), A_GIMME, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_find,
		    gensym("find"), A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_dump,
		    gensym("dump"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_interp,
		    gensym("interp"), A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_interptab,
		    gensym("interptab"), A_FLOAT, A_SYMBOL, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_select,
		    gensym("select"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(funbuff_class, (t_method)funbuff_cut,
		    gensym("cut"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_copy,
		    gensym("copy"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_paste,
		    gensym("paste"), 0);
    class_addmethod(funbuff_class, (t_method)funbuff_undo,
		    gensym("undo"), 0);
#ifdef HAMMERTREE_DEBUG
    class_addmethod(funbuff_class, (t_method)funbuff_debug,
		    gensym("debug"), A_DEFFLOAT, 0);
#endif
    file_setup(funbuff_class, 1);

    funbuffcom_class = class_new(gensym("funbuffcom"), 0, 0,
        sizeof(t_funbuffcom), CLASS_PD, 0);
}
