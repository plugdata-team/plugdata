/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include <common/api.h>

/* Pattern types.  These are the parsing routine's return values.
   If returned value is >= SPRINTF_MINSLOTTYPE, then another slot
   is created (i.e. an inlet, and a proxy handling it). */
#define SPRINTF_UNSUPPORTED  0
#define SPRINTF_LITERAL      1
#define SPRINTF_MINSLOTTYPE  2
#define SPRINTF_INT          2
#define SPRINTF_FLOAT        3
#define SPRINTF_CHAR         4
#define SPRINTF_STRING       5

/* Numbers:  assuming max 62 digits preceding a decimal point in any
   fixed-point representation of a t_float (39 in my system)
   -- need to be sure, that using max precision would never produce
   a representation longer than max width.  If this is so, then no number
   representation would exceed max width (presumably...).
   Strings:  for the time being, any string longer than max width would
   be truncated (somehow compatible with Str256, but LATER warn-and-allow). */
/* LATER rethink it all */
#define SPRINTF_MAXPRECISION  192
#define SPRINTF_MAXWIDTH      256

typedef struct _sprintf
{
    t_object  x_ob;
    int       x_nslots;
    int       x_nproxies;  /* as requested (and allocated) */
    t_pd    **x_proxies;
    int       x_fsize;     /* as allocated (i.e. including a terminating 0) */
    char     *x_fstring;
    int       x_symout;
} t_sprintf;

typedef struct _sprintf_proxy
{
    t_object    p_ob;
    t_sprintf  *p_master;
    int         p_id;
    int         p_type;  /* a value #defined above */
    char       *p_pattern;
    char       *p_pattend;
    t_atom      p_atom;  /* current input */
    int         p_size;
    int         p_valid;
} t_sprintf_proxy;

static t_class *sprintf_class;
static t_class *sprintf_proxy_class;

/* CHECKED: 'symout' argument has no special meaning in max4.07,
   LATER investigate */

/* LATER use snprintf, if it is available on other systems (should be...) */
static void sprintf_proxy_checkit(t_sprintf_proxy *x, char *buf, int checkin)
{
    int result = 0, valid = 0;
    char *pattend = x->p_pattend;
    if (pattend)
    {
	char tmp = *pattend;
	*pattend = 0;
	if (x->p_atom.a_type == A_FLOAT)
	{
	    t_float f = x->p_atom.a_w.w_float;
	    if (x->p_type == SPRINTF_INT)
		/* CHECKME large/negative values */
		result = sprintf(buf, x->p_pattern, (long)f);
	    else if (x->p_type == SPRINTF_FLOAT)
		result = sprintf(buf, x->p_pattern, f);
	    else if (x->p_type == SPRINTF_CHAR)
		/* CHECKED: if 0 is input into a %c-slot, the whole output
		   string is null-terminated */
		/* CHECKED: float into a %c-slot is truncated,
		   but CHECKME large/negative values */
		result = sprintf(buf, x->p_pattern, (unsigned char)f);
	    else if (x->p_type == SPRINTF_STRING)
	    {
		/* CHECKED: any number input into a %s-slot is ok */
		char tmp[64];  /* LATER rethink */
		sprintf(tmp, "%g", f);
		result = sprintf(buf, x->p_pattern, tmp);
	    }
	    else pd_error(x, "sprintf: can't convert float to type of argument %d", x->p_id + 1);
	    if (result > 0)
		valid = 1;
	}
	else if (x->p_atom.a_type == A_SYMBOL)
	{
	    t_symbol *s = x->p_atom.a_w.w_symbol;
	    if (x->p_type == SPRINTF_STRING)
	    {
		if (strlen(s->s_name) > SPRINTF_MAXWIDTH)
		{
		    strncpy(buf, s->s_name, SPRINTF_MAXWIDTH);
		    buf[SPRINTF_MAXWIDTH] = 0;
		    result = SPRINTF_MAXWIDTH;
		}
		else result = sprintf(buf, x->p_pattern, s->s_name);
		if (result >= 0)
		    valid = 1;
	    }
        else pd_error(x, "sprintf: can't convert symbol to type of argument %d", x->p_id + 1);
	}
	*pattend = tmp;
    }
    else pd_error(x, "sprintf_proxy_checkit");
    if (x->p_valid = valid)
    {
	x->p_size = result;
    }
    else
    {
	x->p_size = 0;
    }
}

