
#include <stdio.h>
#include <string.h>
#include "m_pd.h"

// Pattern types
#define format_LITERAL      1
#define format_MINSLOTTYPE  2
#define format_INT          2
#define format_FLOAT        3
#define format_CHAR         4
#define format_STRING       5

/* Numbers:  assuming max 62 digits preceding a decimal point in any
 fixed-point representation of a t_float (39 in my system)
 -- need to be sure, that using max precision would never produce
 a representation longer than max width.  If this is so, then no number
 representation would exceed max width (presumably...).
 Strings:  for the time being, any string longer than max width would
 be truncated (somehow compatible with Str256, but LATER warn-and-allow). */
/* LATER rethink it all */
#define format_MAXPRECISION  192
#define format_MAXWIDTH      512

typedef struct _format{
    t_object  x_ob;
    int       x_nslots;
    int       x_nproxies;  // as requested (and allocated)
    t_pd    **x_proxies;
    int       x_fsize;     // as allocated (i.e. including a terminating 0)
    char     *x_fstring;
}t_format;

typedef struct _format_proxy{
    t_object        p_ob;
    t_format   *p_master;
    int             p_id;
    int             p_type;  // value #defined above
    char           *p_pattern;
    char           *p_pattend;
    t_atom          p_atom;  // input
    int             p_size;
    int             p_valid;
}t_format_proxy;

static t_class *format_class;
static t_class *format_proxy_class;

// LATER: use snprintf (should be available on all systems)
static void format_proxy_checkit(t_format_proxy *x, char *buf){
    int result = 0, valid = 0;
    char *pattend = x->p_pattend;
    if(pattend){
        char tmp = *pattend;
        *pattend = 0;
        if(x->p_atom.a_type == A_FLOAT){
            t_float f = x->p_atom.a_w.w_float;
            if(x->p_type == format_INT) // CHECKME large/negative values
                result = sprintf(buf, x->p_pattern, (long)f);
            else if(x->p_type == format_FLOAT)
                result = sprintf(buf, x->p_pattern, f);
            else if(x->p_type == format_CHAR) // a 0 input into %c nulls output
    // float into %c is truncated, but CHECKME large/negative values
                result = sprintf(buf, x->p_pattern, (unsigned char)f);
            else if(x->p_type == format_STRING){ // a float into %s is ok
                char temp[64];  // rethink
                sprintf(temp, "%g", f);
                result = sprintf(buf, x->p_pattern, temp);
            }
            else
                pd_error(x, "[format]: can't convert float (this shouldn't happen)");
            if(result > 0)
                valid = 1;
        }
        else if (x->p_atom.a_type == A_SYMBOL){
            t_symbol *s = x->p_atom.a_w.w_symbol;
            if(x->p_type == format_STRING){
                if (strlen(s->s_name) > format_MAXWIDTH){
                    strncpy(buf, s->s_name, format_MAXWIDTH);
                    buf[format_MAXWIDTH] = 0;
                    result = format_MAXWIDTH;
                }
                else result = sprintf(buf, x->p_pattern, s->s_name);
                if (result >= 0)
                    valid = 1;
            }
            else
                pd_error(x, "[format]: can't convert (type mismatch)");
        }
        *pattend = tmp;
    }
    else
        pd_error(x, "format_proxy_checkit"); // ????
    if((x->p_valid = valid))
        x->p_size = result;
    else
        x->p_size = 0;
}

