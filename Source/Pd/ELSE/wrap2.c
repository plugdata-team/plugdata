// Porres 2016
 
#include "m_pd.h"
#include <math.h>

static t_class *wrap2_class;

typedef struct _wrap2
{
    t_object    x_obj;
    t_int       x_bytes;
    t_atom      *output_list;
    t_outlet    *x_outlet;
    t_float     x_f;
    t_float     x_min;
    t_float     x_max;
} t_wrap2;

static t_float convert(t_float f, t_float min, t_float max)
{
    float result;
    if(min > max)
    { // swap values
        float temp;
        temp = max;
        max = min;
        min = temp;
    };
    if(min == max)
        result = min;
    else if(f < max && f >= min)
        result = f; // if f range, = in
    else
    { // wrap
        float range = max - min;
        if(f < min)
        {
            result = f;
            while(result < min)
            {
                result += range;
            };
        }
        else
            result = fmod(f - min, range) + min;
    }
    return result;
}

static void wrap2_float(t_wrap2 *x, t_floatarg f)
{
  x->x_f = f;
  outlet_float(x->x_outlet, convert(f, x->x_min, x->x_max));
}

static void wrap2_list(t_wrap2 *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->x_bytes, i = 0;
  x->x_bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->x_bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv), x->x_min, x->x_max));
  outlet_list(x->x_outlet,0,argc,x->output_list);
}

static void wrap2_set(t_wrap2 *x, t_float f)
{
  x->x_f = f;
}

static void wrap2_bang(t_wrap2 *x)
{
  outlet_float(x->x_outlet,convert(x->x_f, x->x_min, x->x_max));
}

static void *wrap2_new(t_symbol *s, int argc, t_atom *argv)
{
  t_wrap2 *x = (t_wrap2 *) pd_new(wrap2_class);
///////////////////////////
    x->x_min = 0.;
    x-> x_max = 1.;
    if(argc == 1)
    {
        if(argv -> a_type == A_FLOAT)
        {
            x->x_min = 0;
            x->x_max = atom_getfloat(argv);
        }
        else goto errstate;
    }
    else if(argc == 2)
    {
        int numargs = 0;
        while(argc > 0 )
        {
            if(argv -> a_type == A_FLOAT)
            { // if nullpointer, should be float or int
                switch(numargs)
                {
                    case 0: x->x_min = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_max = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    default:
                        argc--;
                        argv++;
                        break;
                };
            }
            else // not a float
                goto errstate;
        };
    }
    else if(argc > 2)
        goto errstate;
///////////////////////////
  x->x_outlet = outlet_new(&x->x_obj, 0);
  x->x_bytes = sizeof(t_atom);
  floatinlet_new((t_object *)x, &x->x_min);
  floatinlet_new((t_object *)x, &x->x_max);
  x->output_list = (t_atom *)getbytes(x->x_bytes);
  if(x->output_list==NULL)
    {
    pd_error(x,"wrap2: memory allocation failure");
    return NULL;
    }
  return (x);
errstate:
    pd_error(x, "wrap2: improper args");
    return NULL;
}

static void wrap2_free(t_wrap2 *x)
{
  t_freebytes(x->output_list,x->x_bytes);
}

void wrap2_setup(void)
{
  wrap2_class = class_new(gensym("wrap2"), (t_newmethod)wrap2_new,
			  (t_method)wrap2_free,sizeof(t_wrap2),0, A_GIMME, 0);
  class_addfloat(wrap2_class,(t_method)wrap2_float);
  class_addlist(wrap2_class,(t_method)wrap2_list);
  class_addmethod(wrap2_class,(t_method)wrap2_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(wrap2_class,(t_method)wrap2_bang);
}