static void sprintf_dooutput(t_sprintf *x)
{
    int i, outsize;
    char *outstring;
    outsize = x->x_fsize;  /* this is strlen() + 1 */
    /* LATER consider subtracting format pattern sizes */
    for (i = 0; i < x->x_nslots; i++)
    {
	t_sprintf_proxy *y = (t_sprintf_proxy *)x->x_proxies[i];
	if (y->p_valid)
	    outsize += y->p_size;
	else
	{
	    /* slot i has received an invalid input -- CHECKME if this
	       condition blocks all subsequent output requests? */
	    return;
	}
    }
    if (outsize > 0 && (outstring = getbytes(outsize)))
    {
	char *inp = x->x_fstring;
	char *outp = outstring;
	for (i = 0; i < x->x_nslots; i++)
	{
	    t_sprintf_proxy *y = (t_sprintf_proxy *)x->x_proxies[i];
	    int len = y->p_pattern - inp;
	    if (len > 0)
	    {
		strncpy(outp, inp, len);
		outp += len;
	    }
	    sprintf_proxy_checkit(y, outp, 0);
	    outp += y->p_size;  /* p_size is never negative */
	    inp = y->p_pattend;
	}
	strcpy(outp, inp);

	outp = outstring;
	if(x->x_symout == 1) outlet_symbol(((t_object *) x)->ob_outlet, gensym(outstring));
	else
	  {
	    while (*outp == ' ' || *outp == '\t'
		   || *outp == '\n' || *outp == '\r') outp++;
	    if (*outp)
	      {
		t_binbuf *bb = binbuf_new();
		int ac;
		t_atom *av;
		binbuf_text(bb, outp, strlen(outp));
		ac = binbuf_getnatom(bb);
		av = binbuf_getvec(bb);
		if (ac)
		  {
		    if (av->a_type == A_SYMBOL)
		      outlet_anything(((t_object *)x)->ob_outlet,
				      av->a_w.w_symbol, ac - 1, av + 1);
		    else if (av->a_type == A_FLOAT)
		      {
			if (ac > 1)
			  outlet_list(((t_object *)x)->ob_outlet,
				      &s_list, ac, av);
			else
			  outlet_float(((t_object *)x)->ob_outlet,
				       av->a_w.w_float);
		      }
		  }
		binbuf_free(bb);
	      };
	  };
	freebytes(outstring, outsize);
    }
}

static void sprintf_proxy_bang(t_sprintf_proxy *x)
{
  if(x->p_id == 0) sprintf_dooutput(x->p_master);  /* CHECKED (in any inlet) */
  else
    pd_error(x, "sprintf: can't convert bang to type of argument %d", (x->p_id) + 1);
}

static void sprintf_proxy_float(t_sprintf_proxy *x, t_float f)
{
    char buf[SPRINTF_MAXWIDTH + 1];  /* LATER rethink */
    SETFLOAT(&x->p_atom, f);
    sprintf_proxy_checkit(x, buf, 1);
    if (x->p_id == 0 && x->p_valid)
	sprintf_dooutput(x->p_master);  /* CHECKED: only first inlet */
}

static void sprintf_proxy_symbol(t_sprintf_proxy *x, t_symbol *s)
{
    char buf[SPRINTF_MAXWIDTH + 1];  /* LATER rethink */
    if (s && *s->s_name)
	SETSYMBOL(&x->p_atom, s);
    else
	SETFLOAT(&x->p_atom, 0);
    sprintf_proxy_checkit(x, buf, 1);
    if (x->p_id == 0 && x->p_valid)
	sprintf_dooutput(x->p_master);  /* CHECKED: only first inlet */
}

