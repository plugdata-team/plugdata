// based on vanilla's array

#include "m_pd.h"
#include "g_canvas.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _WIN32
#include <io.h>
#endif

#ifdef _WIN32
# include <malloc.h> /* MSVC or mingw on windows */
#elif defined(__linux__) || defined(__APPLE__)
# include <alloca.h> /* linux, mac, mingw, cygwin */
#else
# include <stdlib.h> /* BSDs for example */
#endif

#ifndef HAVE_ALLOCA     /* can work without alloca() but we never need it */
#define HAVE_ALLOCA 1
#endif
#define TEXT_NGETBYTE 100 /* bigger that this we use alloc, not alloca */
#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < TEXT_NGETBYTE ?  \
        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
    ((n) < TEXT_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

typedef struct _array_client{
    t_object    tc_obj;
    t_symbol   *tc_sym;
    t_symbol   *tc_struct;
    t_symbol   *tc_field;
    t_canvas   *tc_canvas;
}t_array_client;

static t_array *array_client_getbuf(t_array_client *x, t_glist **glist){
    if(x->tc_sym){
        t_garray *y = (t_garray *)pd_findbyclass(x->tc_sym, garray_class);
        if(y){
            *glist = garray_getglist(y);
            return(garray_getarray(y));
        }
        else{
            pd_error(x, "[buffer]: couldn't find named array '%s'", x->tc_sym->s_name);
            *glist = 0;
            return(0);
        }
    }
    else
        return(0);
}

// ------  buffer class -----
static t_class *buffer_class;
static t_class *buffer_proxy_class;

typedef struct _buffer_proxy{
    t_pd              p_pd;
    struct _buffer  *p_owner;
}t_buffer_proxy;

typedef struct _buffer{
    t_array_client  x_tc;
    t_buffer_proxy x_proxy;
}t_buffer;

static void buffer_proxy_init(t_buffer_proxy * p, t_buffer *x){
    p->p_pd = buffer_proxy_class;
    p->p_owner = x;
}

static void *buffer_new(t_symbol *s){
    t_buffer *x = (t_buffer *)pd_new(buffer_class);
    x->x_tc.tc_sym = (s && s != &s_) ? s : 0;
    buffer_proxy_init(&x->x_proxy, x);
    inlet_new(&x->x_tc.tc_obj, &x->x_proxy.p_pd, 0, 0);
    outlet_new(&x->x_tc.tc_obj, &s_float);
    return(x);
}

// ----------------  methods -------------------
static int buffer_getrange(t_buffer *x, char **firstitemp, int *nitemp, int *stridep, int *arrayonsetp){
    t_glist *glist;
    t_array *a = array_client_getbuf(&x->x_tc, &glist);
    int stride, fieldonset, arrayonset, nitem, type;
    t_symbol *arraytype;
    t_template *template;
    if(!a)
        return (0);
    t_symbol *elemfield = gensym("y");
    template = template_findbyname(a->a_templatesym);
    if(!template_find_field(template, elemfield, &fieldonset, &type, &arraytype) || type != DT_FLOAT){
        pd_error(x, "can't find field %s in struct %s", elemfield->s_name, a->a_templatesym->s_name);
        return(0);
    }
    stride = a->a_elemsize;
    arrayonset = 0;
    if(arrayonset < 0)
        arrayonset = 0;
    else if(arrayonset > a->a_n)
        arrayonset = a->a_n;
    nitem = a->a_n;
    *firstitemp = a->a_vec+(fieldonset+arrayonset*stride);
    *nitemp = nitem;
    *stridep = stride;
    *arrayonsetp = arrayonset;
    return(1);
}

static void buffer_bang(t_buffer *x){
    char *itemp, *firstitem;
    int stride, nitem, arrayonset, i;
    t_atom *outv;
    if(!buffer_getrange(x, &firstitem, &nitem, &stride, &arrayonset))
        return;
    ATOMS_ALLOCA(outv, nitem);
    for(i = 0, itemp = firstitem; i < nitem; i++, itemp += stride)
        SETFLOAT(&outv[i],  *(t_float *)itemp);
    outlet_list(x->x_tc.tc_obj.ob_outlet, 0, nitem, outv);
    ATOMS_FREEA(outv, nitem);
}

static void buffer_name(t_buffer *x, t_symbol *s){
    x->x_tc.tc_sym = s;
}

static void buffer_resize(t_buffer *x, t_floatarg f){
    t_glist *glist;
    t_array *a = array_client_getbuf(&x->x_tc, &glist);
    if(a){
        if(x->x_tc.tc_sym){
            t_garray *y = (t_garray *)pd_findbyclass(x->x_tc.tc_sym, garray_class);
            if(!y){
                pd_error(x, "no such array '%s'", x->x_tc.tc_sym->s_name);
                return;
            }
            garray_resize(y, f);
        }
        else{
            int n = f;
            if(n < 1)
                n = 1;
             array_resize_and_redraw(a, glist, n);
        }
    }
}

static void buffer_set(t_buffer *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac <= 1)
        return;
    char *itemp, *firstitem;
    int stride, nitem, arrayonset, i;
    if(!buffer_getrange(x, &firstitem, &nitem, &stride, &arrayonset))
        return;
    int n = atom_getfloatarg(0, ac, av);
    if(n < 0 || n >= nitem){
        post("[buffer]: index out of range");
        return;
    }
    for(i = 0, itemp = firstitem; i < n; i++)
        itemp += stride;
    for(i = 1; i < ac; i++, itemp += stride, n++){
        if(n >= nitem)
            break;
        *(t_float *)itemp = atom_getfloatarg(i, ac, av);
    }
    t_glist *glist = 0;
    t_array *a = array_client_getbuf(&x->x_tc, &glist);
    if(glist_isvisible(glist))
        array_redraw(a, glist);
}

static void buffer_setarray(t_buffer *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    buffer_resize(x, argc);
    char *itemp, *firstitem;
    int stride, nitem, arrayonset, i;
    if(!buffer_getrange(x, &firstitem, &nitem, &stride, &arrayonset))
        return;
    if(nitem > argc)
        nitem = argc;
    for(i = 0, itemp = firstitem; i < nitem; i++, itemp += stride)
        *(t_float *)itemp = atom_getfloatarg(i, argc, argv);
    t_glist *glist = 0;
    t_array *a = array_client_getbuf(&x->x_tc, &glist);
    if(glist)
        array_redraw(a, glist);
}

static void buffer_list(t_buffer *x, t_symbol *s, int ac, t_atom *av){
    buffer_setarray(x, s, ac, av);
    outlet_list(x->x_tc.tc_obj.ob_outlet, 0, ac, av);
}

static void buffer_proxy_list(t_buffer_proxy *p, t_symbol *s, int ac, t_atom *av){
    t_buffer *x = p->p_owner;
    buffer_setarray(x, s, ac, av);
}

void buffer_setup(void){
    buffer_class = class_new(gensym("buffer"), (t_newmethod)buffer_new, 0, sizeof(t_buffer), 0, A_DEFSYMBOL, 0);
    class_addbang(buffer_class, buffer_bang);
    class_addlist(buffer_class, buffer_list);
    class_addmethod(buffer_class, (t_method)buffer_set, gensym("set"), A_GIMME, 0);
    class_addmethod(buffer_class, (t_method)buffer_name, gensym("name"), A_DEFSYMBOL, 0);
    buffer_proxy_class = (t_class *)class_new(gensym("buffer proxy"), 0, 0, sizeof(t_buffer_proxy), 0, 0);
    class_addlist(buffer_proxy_class, buffer_proxy_list);
}
