
#include <string.h>
#include "m_pd.h"

#define HOT_MINSLOTS    2
#define HOT_SIZE        128

typedef struct _hot{
    t_object    x_ob;
    int         x_multiatom;
    int         x_nslots;
    int         x_nproxies;  /* as requested (and allocated) */
    t_pd      **x_proxies;
    t_outlet  **x_outs;
} t_hot;

typedef struct _hot_proxy{
    t_object     p_ob;
    t_hot     *p_master;
    int          p_id;
    t_symbol    *p_selector;
    t_float      p_float;
    t_symbol    *p_symbol;
    t_gpointer  *p_pointer;
    int          p_size;    /* as allocated */
    int          p_natoms;  /* as used */
    t_atom      *p_message;
    t_atom       p_messini[HOT_SIZE];
}t_hot_proxy;

static t_class *hot_class;
static t_class *hot_proxy_class;

static void hot_doit(t_hot *x){
    t_hot_proxy **p = (t_hot_proxy **)x->x_proxies;
    int i = x->x_nslots;
    p = (t_hot_proxy **)x->x_proxies;
    i = x->x_nslots;
    while (i--){
        t_symbol *s = p[i]->p_selector;
        /* LATER consider complaining about extra arguments (CHECKED) */
        if (s == &s_bang)
        outlet_bang(x->x_outs[i]);
        else if (s == &s_float)
        outlet_float(x->x_outs[i], p[i]->p_float);
        else if (s == &s_symbol && p[i]->p_symbol){
            outlet_symbol(x->x_outs[i], p[i]->p_symbol);
            /* LATER rethink */
            /* old code
             if (x->x_multiatom)
             outlet_symbol(x->x_outs[i], p[i]->p_symbol);
             else
             outlet_anything(x->x_outs[i], p[i]->p_symbol, 0, 0);
             */
        }
        else if (s == &s_pointer){
            /* LATER */
        }
        else if (s == &s_list)
            outlet_list(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
        else if (s)  /* CHECKED: a slot may be inactive (in multiatom mode) */
            outlet_anything(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
        //added for 1-elt anythings that are symbols
        else if(!s && (p[i]->p_symbol != &s_ || p[i]->p_symbol != NULL) && p[i]->p_natoms == 0)
            outlet_anything(x->x_outs[i], p[i]->p_symbol, 0, 0);
    }
}

static void hot_arm(t_hot *x){
    hot_doit(x);
}

static void hot_proxy_bang(t_hot_proxy *x){
    hot_arm(x->p_master);  /* CHECKED: bang in any inlet works in this way */
}

static void hot_proxy_dofloat(t_hot_proxy *x, t_float f, int doit){
    x->p_selector = &s_float;
    x->p_float = f;
    x->p_natoms = 0;  /* defensive */
    if (doit) hot_arm(x->p_master);
}

static void hot_proxy_float(t_hot_proxy *x, t_float f){
    hot_proxy_dofloat(x, f, 1);
}

//when we don't want a selector for symbol - DK
static void hot_proxy_donosymbol(t_hot_proxy *x, t_symbol *s, int doit){
    x->p_selector = NULL;
    x->p_symbol = s;
    x->p_natoms = 0;  /* defensive */
    if (doit) hot_arm(x->p_master);
}

static void hot_proxy_dosymbol(t_hot_proxy *x, t_symbol *s, int doit){
    x->p_selector = &s_symbol;
    x->p_symbol = s;
    x->p_natoms = 0;  /* defensive */
    if (doit) hot_arm(x->p_master);
}

static void hot_proxy_symbol(t_hot_proxy *x, t_symbol *s){
    hot_proxy_dosymbol(x, s, 1);
}

static void hot_proxy_dopointer(t_hot_proxy *x, t_gpointer *gp, int doit){
    x->p_selector = &s_pointer;
    x->p_pointer = gp;
    x->p_natoms = 0;  /* defensive */
    if (doit) hot_arm(x->p_master);
}

static void hot_proxy_pointer(t_hot_proxy *x, t_gpointer *gp){
    hot_proxy_dopointer(x, gp, 1);
}

/* CHECKED: the slots fire in right-to-left order,
   but they trigger only once (refman error) */
static void hot_distribute(t_hot *x, int startid,
			     t_symbol *s, int ac, t_atom *av, int doit){
    t_atom *ap = av;
    t_hot_proxy **pp;
    int id = startid + ac;
    if (s) id++;
    if (id > x->x_nslots)
	id = x->x_nslots;
    ap += id - startid;
    pp = (t_hot_proxy **)(x->x_proxies + id);
    if (s) ap--;
    while (ap-- > av){
        pp--;
        if (ap->a_type == A_FLOAT)
            hot_proxy_dofloat(*pp, ap->a_w.w_float, 0);
        else if (ap->a_type == A_SYMBOL)
            hot_proxy_donosymbol(*pp, ap->a_w.w_symbol, 0);
        else if (ap->a_type == A_POINTER)
            hot_proxy_dopointer(*pp, ap->a_w.w_gpointer, 0);
    }
    if(s)
        hot_proxy_donosymbol((t_hot_proxy *)x->x_proxies[startid], s, 0);
    if(doit)
        hot_arm(x);
}

static void hot_proxy_domultiatom(t_hot_proxy *x, int ac, t_atom *av, int doit){
    if (ac > x->p_size){
        pd_error(x, "hot: maximum size is %d elements", (int)HOT_SIZE);
    }
    x->p_natoms = ac;
    memcpy(x->p_message, av, ac * sizeof(*x->p_message));
    if (doit) hot_arm(x->p_master);
}

static void hot_proxy_dolist(t_hot_proxy *x, int ac, t_atom *av, int doit){
    if (x->p_master->x_multiatom){
        x->p_selector = &s_list;
        hot_proxy_domultiatom(x, ac, av, doit);
    }
    else
        hot_distribute(x->p_master, x->p_id, 0, ac, av, doit);
}

static void hot_proxy_list(t_hot_proxy *x,
			     t_symbol *s, int ac, t_atom *av){
    hot_proxy_dolist(x, ac, av, 1);
}

static void hot_proxy_doanything(t_hot_proxy *x,
				   t_symbol *s, int ac, t_atom *av, int doit){
    if (x->p_master->x_multiatom){
        /* LATER rethink and CHECKME */
        if (s == &s_symbol){
            if (ac && av->a_type == A_SYMBOL)
            hot_proxy_dosymbol(x, av->a_w.w_symbol, doit);
            else
            hot_proxy_dosymbol(x, &s_symbol, doit);
        }
        else{
            x->p_selector = s;
            hot_proxy_domultiatom(x, ac, av, doit);
        }
    }
    else hot_distribute(x->p_master, x->p_id, s, ac, av, doit);
}

static void hot_proxy_anything(t_hot_proxy *x, t_symbol *s, int ac, t_atom *av){
    hot_proxy_doanything(x, s, ac, av, 1);
}

static void hot_proxy_set(t_hot_proxy *x, t_symbol *s, int ac, t_atom *av){
    if (ac){
        if (av->a_type == A_FLOAT){
            if (ac > 1)
            hot_proxy_dolist(x, ac, av, 0);
            else
            hot_proxy_dofloat(x, av->a_w.w_float, 0);
        }
        else if (av->a_type == A_SYMBOL)
        /* CHECKED: no tests for 'set float ...' and 'set list...' --
         the parsing is made in an output routine */
        hot_proxy_doanything(x, av->a_w.w_symbol, ac-1, av+1, 0);
        else if (av->a_type == A_POINTER)
        hot_proxy_dopointer(x, av->a_w.w_gpointer, 0);
    }
    /* CHECKED: 'set' without arguments makes a slot inactive,
       if multiatom, but is ignored, if !multiatom */
    else if (x->p_master->x_multiatom)
        x->p_selector = 0;
}

static void hot_bang(t_hot *x){
    hot_proxy_bang((t_hot_proxy *)x->x_proxies[0]);
}

static void hot_float(t_hot *x, t_float f){
    hot_proxy_dofloat((t_hot_proxy *)x->x_proxies[0], f, 1);
}

static void hot_symbol(t_hot *x, t_symbol *s){
    hot_proxy_dosymbol((t_hot_proxy *)x->x_proxies[0], s, 1);
}

static void hot_pointer(t_hot *x, t_gpointer *gp){
    hot_proxy_dopointer((t_hot_proxy *)x->x_proxies[0], gp, 1);
}

static void hot_list(t_hot *x, t_symbol *s, int ac, t_atom *av){
    hot_proxy_dolist((t_hot_proxy *)x->x_proxies[0], ac, av, 1);
}

static void hot_anything(t_hot *x, t_symbol *s, int ac, t_atom *av){
    hot_proxy_doanything((t_hot_proxy *)x->x_proxies[0], s, ac, av, 1);
}

static void hot_set(t_hot *x, t_symbol *s, int ac, t_atom *av){
    hot_proxy_set((t_hot_proxy *)x->x_proxies[0], s, ac, av);
}

static void hot_free(t_hot *x){
    if (x->x_proxies){
        int i = x->x_nslots;
        while (i--){
            t_hot_proxy *y = (t_hot_proxy *)x->x_proxies[i];
            if (y->p_message != y->p_messini)
            freebytes(y->p_message, y->p_size * sizeof(*y->p_message));
            pd_free((t_pd *)y);
        }
        freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
    if (x->x_outs)
        freebytes(x->x_outs, x->x_nslots * sizeof(*x->x_outs));
}

static void *hot_new(t_floatarg f){
    t_hot *x;
    int i, nslots, nproxies = HOT_MINSLOTS;
    int multiatom = 1;
    t_pd **proxies;
    t_outlet **outs;
    nproxies = (int)f;
    if (nproxies < HOT_MINSLOTS)
        nproxies = HOT_MINSLOTS;
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies))))
	return (0);
    for (nslots = 0; nslots < nproxies; nslots++)
	if (!(proxies[nslots] = pd_new(hot_proxy_class))) break;
    if (nslots < HOT_MINSLOTS
	|| !(outs = (t_outlet **)getbytes(nslots * sizeof(*outs))))
    {
	i = nslots;
	while (i--) pd_free(proxies[i]);
	freebytes(proxies, nproxies * sizeof(*proxies));
	return (0);
    }
    x = (t_hot *)pd_new(hot_class);
    x->x_multiatom = multiatom;
    x->x_nslots = nslots;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    x->x_outs = outs;
    for (i = 0; i < nslots; i++)
    {
	t_hot_proxy *y = (t_hot_proxy *)proxies[i];
	y->p_master = x;
	y->p_id = i;
	y->p_selector = &s_float;  /* CHECKED: it is so in multiatom mode too */
	y->p_float = 0;
	y->p_symbol = 0;
	y->p_pointer = 0;
	y->p_size = HOT_SIZE;
	y->p_natoms = 0;
	y->p_message = y->p_messini;
	if (i) inlet_new((t_object *)x, (t_pd *)y, 0, 0);
	x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    }
    return (x);
}