static void sprintf_dolist(t_sprintf *x,
			   t_symbol *s, int ac, t_atom *av, int startid)
{
    int cnt = x->x_nslots - startid;
    if (ac > cnt)
	ac = cnt;
    if (ac-- > 0)
    {
	int id;
	for (id = startid + ac, av += ac; id >= startid; id--, av--)
	{
	    if (av->a_type == A_FLOAT)
		sprintf_proxy_float((t_sprintf_proxy *)x->x_proxies[id],
				    av->a_w.w_float);
	    else if (av->a_type == A_SYMBOL)
		sprintf_proxy_symbol((t_sprintf_proxy *)x->x_proxies[id],
				     av->a_w.w_symbol);
	}
    }
}

static void sprintf_doanything(t_sprintf *x,
			       t_symbol *s, int ac, t_atom *av, int startid)
{
    if (s && s != &s_)
    {
	sprintf_dolist(x, 0, ac, av, startid + 1);
	sprintf_proxy_symbol((t_sprintf_proxy *)x->x_proxies[startid], s);
    }
    else sprintf_dolist(x, 0, ac, av, startid);
}

static void sprintf_proxy_list(t_sprintf_proxy *x,
			       t_symbol *s, int ac, t_atom *av)
{
    sprintf_dolist(x->p_master, s, ac, av, x->p_id);
}

static void sprintf_proxy_anything(t_sprintf_proxy *x,
				   t_symbol *s, int ac, t_atom *av)
{
    sprintf_doanything(x->p_master, s, ac, av, x->p_id);
}

static void sprintf_bang(t_sprintf *x)
{
    if (x->x_nslots)
	sprintf_proxy_bang((t_sprintf_proxy *)x->x_proxies[0]);
    else if(x->x_fsize >= 2) outlet_symbol(((t_object *) x)->ob_outlet, gensym(x->x_fstring));
    else
      pd_error(x, "sprintf: no arguments given");
}

static void sprintf_float(t_sprintf *x, t_float f)
{
    if (x->x_nslots)
	sprintf_proxy_float((t_sprintf_proxy *)x->x_proxies[0], f);
    else
      pd_error(x, "sprintf: can't convert float to type of argument 1");
}

static void sprintf_symbol(t_sprintf *x, t_symbol *s)
{
    if (x->x_nslots)
	sprintf_proxy_symbol((t_sprintf_proxy *)x->x_proxies[0], s);
    else
      pd_error(x, "sprintf: can't convert symbol to type of argument 1");
}

static void sprintf_list(t_sprintf *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_nslots)
	sprintf_dolist(x, s, ac, av, 0);
    else
      pd_error(x, "sprintf: can't convert list to type of argument 1");
}

static void sprintf_anything(t_sprintf *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_nslots)
	sprintf_doanything(x, s, ac, av, 0);
    else
      pd_error(x, "sprintf: can't convert anything to type of argument 1");
}

