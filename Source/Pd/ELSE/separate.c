// based on zexy/symbol2list

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <s_utf8.h>

typedef struct _separate{
    t_object    x_obj;
    t_symbol   *separator;
    t_atom     *av;
    int         ac;
    int         arg_n; // reserved atoms (might be > ac)
}t_separate;

static t_class *separate_class;

static void set_separator(t_separate *x, t_symbol *s){
    if(s == gensym("empty")){
        x->separator = NULL;
        return;
    }
    else
        x->separator = s;
}

static int ishex(const char *s){
    s++;
    return(*s == 'x' || *s == 'X');
}

static void string2atom(t_separate *x, t_atom *ap, char* cp, int clen){
    char *buf = getbytes(sizeof(char)*(clen+1));
    char *endptr[1];
    strncpy(buf, cp, clen);
    buf[clen] = 0;
    t_float ftest = strtod(buf, endptr);
    if(buf+clen != *endptr){ // strtof() failed, we have a symbol
        SETSYMBOL(ap, gensym(buf));
    }
    else{ // probably a number
        if(ishex(buf)) // test for hexadecimal (inf/nan are still floats)
            SETSYMBOL(ap, gensym(buf));
        else if(gensym(buf) == gensym("")) // if symbol ends with separator
            x->ac--;
        else
            SETFLOAT(ap, ftest);
    }
    freebytes(buf, sizeof(char)*(clen+1));
}

static void separate_process(t_separate *x, t_symbol *s){
    char *cc = (char*)s->s_name;
    char *cp = cc;
    if(x->separator == NULL || x->separator == gensym("")){
        int len = strlen(cc); // BUG
        if(x->arg_n < len){ // resize if necessary
            freebytes(x->av, x->arg_n *sizeof(t_atom));
            x->arg_n = len+10;
            x->av = getbytes(x->arg_n *sizeof(t_atom));
        }
        int total_chars = 0;
        for(int i = 0; i < len; i++){
            unsigned char c = s->s_name[i];
            if(c < 128 || c > 191)
                total_chars++;
        }
        x->ac = total_chars;
        int nchars = 1;
        while(len--){
            unsigned char c = s->s_name[len];
            if(c < 128 || c > 191){
                string2atom(x, x->av+total_chars-1, cc+len, nchars);
                nchars = 1;
                total_chars--;
            }
            else
                nchars++;
        }
        return;
    }
    int i = 1;
    char *deli = (char*)x->separator->s_name;
    int dell = strlen(deli);
    char *d;
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
    // parse the items into the list-buffer
    i = 0;
    // find the first item
    cp = cc;
    while(cp == (d = strstr(cp,deli)))
        cp += dell;
    while((d = strstr(cp, deli))){
        if(d != cp){
            string2atom(x, x->av+i, cp, d-cp);
            i++;
        }
        cp = d+dell;
    }
    if(cp)
        string2atom(x, x->av+i, cp, strlen(cp));
}

static void separate_symbol(t_separate *x, t_symbol *s){
    if(!s || s == gensym("") || s == NULL){
        x->ac = 0;
        outlet_bang(x->x_obj.ob_outlet);
    }
    else{
       separate_process(x, s);
        if(x->ac)
            outlet_list(x->x_obj.ob_outlet, &s_list, x->ac, x->av);
    }
}

static void *separate_new(t_symbol *s, int ac, t_atom *av){
    t_separate *x = (t_separate *)pd_new(separate_class);
    s = NULL;
    x->ac = 0;
    x->arg_n = 16;
    x->av = getbytes(x->arg_n *sizeof(t_atom));
    x->separator =  gensym(" "); //
    if(ac){
        if(av->a_type == A_SYMBOL)
            set_separator(x, atom_getsymbol(av));
        else if(av->a_type == A_FLOAT){
            char buf[256];
            sprintf(buf, "%g", atom_getfloat(av));
            set_separator(x, gensym(buf));
        }
    }
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym(""));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void separate_setup(void){
    separate_class = class_new(gensym("separate"), (t_newmethod)separate_new,
        0, sizeof(t_separate), 0, A_GIMME, 0);
    class_addsymbol(separate_class, separate_symbol);
    class_addmethod(separate_class, (t_method)set_separator, gensym(""), A_SYMBOL, 0);
}