static void format_dooutput(t_format *x){
    int i, outsize;
    char *outstring;
    outsize = x->x_fsize;  // strlen() + 1
    // LATER consider subtracting format pattern sizes
    for(i = 0; i < x->x_nslots; i++){
        t_format_proxy *y = (t_format_proxy *)x->x_proxies[i];
        if (y->p_valid)
            outsize += y->p_size;
        else // invalid input
            return;
    }
    if(outsize > 0 && (outstring = getbytes(outsize))){
        char *inp = x->x_fstring;
        char *outp = outstring;
        for(i = 0; i < x->x_nslots; i++){
            t_format_proxy *y = (t_format_proxy *)x->x_proxies[i];
            int len = y->p_pattern - inp;
            if(len > 0){
                strncpy(outp, inp, len);
                outp += len;
            }
            format_proxy_checkit(y, outp);
            outp += y->p_size;  /* p_size is never negative */
            inp = y->p_pattend;
        }
        strcpy(outp, inp);
        outp = outstring;
        while(*outp == ' ' || *outp == '\t' || *outp == '\n' || *outp == '\r')
            outp++;
        if(*outp){
            t_binbuf *bb = binbuf_new();
            int ac;
            t_atom *av;
            binbuf_text(bb, outp, strlen(outp));
            ac = binbuf_getnatom(bb);
            av = binbuf_getvec(bb);
            if(ac){
                if(av->a_type == A_SYMBOL)
                  outlet_anything(((t_object *)x)->ob_outlet, av->a_w.w_symbol, ac - 1, av + 1);
                else if(av->a_type == A_FLOAT){
                    if(ac > 1)
                        outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
                    else
                        outlet_float(((t_object *)x)->ob_outlet, av->a_w.w_float);
                }
            }
            binbuf_free(bb);
        };
        freebytes(outstring, outsize);
    }
}

static void format_proxy_bang(t_format_proxy *x){
    pd_error(x, "[format]: no method for bang in secondary inlets");
}

static void format_proxy_float(t_format_proxy *x, t_float f){
    char buf[format_MAXWIDTH + 1];  /* LATER rethink */
    SETFLOAT(&x->p_atom, f);
    format_proxy_checkit(x, buf);
    if (x->p_id == 0 && x->p_valid)
        format_dooutput(x->p_master);  // CHECKED: only first inlet
}

static void format_proxy_symbol(t_format_proxy *x, t_symbol *s){
    char buf[format_MAXWIDTH + 1];  // LATER rethink
    if (s && *s->s_name)
        SETSYMBOL(&x->p_atom, s);
    else
        SETFLOAT(&x->p_atom, 0);
    format_proxy_checkit(x, buf);
    if (x->p_id == 0 && x->p_valid)
        format_dooutput(x->p_master);  // CHECKED: only first inlet
}

static void format_dolist(t_format *x, t_symbol *s, int ac, t_atom *av, int startid){
    t_symbol *dummy = s;
    dummy = NULL;
    int cnt = x->x_nslots - startid;
    if(ac > cnt)
        ac = cnt;
    if(ac-- > 0){
        int id;
        for(id = startid + ac, av += ac; id >= startid; id--, av--){
            if(av->a_type == A_FLOAT)
                format_proxy_float((t_format_proxy *)x->x_proxies[id], av->a_w.w_float);
            else if(av->a_type == A_SYMBOL)
                format_proxy_symbol((t_format_proxy *)x->x_proxies[id], av->a_w.w_symbol);
        }
    }
}

static void format_doanything(t_format *x, t_symbol *s, int ac, t_atom *av, int startid){
    if(s && s != &s_){
        format_dolist(x, 0, ac, av, startid + 1);
        format_proxy_symbol((t_format_proxy *)x->x_proxies[startid], s);
    }
    else
        format_dolist(x, 0, ac, av, startid);
}

static void format_proxy_list(t_format_proxy *x, t_symbol *s, int ac, t_atom *av){
    format_dolist(x->p_master, s, ac, av, x->p_id);
}

static void format_proxy_anything(t_format_proxy *x, t_symbol *s, int ac, t_atom *av){
    format_doanything(x->p_master, s, ac, av, x->p_id);
}

static void format_bang(t_format *x){
    if(x->x_nslots)
        format_dooutput(x);
    else
        pd_error(x, "[format]: no variable arguments given");
}

static void format_float(t_format *x, t_float f){
    if(x->x_nslots)
        format_proxy_float((t_format_proxy *)x->x_proxies[0], f);
    else
        pd_error(x, "[format]: no variable arguments given");
}

static void format_symbol(t_format *x, t_symbol *s){
    if(x->x_nslots)
        format_proxy_symbol((t_format_proxy *)x->x_proxies[0], s);
    else
        pd_error(x, "[format]: no variable arguments given");
}