/* adjusted binbuf_gettext(), LATER do it right */
static char *sprintf_gettext(int ac, t_atom *av, int *sizep)
{
    char *buf = getbytes(1);
    int size = 1;
    char atomtext[MAXPDSTRING];
    while (ac--)
    {
	char *newbuf;
    	int newsize;
    	if (buf[size-1] == 0 || av->a_type == A_SEMI || av->a_type == A_COMMA)
	    size--;
    	atom_string(av, atomtext, MAXPDSTRING);
    	newsize = size + strlen(atomtext) + 1;
    	if (!(newbuf = resizebytes(buf, size, newsize)))
	{
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

/* Called twice:  1st pass (with x == 0) is used for counting valid patterns;
   2nd pass (after object allocation) -- for initializing the proxies.
   If there is a "%%" pattern, then the buffer is shrunk in the second pass
   (LATER rethink). */
static int sprintf_parsepattern(t_sprintf *x, char **patternp)
{
    int type = SPRINTF_UNSUPPORTED;
    char errstring[MAXPDSTRING];
    char *ptr;
    char modifier = 0;
    int width = 0;
    int precision = 0;
    int *numfield = &width;
    int dotseen = 0;
    *errstring = 0;
    for (ptr = *patternp; *ptr; ptr++)
        {
        if (*ptr >= '0' && *ptr <= '9')
            {
            if (!numfield)
                {
                if (x) sprintf(errstring, "extra number field");
                    break;
                }
            *numfield = 10 * *numfield + *ptr - '0';
            if (dotseen)
                {
                if (precision > SPRINTF_MAXPRECISION)
                    {
                    if (x) sprintf(errstring, "precision field too large");
                        break;
                    }
                }
            else
                {
                if (width > SPRINTF_MAXWIDTH)
                    {
                    if (x) sprintf(errstring, "width field too large");
                        break;
                    }
                }
            continue;
            }
        if (*numfield)
            numfield = 0;
        if (strchr("diouxX", *ptr))
            {
            type = SPRINTF_INT;
            break;
            }
        else if (strchr("eEfFgG", *ptr))
            {
/*            if (modifier)
                {
                if (x) sprintf(errstring, "\'%c\' modifier not supported", modifier);
                    break;
                }*/
            type = SPRINTF_FLOAT;
            break;
            }
        else if (strchr("c", *ptr))
            {
            if (modifier)
                {
                if (x) sprintf(errstring, "\'%c\' modifier not supported", modifier);
                    break;
                }
            type = SPRINTF_CHAR;
            break;
            }
        else if (strchr("s", *ptr))
            {
            if (modifier)
                {
                if (x) sprintf(errstring, "\'%c\' modifier not supported", modifier);
                    break;
                }
            type = SPRINTF_STRING;
            break;
            }
        else if (*ptr == '%')
            {
            type = SPRINTF_LITERAL;
            if (x)
                {  /* buffer-shrinking hack, LATER rethink */
                char *p1 = ptr, *p2 = ptr + 1;
                do
                    *p1++ = *p2;
                    while (*p2++);
                        ptr--;
                }
            break;
            }
        else if (strchr("CSnm", *ptr))
            {
            if (x) sprintf(errstring, "\'%c\' type not supported", *ptr);
                break;
            }
        else if (strchr("l", *ptr))
            {
            if (modifier)
                {
                if (x) sprintf(errstring, "only single modifier is supported");
                    break;
                }
            modifier = *ptr;
            }
/* added this for new formats but it did not work (porres)
        else if(strchr("L", *ptr)){
                if(modifier){
                if(x)
                    sprintf(errstring, "only single modifier is supported");
                break;
            }
            modifier = *ptr;
        }*/
        else if (strchr("h", *ptr))
        {
            if (modifier)
            {
                if (x) sprintf(errstring, "only single modifier is supported");
                break;
            }
            modifier = *ptr;
        }
        else if (strchr("aAhjLqtzZ", *ptr))
            {
            if (x) sprintf(errstring, "\'%c\' modifier not supported", *ptr);
                break;
            }
        else if (*ptr == '.')
            {
            if (dotseen)
                {
                if (x) sprintf(errstring, "multiple dots");
                    break;
                }
            numfield = &precision;
            dotseen = 1;
            }
        else if (*ptr == '$')
            {
            if (x) sprintf(errstring, "parameter number field not supported");
                break;
            }
        else if (*ptr == '*')
            {
            if (x) sprintf(errstring, "%s parameter not supported", (dotseen ? "precision" : "width"));
                break;
            }
        else if (!strchr("-+ #\'", *ptr))
            {
            if (x) sprintf(errstring, "\'%c\' format character not supported", *ptr);
                break;
            }
        }
    if (*ptr)
        ptr++;  /* LATER rethink */
    else
        if (x) sprintf(errstring, "type not specified");
            if (x && type == SPRINTF_UNSUPPORTED)
            {
            if (*errstring)
                pd_error(x, "[sprintf]: slot skipped (%s %s)", errstring, "in a format pattern");
            else
                pd_error(x, "[sprintf]: slot skipped");
            }
    *patternp = ptr;
    return (type);
}

static void sprintf_free(t_sprintf *x)
{
    if (x->x_proxies)
    {
	int i = x->x_nslots;
	while (i--)
	{
	    t_sprintf_proxy *y = (t_sprintf_proxy *)x->x_proxies[i];
	    pd_free((t_pd *)y);
	}
	freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
    if (x->x_fstring)
	freebytes(x->x_fstring, x->x_fsize);
}

static void *sprintf_new(t_symbol *s, int ac, t_atom *av)
{
    t_sprintf *x;
    int fsize;
    char *fstring;
    char *p1, *p2;
    int i, symout = 0, nslots, nproxies = 0;
    t_pd **proxies;
    if(ac){
      if(av->a_type == A_SYMBOL)
	{
	  t_symbol * curav = atom_getsymbolarg(0, ac, av);
	  if(strcmp(curav->s_name, "symout")  == 0)
	    {
	      symout = 1;
	      ac--;
	      av++;
	    };
	};
    };
    fstring = sprintf_gettext(ac, av, &fsize);
    p1 = fstring;

    
    while (p2 = strchr(p1, '%'))
        {
        int type;
        p1 = p2 + 1;
        type = sprintf_parsepattern(0, &p1);
        if (type >= SPRINTF_MINSLOTTYPE) nproxies++;
        }
    if (!nproxies) 	// object without arguments doesn't create in max
        {           // in cyclone it creates without an outlet
        x = (t_sprintf *)pd_new(sprintf_class);
        x->x_nslots = 0;
        x->x_nproxies = 0;
        x->x_proxies = 0;
        x->x_fsize = fsize;
	x->x_symout = symout;
        x->x_fstring = fstring;
        p1 = fstring;
        while (p2 = strchr(p1, '%'))
            {
            p1 = p2 + 1;
            sprintf_parsepattern(x, &p1);
            };
	outlet_new((t_object *)x, &s_anything);
        return (x);
        }

    /* CHECKED: max creates as many inlets, as there are %-signs, no matter
       if they are valid, or not -- if not, it prints ``can't convert'' errors
       for any input... */
    
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies))))
        {
        freebytes(fstring, fsize);
        return (0);
        }
    for (nslots = 0; nslots < nproxies; nslots++)
	if (!(proxies[nslots] = pd_new(sprintf_proxy_class))) break;
    if (!nslots)
        {
        freebytes(fstring, fsize);
        freebytes(proxies, nproxies * sizeof(*proxies));
        return (0);
        }
    x = (t_sprintf *)pd_new(sprintf_class);
    x->x_nslots = nslots;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    x->x_fsize = fsize;
    x->x_symout = symout;
    x->x_fstring = fstring;
    p1 = fstring;
    i = 0;
    while (p2 = strchr(p1, '%'))
        {
        int type;
        p1 = p2 + 1;
        type = sprintf_parsepattern(x, &p1);
        if (type >= SPRINTF_MINSLOTTYPE)
            {
            if (i < nslots)
                {
                char buf[SPRINTF_MAXWIDTH + 1];  /* LATER rethink */
                t_sprintf_proxy *y = (t_sprintf_proxy *)proxies[i];
                y->p_master = x;
                y->p_id = i;
                y->p_type = type;
                y->p_pattern = p2;
                y->p_pattend = p1;
                if (type == SPRINTF_STRING)
                    SETSYMBOL(&y->p_atom, &s_);
                else
                    SETFLOAT(&y->p_atom, 0);
                y->p_size = 0;
                y->p_valid = 0;
                if (i) inlet_new((t_object *)x, (t_pd *)y, 0, 0);
                sprintf_proxy_checkit(y, buf, 1);
                i++;
                }
            }
        }
    outlet_new((t_object *)x, &s_anything);
    return (x);
}

CYCLONE_OBJ_API void sprintf_setup(void)
{
    sprintf_class = class_new(gensym("sprintf"),
			      (t_newmethod)sprintf_new,
			      (t_method)sprintf_free,
			      sizeof(t_sprintf), 0, A_GIMME, 0);
    class_addbang(sprintf_class, sprintf_bang);
    class_addfloat(sprintf_class, sprintf_float);
    class_addsymbol(sprintf_class, sprintf_symbol);
    class_addlist(sprintf_class, sprintf_list);
    class_addanything(sprintf_class, sprintf_anything);
    sprintf_proxy_class = class_new(gensym("_sprintf_proxy"), 0, 0,
				    sizeof(t_sprintf_proxy),
				    CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(sprintf_proxy_class, sprintf_proxy_bang);
    class_addfloat(sprintf_proxy_class, sprintf_proxy_float);
    class_addsymbol(sprintf_proxy_class, sprintf_proxy_symbol);
    class_addlist(sprintf_proxy_class, sprintf_proxy_list);
    class_addanything(sprintf_proxy_class, sprintf_proxy_anything);
}
