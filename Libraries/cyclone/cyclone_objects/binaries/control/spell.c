/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdio.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _spell{
    t_object  x_ob;
    int       x_minsize;
    int       x_padchar;  /* actually, any nonnegative integer (CHECKED) */
}t_spell;

static t_class *spell_class;

static void spell_fill(t_spell *x, int cnt){
    for (; cnt < x->x_minsize; cnt++)
	outlet_float(((t_object *)x)->ob_outlet, x->x_padchar);
}

/* CHECKED: chars are spelled as signed */
static int spell_out(t_spell *x, char *ptr, int flush)
{
    int cnt = 0;
    while (*ptr)
	outlet_float(((t_object *)x)->ob_outlet, *ptr++), cnt++;
    if (flush)
    {
	spell_fill(x, cnt);
	return (0);
    }
    return (cnt);
}

static void spell_bang(t_spell *x){
    // ignore...
}

static void spell_float(t_spell *x, t_float f)
{
    int i = (int)f;
    if(f == i){  /* CHECKED */
        char buf[16];
        sprintf(buf, "%d", i);  /* CHECKED (negative numbers) */
        spell_out(x, buf, 1);
    }
    else
        post("[spell] doesn't understand \"non integer floats\"");
}

/* CHECKED: 'symbol' selector is not spelled! */
static void spell_symbol(t_spell *x, t_symbol *s)
{
    spell_out(x, s->s_name, 1);
}

static void spell_list(t_spell *x, t_symbol *s, int ac, t_atom *av){
    int cnt = 0;
    int addsep = 0;
    while (ac--){
        if (addsep){
            outlet_float(((t_object *)x)->ob_outlet, x->x_padchar);
            cnt++;
        }
        else
            addsep = 1;
        if (av->a_type == A_FLOAT){
            float f = av->a_w.w_float;
            int i = (int)f;
            if (f == i){
                char buf[16];
                sprintf(buf, "%d", i);  /* CHECKED (negative numbers) */
                cnt += spell_out(x, buf, 0);
            } // else: ignore
        }
        /* CHECKED: symbols as empty strings (separator is added) */
        av++;
    }
    if(cnt)  /* CHECKED: empty list is silently ignored */
        spell_fill(x, cnt);
}

static void spell_anything(t_spell *x, t_symbol *s, int ac, t_atom *av)
{
    int cnt = 0;
    int addsep = 0;
    if (s)
    {
	cnt += spell_out(x, s->s_name, 0);
	addsep = 1;
    }
    while (ac--)
    {
	if (addsep)
	{
	    outlet_float(((t_object *)x)->ob_outlet, x->x_padchar);
	    cnt++;
	}
	else addsep = 1;
	if (av->a_type == A_FLOAT){
        float f = av->a_w.w_float;
        int i = (int)f;
        if (f == i){
            char buf[16];
            sprintf(buf, "%d", i);  /* CHECKED (negative numbers) */
            cnt += spell_out(x, buf, 0);
	    } // else: ignore
	}
	else if (av->a_type == A_SYMBOL && av->a_w.w_symbol)
	    cnt += spell_out(x, av->a_w.w_symbol->s_name, 0);
	av++;
    }
    if (cnt)  /* CHECKED: empty list is silently ignored */
	spell_fill(x, cnt);
}

static void *spell_new(t_floatarg f1, t_floatarg f2)
{
    t_spell *x = (t_spell *)pd_new(spell_class);
    int i2 = (int)f2;  /* CHECKED */
    x->x_minsize = (f1 > 0 ? (int)f1 : 0);
    x->x_padchar = (i2 < 0 ? 0 : (i2 > 0 ? i2 : ' '));  /* CHECKED */
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void spell_setup(void)
{
    spell_class = class_new(gensym("spell"),
			    (t_newmethod)spell_new, 0,
			    sizeof(t_spell), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(spell_class, spell_bang);
    class_addfloat(spell_class, spell_float);
    class_addsymbol(spell_class, spell_symbol);
    class_addlist(spell_class, spell_list);
    class_addanything(spell_class, spell_anything);
}
