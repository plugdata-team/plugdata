
#include "m_pd.h"
#include <stdlib.h>
#include <string.h>

typedef struct _symbol2any{
    t_object    x_obj;
    t_symbol   *x_separator;
    t_symbol   *x_symbol;
    t_atom     *av;
    int         ac;
    int         arg_n; // reserved atoms (might be > ac)
}t_symbol2any;

static t_class *symbol2any_class;

// helper ---------------------------------------------------------------------------
static int ishex(const char *s){
    s++;
    if(*s == 'x' || *s == 'X')
        return(1);
    else
        return(0);
}

static void string2atom(t_atom *ap, char* cp, int clen){
    char *buffer = getbytes(sizeof(char)*(clen+1));
    char *endptr[1];
    t_float ftest;
    strncpy(buffer, cp, clen);
    buffer[clen] = 0;
    ftest = strtod(buffer, endptr);
    if(buffer+clen != *endptr) // strtof() failed, we have a symbol
        SETSYMBOL(ap, gensym(buffer));
    else{ // probably a number, let's test for hexadecimal (inf/nan are still floats)
        if(ishex(buffer))
            SETSYMBOL(ap, gensym(buffer));
        else
            SETFLOAT(ap, ftest);
    }
    freebytes(buffer, sizeof(char)*(clen+1));
}

static void symbol2any_process(t_symbol2any *x, t_symbol *s){
    char *cc;
    char *deli;
    int   dell;
    char *cp, *d;
    int i = 1;
    cc = (char*)s->s_name;
    cp = cc;
    deli = (char*)x->x_separator->s_name;
    dell = strlen(deli);
    while((d = strstr(cp, deli))){ // get the number of items
        if(d != NULL && d != cp)
            i++;
        cp = d+dell;
    }
    if(x->arg_n < i){ // resize if necessary
        freebytes(x->av, x->arg_n *sizeof(t_atom));
        x->arg_n = i+10;
        x->av = getbytes(x->arg_n *sizeof(t_atom));
    }
    x->ac = i;
    /* parse the items into the list-buffer */
    i = 0;
    /* find the first item */
    cp = cc;
    while(cp == (d = strstr(cp,deli)))
        cp += dell;
    while((d = strstr(cp, deli))){
        if(d != cp){
            string2atom(x->av+i, cp, d-cp);
            i++;
        }
        cp = d+dell;
    }
    if(cp)
        string2atom(x->av+i, cp, strlen(cp));
}
// helper ---------------------------------------------------------------------------

static void symbol2any_output(t_symbol2any *x){
    if(x->av->a_type == A_FLOAT){
        if(x->ac == 1)
            outlet_float(x->x_obj.ob_outlet, atom_getfloatarg(0, x->ac, x->av));
        else
            outlet_list(x->x_obj.ob_outlet, &s_list, x->ac, x->av);
    }
    else{
        t_symbol *s = atom_getsymbolarg(0, x->ac, x->av);
        outlet_anything(x->x_obj.ob_outlet, s, x->ac-1, x->av+1);
    }
}

static void symbol2any_symbol(t_symbol2any *x, t_symbol *s){
    x->x_symbol = s;
    if(!s || s == gensym(""))
        outlet_bang(x->x_obj.ob_outlet);
    else{
        symbol2any_process(x, s);
        if(x->ac)
            symbol2any_output(x);
    }
}

static void symbol2any_bang(t_symbol2any *x){
    if(!x->x_symbol || x->x_symbol == gensym(""))
        outlet_bang(x->x_obj.ob_outlet);
    else if(x->ac)
        symbol2any_output(x);
}

static void *symbol2any_new(void){
    t_symbol2any *x = (t_symbol2any *)pd_new(symbol2any_class);
    x->ac = 0;
    x->arg_n = 16;
    x->av = getbytes(x->arg_n *sizeof(t_atom));
    x->x_separator =  gensym(" "); //
    outlet_new(&x->x_obj, 0);
    return(x);
}

void symbol2any_setup(void){
    symbol2any_class = class_new(gensym("symbol2any"), (t_newmethod)symbol2any_new,
        0, sizeof(t_symbol2any), 0, 0);
    class_addbang(symbol2any_class, symbol2any_bang);
    class_addsymbol(symbol2any_class, symbol2any_symbol);
}