static void format_list(t_format *x, t_symbol *s, int ac, t_atom *av){
    if(x->x_nslots)
        format_dolist(x, s, ac, av, 0);
    else
        pd_error(x, "[format]: no variable arguments given");
}

static void format_anything(t_format *x, t_symbol *s, int ac, t_atom *av){
    if (x->x_nslots)
        format_doanything(x, s, ac, av, 0);
    else
        pd_error(x, "[format]: no variable arguments given");
}

// adjusted binbuf_gettext(), LATER do it right
static char *makename_getstring(int ac, t_atom *av, int *sizep){
    char *buf = getbytes(1);
    int size = 1;
    char atomtext[MAXPDSTRING];
    while (ac--){
        char *newbuf;
        int newsize;
        if (buf[size-1] == 0 || av->a_type == A_SEMI || av->a_type == A_COMMA)
            size--;
        atom_string(av, atomtext, MAXPDSTRING);
        newsize = size + strlen(atomtext) + 1;
        if (!(newbuf = resizebytes(buf, size, newsize))){
            *sizep = 1;
            return (getbytes(1));
        }
        buf = newbuf;
        strcpy(buf + size, atomtext);
        size = newsize;
        buf[size-1] = ' ';
        av++;
    }
    buf[size-1] = 0;
    *sizep = size;
    return (buf);
}

// used 2x, 1st (x==0) counts valid patterns, 2nd inits proxies and shrinks %%
static int format_get_type(t_format *x, char **patternp){
    int type = 0;
    char errstring[MAXPDSTRING];
    char *ptr;
//    char modifier = 0;
    int width = 0;
    int precision = 0;
    int *numfield = &width;
    int dotseen = 0;
    *errstring = 0;
    for(ptr = *patternp; *ptr; ptr++){
        if(*ptr >= '0' && *ptr <= '9'){
            if(!numfield){
                if(x)
                    sprintf(errstring, "extra number field");
                break;
            }
            *numfield = 10 * *numfield + *ptr - '0';
            if(dotseen){
                if(precision > format_MAXPRECISION){
                    if(x)
                        sprintf(errstring, "precision field too large");
                    break;
                }
            }
            else{
                if(width > format_MAXWIDTH){
                    if(x)
                        sprintf(errstring, "width field too large");
                    break;
                }
            }
            continue;
        }
        if(*numfield)
            numfield = 0;
        if(strchr("pdiouxX", *ptr)){ // INT
            type = format_INT;
            break;
        }
        else if(strchr("eEfFgG", *ptr)){ // FLOAT
            // needed to include if(modifier) to prevent %lf and stuff
            type = format_FLOAT;
            break;
        }
        else if(strchr("c", *ptr)){ // CHAR
            type = format_CHAR;
            break;
        }
        else if(strchr("s", *ptr)){
            type = format_STRING;
            break;
        }
        else if(*ptr == '%'){
            type = format_LITERAL;
            if(x){ // buffer-shrinking hack
                char *p1 = ptr, *p2 = ptr + 1;
                do
                    *p1++ = *p2;
                while (*p2++);
                ptr--;
            }
            break;
        }
/*        else if(strchr("l", *ptr)){ // remove longs for now
            if(modifier){
                if(x)
                    sprintf(errstring, "only single modifier is supported");
                break;
            }
            modifier = *ptr;
        }*/
        else if(*ptr == '.'){
            if(dotseen){
                if(x)
                    sprintf(errstring, "multiple dots");
                break;
            }
            numfield = &precision;
            dotseen = 1;
        }
        else if(*ptr == '$'){
            if(x)
                sprintf(errstring, "parameter number field not supported");
            break;
        }
        else if(*ptr == '*'){
            if(x)
                sprintf(errstring, "%s parameter not supported", (dotseen ? "precision" : "width"));
            break;
        }
        else if(!strchr("-+ #\'", *ptr)){
            if (x)
                sprintf(errstring, "\'%c\' format character not supported", *ptr);
            break;
        }
    }
    if(*ptr)
        ptr++; // LATER rethink
    else
        if(x)
            sprintf(errstring, "type not specified");
    *patternp = ptr;
    return(type);
}

