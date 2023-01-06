// based on vanilla's pipe

#include "m_pd.h"

static t_class *pipe2_class;

typedef struct _hang
{
    t_clock             *h_clock;
    struct _hang        *h_next;
    struct _pipe2    *h_owner;
    int                 h_n; /* number of atoms in h_list */
    t_atom              *h_atoms; /* pointer to a list of h_n t_atoms */
}t_hang;

typedef struct _pipe2{
    t_object    x_obj;
    float       x_deltime;
    t_outlet    *x_pipe2out;
    t_hang      *x_hang;
}t_pipe2;

static void *pipe2_new(t_symbol *s, int argc, t_atom *argv);
static void pipe2_hang_free(t_hang *h);
static void pipe2_hang_tick(t_hang *h);
static void pipe2_any_hang_tick(t_hang *h);
static void pipe2_list(t_pipe2 *x, t_symbol *s, int ac, t_atom *av);
static void pipe2_anything(t_pipe2 *x, t_symbol *s, int ac, t_atom *av);
void pipe2_setup(void);

static void pipe2_hang_tick(t_hang *h){
    t_pipe2  *x = h->h_owner;
    t_hang      *h2, *h3;
    /* excise h from the linked list of hangs */
    if (x->x_hang == h) x->x_hang = h->h_next;
    else for (h2 = x->x_hang; ((h3 = h2->h_next)!=NULL); h2 = h3){
        if (h3 == h){
            h2->h_next = h3->h_next;
            break;
        }
    }
    /* output h's list */
    outlet_list(x->x_pipe2out, &s_list, h->h_n, h->h_atoms);
    /* free h */
    pipe2_hang_free(h);
}

static void pipe2_any_hang_tick(t_hang *h){
    t_pipe2  *x = h->h_owner;
    t_hang      *h2, *h3;
    /* excise h from the linked list of hangs */
    if (x->x_hang == h) x->x_hang = h->h_next;
    else for (h2 = x->x_hang; ((h3 = h2->h_next)!=NULL); h2 = h3){
        if (h3 == h){
            h2->h_next = h3->h_next;
            break;
        }
    }
    /* output h's atoms */
    outlet_anything(x->x_pipe2out, h->h_atoms[0].a_w.w_symbol, h->h_n-1, &h->h_atoms[1]);
    /* free h */
    pipe2_hang_free(h);
}

static void pipe2_list(t_pipe2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if (x->x_deltime > 0){ // if delay is real, save the list for output in delay milliseconds
        t_hang *h;
        int i;
        h = (t_hang *)getbytes(sizeof(t_hang));
        h->h_n = ac;
        h->h_atoms = (t_atom *)getbytes(h->h_n*sizeof(t_atom));
        for(i = 0; i < h->h_n; ++i)
            h->h_atoms[i] = av[i];
        h->h_next = x->x_hang;
        x->x_hang = h;
        h->h_owner = x;
        h->h_clock = clock_new(h, (t_method)pipe2_hang_tick);
        clock_delay(h->h_clock, (x->x_deltime >= 0 ? x->x_deltime : 0));
    }
    else // otherwise just pass the list straight through
        outlet_list(x->x_pipe2out, &s_list, ac, av);
}

static void pipe2_anything(t_pipe2 *x, t_symbol *s, int ac, t_atom *av){
    if (x->x_deltime > 0){ // if delay is real, save the list for output in delay milliseconds
        t_hang *h;
        int i;
        h = (t_hang *)getbytes(sizeof(t_hang));
        h->h_n = ac+1;
        h->h_atoms = (t_atom *)getbytes(h->h_n*sizeof(t_atom));
        SETSYMBOL(&h->h_atoms[0], s);
        for(i = 1; i < h->h_n; ++i)
            h->h_atoms[i] = av[i-1];
        h->h_next = x->x_hang;
        x->x_hang = h;
        h->h_owner = x;
        h->h_clock = clock_new(h, (t_method)pipe2_any_hang_tick);
        clock_delay(h->h_clock, (x->x_deltime >= 0 ? x->x_deltime : 0));
    }
    else  // otherwise just pass it straight through
        outlet_anything(x->x_pipe2out, s, ac, av);
}

static void *pipe2_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_pipe2  *x = (t_pipe2 *)pd_new(pipe2_class);
    float       deltime;
    if (argc){ /* We accept one argument to set the delay time, ignore any further args */
        if (argv[0].a_type != A_FLOAT){
            char stupid[80];
            atom_string(&argv[argc-1], stupid, 79);
            post("pipe2: %s: bad time delay value", stupid);
            deltime = 0;
        }
        else
            deltime = argv[argc-1].a_w.w_float;
    }
    else
        deltime = 0;
    x->x_pipe2out = outlet_new(&x->x_obj, &s_list);
    floatinlet_new(&x->x_obj, &x->x_deltime);
    x->x_hang = NULL;
    x->x_deltime = deltime;
    return (x);
}

static void pipe2_hang_free(t_hang *h){
    freebytes(h->h_atoms, h->h_n*sizeof(t_atom));
    clock_free(h->h_clock);
    freebytes(h, sizeof(t_hang));
}

static void pipe2_free(t_pipe2 *x){
    t_hang *hang;
    while ((hang = x->x_hang) != NULL){
        x->x_hang = hang->h_next;
        pipe2_hang_free(hang);
    }
}

void pipe2_setup(void){
    pipe2_class = class_new(gensym("pipe2"), (t_newmethod)pipe2_new,
        (t_method)pipe2_free, sizeof(t_pipe2), 0, A_GIMME, 0);
    class_addlist(pipe2_class, pipe2_list);
    class_addanything(pipe2_class, pipe2_anything);
}
