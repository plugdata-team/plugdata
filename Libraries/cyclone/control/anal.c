/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include <string.h>

#define ANAL_DEFSIZE  128
#define ANAL_MAXSIZE  16384

typedef struct _anal{
    t_object  x_ob;
    int       x_value;  /* negative, if initialized or reset */
    int       x_size;
    int       x_bytesize;
    int      *x_table;
    /* CHECKED: the 'weight' count is a signed short (2 bytes),
       wrapping into negative domain without any warning.
       LATER consider complaining at == SHRT_MAX. */
} t_anal;

static t_class *anal_class;

static void anal_reset(t_anal *x){
    memset(x->x_table, 0, x->x_bytesize);
    x->x_value = -1;
}

static void anal_clear(t_anal *x){
    memset(x->x_table, 0, x->x_bytesize);
}

static void anal_float(t_anal *x, t_float f){
    int value = (int)f;
    if(f == value){  /* CHECKED */
        /* CHECKED: any input negative or >= size is ignored with an error
         -- there is no output, and a previous state is kept. */
        if (value >= 0 && value < x->x_size){
            if (x->x_value >= 0){
                t_atom at[3];
                int ndx = x->x_value * x->x_size + value;
                x->x_table[ndx]++;
                SETFLOAT(at, x->x_value);
                SETFLOAT(&at[1], value);
                SETFLOAT(&at[2], x->x_table[ndx]);
                outlet_list(((t_object *)x)->ob_outlet, &s_list, 3, at);
            }
            x->x_value = value;
        }
        else
            pd_error(x, "[anal]: %d outside of table bounds", value);
    }
    else{
        pd_error(x, "[anal]: doesn't understand \"non integer floats\"");
    }
}

static void anal_free(t_anal *x){
    if (x->x_table)
        freebytes(x->x_table, x->x_bytesize);
}

static void *anal_new(t_floatarg f){
    t_anal *x;
    int size = (int)f;
    int bytesize;
    int *table;
    if (size < 1)
	/* CHECKED: 1 is allowed (such an object rejects any nonzero input) */
	size = ANAL_DEFSIZE;
    else if (size > ANAL_MAXSIZE){ /* CHECKED: */
        pd_error(x, "[anal]: size too large, using %d", ANAL_MAXSIZE);
        size = ANAL_MAXSIZE;  /* LATER switch into a 'sparse' mode */
    } /* CHECKED: actually the bytesize is size * size * sizeof(short),
       and it shows up in */
    bytesize = size * size * sizeof(*table);
    if (!(table = (int *)getbytes(bytesize)))
        return (0);
    x = (t_anal *)pd_new(anal_class);
    x->x_size = size;
    x->x_bytesize = bytesize;
    x->x_table = table;
    outlet_new((t_object *)x, &s_list);
    anal_reset(x);
    anal_clear(x);
    return (x);
}

CYCLONE_OBJ_API void anal_setup(void){
    anal_class = class_new(gensym("anal"), (t_newmethod)anal_new,
            (t_method)anal_free, sizeof(t_anal), 0, A_DEFFLOAT, 0);
    class_addfloat(anal_class, anal_float);
    class_addmethod(anal_class, (t_method)anal_reset, gensym("reset"), 0);
    class_addmethod(anal_class, (t_method)anal_clear, gensym("clear"), 0);
}