static void format_free(t_format *x){
    if (x->x_proxies){
        int i = x->x_nslots;
        while (i--){
            t_format_proxy *y = (t_format_proxy *)x->x_proxies[i];
            pd_free((t_pd *)y);
        }
        freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
    if (x->x_fstring)
        freebytes(x->x_fstring, x->x_fsize);
}

static void *format_new(t_symbol *s, int ac, t_atom *av){
    t_symbol *dummy = s;
    dummy = NULL;
    t_format *x;
    int fsize;
    char *fstring;
    char *p1, *p2;
    int i = 1, nslots, nproxies = 0;
    t_pd **proxies;
    fstring = makename_getstring(ac, av, &fsize);
    p1 = fstring;
    while((p2 = strchr(p1, '%'))){
        int type;
        p1 = p2 + 1;
        type = format_get_type(0, &p1);
        if(type >= format_MINSLOTTYPE)
            nproxies++;
    }
    if(!nproxies){ // no arguments creates with an inlet and prints errors
        x = (t_format *)pd_new(format_class);
        x->x_nslots = 0;
        x->x_nproxies = 0;
        x->x_proxies = 0;
        x->x_fsize = fsize;
        x->x_fstring = fstring;
        p1 = fstring;
        while ((p2 = strchr(p1, '%'))){
            p1 = p2 + 1;
            format_get_type(x, &p1);
        };
        outlet_new((t_object *)x, &s_symbol);
        return (x);
    }
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies)))){
        freebytes(fstring, fsize);
        return (0);
    }
    for (nslots = 0; nslots < nproxies; nslots++)
        if (!(proxies[nslots] = pd_new(format_proxy_class))) break;
    if (!nslots){
        freebytes(fstring, fsize);
        freebytes(proxies, nproxies * sizeof(*proxies));
        return (0);
    }
    x = (t_format *)pd_new(format_class);
    x->x_nslots = nslots;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    x->x_fsize = fsize;
    x->x_fstring = fstring;
    p1 = fstring;
    i = 0;
    while((p2 = strchr(p1, '%'))){
        int type;
        p1 = p2 + 1;
        type = format_get_type(x, &p1);
        if(type >= format_MINSLOTTYPE){
            if(i < nslots){
                char buf[format_MAXWIDTH + 1];  // LATER rethink
                t_format_proxy *y = (t_format_proxy *)proxies[i];
                y->p_master = x;
                y->p_id = i;
                y->p_type = type;
                y->p_pattern = p2;
                y->p_pattend = p1;
                if(type == format_STRING)
                    SETSYMBOL(&y->p_atom, &s_);
                else
                    SETFLOAT(&y->p_atom, 0);
                y->p_size = 0;
                y->p_valid = 0;
                if(i) // creates inlets for valid '%'
                    inlet_new((t_object *)x, (t_pd *)y, 0, 0);
                format_proxy_checkit(y, buf);
                i++;
            }
        }
    }
    outlet_new((t_object *)x, &s_symbol);
    return(x);
}

void format_setup(void){
    format_class = class_new(gensym("format"), (t_newmethod)format_new,
        (t_method)format_free, sizeof(t_format), 0, A_GIMME, 0);
    class_addbang(format_class, format_bang);
    class_addfloat(format_class, format_float);
    class_addsymbol(format_class, format_symbol);
    class_addlist(format_class, format_list);
    class_addanything(format_class, format_anything);
    format_proxy_class = class_new(gensym("_format_proxy"), 0, 0,
        sizeof(t_format_proxy), CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(format_proxy_class, format_proxy_bang);
    class_addfloat(format_proxy_class, format_proxy_float);
    class_addsymbol(format_proxy_class, format_proxy_symbol);
    class_addlist(format_proxy_class, format_proxy_list);
    class_addanything(format_proxy_class, format_proxy_anything);
}