void hot_setup(void)
{
    hot_class = class_new(gensym("hot"), (t_newmethod)hot_new, (t_method)hot_free,
			    sizeof(t_hot), 0, A_DEFFLOAT, 0);
    class_addbang(hot_class, hot_bang);
    class_addfloat(hot_class, hot_float);
    class_addsymbol(hot_class, hot_symbol);
    class_addpointer(hot_class, hot_pointer);
    class_addlist(hot_class, hot_list);
    class_addanything(hot_class, hot_anything);
    class_addmethod(hot_class, (t_method)hot_set, gensym("set"), A_GIMME, 0);
    hot_proxy_class = class_new(gensym("_hot_proxy"), 0, 0,
				  sizeof(t_hot_proxy),
				  CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(hot_proxy_class, hot_proxy_bang);
    class_addfloat(hot_proxy_class, hot_proxy_float);
    class_addsymbol(hot_proxy_class, hot_proxy_symbol);
    class_addpointer(hot_proxy_class, hot_proxy_pointer);
    class_addlist(hot_proxy_class, hot_proxy_list);
    class_addanything(hot_proxy_class, hot_proxy_anything);
    class_addmethod(hot_proxy_class, (t_method)hot_proxy_set,
		    gensym("set"), A_GIMME, 0);
}
