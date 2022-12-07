/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


//Derek Kwan 2016 - adding separator attribute, redoing tosymbol_separator

#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define TOSYMBOL_INISTRING   128  /* LATER rethink */
#define TOSYMBOL_MAXSTRING  2048  /* the refman says so, later CHECKME */
static char tosymbol_defseparator[] = " ";

typedef struct _tosymbol
{
    t_object   x_ob;
    t_symbol  *x_separator;
    int        x_bufsize;
    char      *x_buffer;
    char       x_bufini[TOSYMBOL_INISTRING];
    int        x_entered;
} t_tosymbol;

static t_class *tosymbol_class;
static char tosymbol_buffer[TOSYMBOL_MAXSTRING];
static int tosymbol_bufferlocked = 0;
/* The idea is to prevent two different tosymbol objects from using the static
   buffer at the same time.  In the current scenario this buffer is never used
   for output, so this lock is unnecessary... but it does no harm either... */

static void tosymbol_flushbuffer(t_tosymbol *x)
{
    if (*x->x_buffer)
    {
	x->x_entered = 1;
	outlet_symbol(((t_object *)x)->ob_outlet, gensym(x->x_buffer));
	x->x_entered = 0;
    }
}

static void tosymbol_bang(t_tosymbol *x)
{
    outlet_bang(((t_object *)x)->ob_outlet);  /* CHECKED */
}

static void tosymbol_float(t_tosymbol *x, t_float f)
{
    if (!x->x_entered)
    {
	sprintf(x->x_buffer, "%g", f);
	tosymbol_flushbuffer(x);
    }
}

static void tosymbol_symbol(t_tosymbol *x, t_symbol *s)
{
    outlet_symbol(((t_object *)x)->ob_outlet, s);
}

static void tosymbol_pointer(t_tosymbol *x, t_gpointer *gp)
{
    /* nop: otherwise gpointer would be converted to 'list <gp>' */
}

static int tosymbol_parse(t_symbol *s, int ac, t_atom *av, t_symbol *separator,
			  int bufsize, char *buffer)
{
    int nleft = bufsize - 1;
    int len;
    char *bp = buffer;
    bp[0] = bp[nleft] = 0;
    if (s)
	strncpy(bp, s->s_name, nleft);
    len = strlen(bp);
    nleft -= len;
    bp += len;
    if (ac && nleft > 0)
    {
	char *sepstring = (separator ?
			   separator->s_name : tosymbol_defseparator);
	while (ac--)
	{
	    if (*sepstring && bp > buffer)
	    {
		strncpy(bp, sepstring, nleft);
		len = strlen(bp);
		nleft -= len;
		if (nleft <= 0) break;
		bp += len;
	    }
	    /* LATER rethink: type-checking */
	    atom_string(av, bp, nleft);
	    len = strlen(bp);
	    nleft -= len;
	    bp += len;
	    if (nleft <= 0) break;
	    av++;
	}
    }
    if (nleft < 0)
    {
	post("bug [tosymbol]: tosymbol_parse");
	return (bufsize);
    }
    return (bufsize - nleft);
}

static void tosymbol_anything(t_tosymbol *x, t_symbol *s, int ac, t_atom *av)
{
    if (!x->x_entered)
    {
	if (tosymbol_bufferlocked)
	{
        pd_error(x, "bug [tosymbol]: tosymbol_anything");
	    tosymbol_parse(s, ac, av, x->x_separator,
			   x->x_bufsize, x->x_buffer);
	}
	else
	{
	    int ntotal;
	    tosymbol_bufferlocked = 1;
	    ntotal = tosymbol_parse(s, ac, av, x->x_separator,
				    TOSYMBOL_MAXSTRING, tosymbol_buffer);
	    if (ntotal > x->x_bufsize)
	    {
		int newtotal = ntotal;
		x->x_buffer = grow_nodata(&newtotal, &x->x_bufsize, x->x_buffer,
					  TOSYMBOL_INISTRING, x->x_bufini,
					  sizeof(*x->x_buffer));
		if (newtotal < ntotal)
		{
		    ntotal = newtotal - 1;
		    x->x_buffer[ntotal] = 0;
		}
	    }
	    memcpy(x->x_buffer, tosymbol_buffer, ntotal);
	    tosymbol_bufferlocked = 0;
	}
	tosymbol_flushbuffer(x);
    }
}

static void tosymbol_separator(t_tosymbol *x, t_symbol *s, int argc, t_atom * argv)
{
    //hack to make it detect " " - Derek Kwan
    int numq = 0; //number of quotes in a row, if mod 2 == 0 and  > 0, count as space
    int set = 0; //if separator is set
    while(argc){
        t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
        if(strcmp(cursym->s_name, "@separator")!=0){
            x->x_separator = cursym;
            set = 1;
            };
        if(strcmp(cursym->s_name, "\"\0") == 0 || strcmp(cursym->s_name, "'\0") == 0){
            numq++;
        };
        argc--;
        argv++;
    };
    if(numq > 0 && numq % 2 == 0){
        //if follows quote conditions
        x->x_separator = gensym(" ");
    }
    else if(!set){
        //removes all spaces if not set
        x->x_separator = gensym("");;
    };
}


static void tosymbol_list(t_tosymbol *x, t_symbol *s, int ac, t_atom *av)
{
    tosymbol_anything(x, 0, ac, av);
}


static void tosymbol_free(t_tosymbol *x)
{
    if (x->x_buffer != x->x_bufini)
	freebytes(x->x_buffer, x->x_bufsize);
}

static void *tosymbol_new(t_symbol *s, int argc, t_atom *argv)
{
    t_tosymbol *x = (t_tosymbol *)pd_new(tosymbol_class);

     
    if(argc >= 1){
        t_symbol * curarg = atom_getsymbolarg(0, argc, argv);
        if(strcmp(curarg->s_name, "@separator") == 0){
            argc--;
            argv++;
            tosymbol_separator(x, 0, argc, argv);
        }
        else{
            goto errstate;
        };

    }
    else{
        x->x_separator = 0;
    };

    
    x->x_bufsize = TOSYMBOL_INISTRING;
    x->x_buffer = x->x_bufini;
    x->x_entered = 0;
    outlet_new((t_object *)x, &s_symbol);
    return (x);
	errstate:
		pd_error(x, "tosymbol: improper args");
		return NULL;
}

CYCLONE_OBJ_API void tosymbol_setup(void)
{
    tosymbol_class = class_new(gensym("tosymbol"),
			       (t_newmethod)tosymbol_new,
			       (t_method)tosymbol_free,
			       sizeof(t_tosymbol), 0, A_GIMME, 0);
    class_addbang(tosymbol_class, tosymbol_bang);
    class_addfloat(tosymbol_class, tosymbol_float);
    class_addsymbol(tosymbol_class, tosymbol_symbol);
    class_addpointer(tosymbol_class, tosymbol_pointer);
    class_addlist(tosymbol_class, tosymbol_list);
    class_addanything(tosymbol_class, tosymbol_anything);
    class_addmethod(tosymbol_class, (t_method)tosymbol_separator,
		    gensym("separator"), A_GIMME, 0);
}
