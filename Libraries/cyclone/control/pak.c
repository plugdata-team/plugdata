/*
 // adapted from code by Pierre Guillot to conform 
 // with Max/MSP fullest compatibility as possible.
*/

#include <m_pd.h>
#include <common/api.h>

static t_class *pak_class;
static t_class* pak_inlet_class;

struct _pak_inlet;

typedef struct _pak
{
    t_object            x_obj;
    t_int               x_n;
    t_atom*             x_vec;
    t_atom*             x_out;
    struct _pak_inlet*  x_ins;
} t_pak;

typedef struct _pak_inlet
{
    t_class*    x_pd;
    t_atom*     x_atoms;
    t_int       x_max;
    t_int       x_int;
    t_pak*      x_owner;
    int         x_idx; //index of inlet
} t_pak_inlet;


static void *pak_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_pak *x = (t_pak *)pd_new(pak_class);
    t_atom defarg[2];
    int noargs = !argc; //go to defaults
    if(!argc)
    {
        argv = defarg;
        argc = 2;
        SETFLOAT(&defarg[0], 0);
        SETFLOAT(&defarg[1], 0);
    }
    
    x->x_n   = argc;
    x->x_vec = (t_atom *)getbytes(argc * sizeof(*x->x_vec));
    x->x_out = (t_atom *)getbytes(argc * sizeof(*x->x_out));
    x->x_ins = (t_pak_inlet *)getbytes(argc * sizeof(*x->x_ins));
    
    for(i = 0; i < x->x_n; ++i)
    {
        if(argv[i].a_type == A_FLOAT)
        {
            x->x_vec[i].a_type      = A_FLOAT;
            x->x_vec[i].a_w.w_float = argv[i].a_w.w_float;
            x->x_ins[i].x_pd    = pak_inlet_class;
            x->x_ins[i].x_atoms = x->x_vec+i;
            x->x_ins[i].x_max   = x->x_n-i;
            x->x_ins[i].x_owner = x;
            x->x_ins[i].x_idx = i;
            if(noargs){
                //should default to int
                x->x_ins[i].x_int = 1;
            }
            else{

                x->x_ins[i].x_int = 0;
            }
            inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
        }
        else if(argv[i].a_type == A_SYMBOL)
        {
        if(argv[i].a_w.w_symbol == gensym("f")) // only "f" arg converts to float
            {
                x->x_vec[i].a_type      = A_FLOAT;
                x->x_vec[i].a_w.w_float = 0.f;
                x->x_ins[i].x_pd    = pak_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                x->x_ins[i].x_int = 0;
                x->x_ins[i].x_idx = i;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
                
            }
        else if(argv[i].a_w.w_symbol == gensym("i")) // only "i" arg should be integer
            {
                x->x_vec[i].a_type      = A_FLOAT;
                x->x_vec[i].a_w.w_float = 0.f;
                x->x_ins[i].x_pd    = pak_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                x->x_ins[i].x_int = 1;
                x->x_ins[i].x_int = i;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
            else
            {
                x->x_vec[i].a_type       = A_SYMBOL;
                x->x_vec[i].a_w.w_symbol = argv[i].a_w.w_symbol; // loads symbol from arg
                x->x_ins[i].x_pd    = pak_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                x->x_ins[i].x_int = 0;
                x->x_ins[i].x_idx = i;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
        }
    }
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void pak_bang(t_pak *x)
{
    int i;
    for(i = 0; i < x->x_n; ++i)
    {
        x->x_out[i] = x->x_vec[i];
    }
    outlet_list(x->x_obj.ob_outlet, &s_list, x->x_n, x->x_out);
}

static void pak_copy(t_pak *x, int ndest, t_atom* dest, int nsrc, t_atom* src, int srcidx)
{
    int i, isint;
    for(i = 0; i < ndest && i < nsrc; ++i)
    {
        if(src[i].a_type == A_FLOAT)
        {
            if(dest[i].a_type == A_FLOAT)
            {   
                //if current inlet is an int
                isint = x->x_ins[srcidx+i].x_int;
                if(isint){
                    dest[i].a_w.w_float = (int)src[i].a_w.w_float;
                }
                else{
                    dest[i].a_w.w_float = src[i].a_w.w_float;
                };
            }
            else if(dest[i].a_type == A_SYMBOL)
            {
                dest[i].a_w.w_symbol = &s_; // float becomes blank symbol
            }
        }
        else if(src[i].a_type == A_SYMBOL)
        {
            if(dest[i].a_type == A_SYMBOL)
            {
                dest[i].a_w.w_symbol = src[i].a_w.w_symbol;
            }
            else if(dest[i].a_type == A_FLOAT)
            {
            dest[i].a_w.w_float = 0; // symbol becomes 0
            }
        }
    }
}

static void pak_free(t_pak *x)
{
    freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
    freebytes(x->x_out, x->x_n * sizeof(*x->x_out));
    freebytes(x->x_ins, x->x_n * sizeof(*x->x_ins));
}


static void pak_inlet_bang(t_pak_inlet *x)
{
    pak_bang(x->x_owner);
}

static void pak_inlet_float(t_pak_inlet *x, float f)
{
    if(x->x_int)
    {
        x->x_atoms->a_w.w_float = (int)f;
        pak_bang(x->x_owner);
    }
    else if(x->x_atoms->a_type == A_FLOAT)
    {
        x->x_atoms->a_w.w_float = f;
        pak_bang(x->x_owner);
    }
    else if(x->x_atoms->a_type == A_SYMBOL)
    {
        x->x_atoms->a_w.w_symbol =  &s_; // float becomes blank symbol
        pak_bang(x->x_owner);
    }
}

static void pak_inlet_symbol(t_pak_inlet *x, t_symbol* s)
{
    if(x->x_atoms->a_type == A_SYMBOL)
    {
        x->x_atoms->a_w.w_symbol = s;
        pak_bang(x->x_owner);
    }
    else if(x->x_atoms->a_type == A_FLOAT)
    {
        x->x_atoms->a_w.w_float = 0; // symbol becomes 0
        pak_bang(x->x_owner);
    }
}

static void pak_inlet_list(t_pak_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    pak_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv, x->x_idx);
    pak_bang(x->x_owner);
}

static void pak_inlet_anything(t_pak_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->x_atoms[0].a_type == A_SYMBOL)
    {
        x->x_atoms[0].a_w.w_symbol = s;
    }
    else if (x->x_atoms[0].a_type == A_FLOAT)
    {
        x->x_atoms[0].a_w.w_float = 0; // symbol becomes 0
    }
    pak_copy(x->x_owner, x->x_max-1, x->x_atoms+1, argc, argv, x->x_idx);
    pak_bang(x->x_owner);
}

static void pak_inlet_set(t_pak_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    pak_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv, x->x_idx);
}

CYCLONE_OBJ_API extern void pak_setup(void)
{
    t_class* c = NULL;
    
    c = class_new(gensym("pak-inlet"), 0, 0, sizeof(t_pak_inlet), CLASS_PD, 0);
    if(c)
    {
        class_addbang(c,    (t_method)pak_inlet_bang);
        class_addfloat(c,   (t_method)pak_inlet_float);
        class_addsymbol(c,  (t_method)pak_inlet_symbol);
        class_addlist(c,    (t_method)pak_inlet_list);
        class_addanything(c,(t_method)pak_inlet_anything);
        class_addmethod(c,  (t_method)pak_inlet_set, gensym("set"), A_GIMME, 0);
    }
    pak_inlet_class = c;
    
    c = class_new(gensym("pak"), (t_newmethod)pak_new, (t_method)pak_free, sizeof(t_pak), CLASS_NOINLET, A_GIMME, 0);
    pak_class = c;
}
