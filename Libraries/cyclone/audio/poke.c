/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


/*dearsic-ing and fragile-ing (plucked the necessary bits from it)
 
  note, the fragile bit was using a way to pluck the float val from a signal
  ignoring usual float-to-signal conversions, this is found in pd's m_obj.c so 
  if something breaks wrt to inlet2, look there first

  Derek Kwan 2016

    needed for that one x->x_indexptr, seems like not many objects use fragile anymore so decided
    to stick it here, plus this is the only one that does this particular thing,
*/

#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"

#define POKE_MAXCHANNELS  64
#define POKE_REDRAWMS 500 //redraw time in ms

union inletunion
{
    t_symbol *iu_symto;
    t_gpointer *iu_pointerslot;
    t_float *iu_floatslot;
    t_symbol **iu_symslot;
    t_sample iu_floatsignalvalue;
};

struct _inlet
{
    t_pd i_pd;
    struct _inlet *i_next;
    t_object *i_owner;
    t_pd *i_dest;
    t_symbol *i_symfrom;
    union inletunion i_un;
};

// end extra stuff for x->x_indexptr

typedef struct _poke
{
    t_object x_obj;
    t_cybuf   *x_cybuf;
    t_sample  *x_indexptr;
    t_clock   *x_clock;
    int        x_channum; //current channel number (1-indexed)
    double     x_clocklasttick;
    int        x_clockset;
    double     x_redrawms; //time to redraw in ms
    t_inlet   *x_idxlet;
} t_poke;

static t_class *poke_class;

static void poke_tick(t_poke *x)
{
    cybuf_redraw(x->x_cybuf);  /* LATER redraw only dirty channel(s!) */
    x->x_clockset = 0;
    x->x_clocklasttick = clock_getlogicaltime();
}

static void poke_set(t_poke *x, t_symbol *s)
{
    cybuf_setarray(x->x_cybuf, s);
}

//redraw method/limiter
static void poke_redraw_lim(t_poke *x)
{
    double redrawms = x->x_redrawms;
    double timesince = clock_gettimesince(x->x_clocklasttick);
    if (timesince > redrawms ) poke_tick(x);
    else if (!x->x_clockset)
        {
        clock_delay(x->x_clock, redrawms - timesince);
        x->x_clockset = 1;
        };
}

/* CHECKED: index 0-based, negative values block input, overflowed are clipped.
   LATER revisit: incompatibly, the code below is nop for any out-of-range index
   (see also peek.c) */ // <= this looks like a bug, index 0 never written with signal (porres)

/* CHECKED: value never clipped, 'clip' not understood */ // ??? (porres)

/* CHECKED: no float-to-signal conversion.  'Float' message is ignored
   when dsp is on -- whether a signal is connected to the left inlet, or not
   (if not, current index is set to zero).  Incompatible (revisit LATER) */

// Above is not true, it should write when the dsp is on! (porres 2017)

static void poke_float(t_poke *x, t_float f)
    {
    t_cybuf *c = x->x_cybuf;
    t_word *vp = c->c_vectors[0];
    //second arg is to allow error posting
    cybuf_validate(c, 1);  // LATER rethink (efficiency, and complaining)
    if (vp)
        {
        int index = (int)*x->x_indexptr;
        if (index >= 0 && index < c->c_npts)
            {
            vp[index].w_float = f;
            poke_redraw_lim(x);
            };
        }
    }

static void poke_ft2(t_poke *x, t_floatarg f)
{
    int ch = (f < 1) ? 1 : (f > CYBUF_MAXCHANS) ? CYBUF_MAXCHANS : (int) f;
    x->x_channum = ch;
    cybuf_getchannel(x->x_cybuf, ch, 1);
}

/*
 static void poke_redraw_rate(t_poke *x, t_floatarg f){
    double redrawms = f > 0 ? (double)f : 1;
    x->x_redrawms = redrawms;
}
 
static void poke_redraw_force(t_poke *x){
    poke_tick(x);
}
*/

static t_int *poke_perform(t_int *w)
{
    t_poke *x = (t_poke *)(w[1]);
    t_cybuf *c = x->x_cybuf;
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // value
    t_float *in2 = (t_float *)(w[4]); // index
    t_word *vp = c->c_vectors[0];
    if (vp && c->c_playable) // ??? (porres)
        {
        poke_redraw_lim(x);
        int npts = c->c_npts;
        while (nblock--)
            {
            t_float value = *in1++; // value
            int index = (int)*in2++; // index
            if (index >= 0 && index < npts) vp[index].w_float = value;
            }
        }
    return (w + 5);
}

static void poke_dsp(t_poke *x, t_signal **sp)
{
    cybuf_checkdsp(x->x_cybuf);
    dsp_add(poke_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void poke_free(t_poke *x)
{
    if (x->x_clock) clock_free(x->x_clock);
    inlet_free(x->x_idxlet);
    cybuf_free(x->x_cybuf);
}

static void *poke_new(t_symbol *s, t_floatarg f)
{
    int ch = (f < 1) ? 1 : (f > CYBUF_MAXCHANS) ? CYBUF_MAXCHANS : (int)f;
	t_poke *x = (t_poke  *)pd_new(poke_class);
    x->x_cybuf = cybuf_init((t_class *) x, s, 1, ch);
    if (x) // ??? (porres)
        {
        x->x_channum = ch;
        x->x_redrawms = POKE_REDRAWMS; //default redraw rate
        
/* floats in 2nd inlet should be ignored when dsp is on
   when a signal is connected (porres 2017) */
        x->x_idxlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

/* plucked from old unstable/fragile.c. found in pd's m_obj.c, basically plucks the 
   float value from the signal without the float-to-signal conversion, I think... - DK */
        x->x_indexptr = &(x->x_idxlet)->i_un.iu_floatsignalvalue;
            
        inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft2"));
        x->x_clock = clock_new(x, (t_method)poke_tick);
        x->x_clocklasttick = clock_getlogicaltime();
        x->x_clockset = 0;
        }
    return (x);
}

CYCLONE_OBJ_API void poke_tilde_setup(void){
    poke_class = class_new(gensym("poke~"), (t_newmethod)poke_new,
        (t_method)poke_free, sizeof(t_poke), 0, A_DEFSYM, A_DEFFLOAT, 0);
    class_domainsignalin(poke_class, -1);
    class_addfloat(poke_class, poke_float);
    class_addmethod(poke_class, (t_method)poke_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(poke_class, (t_method)poke_set, gensym("set"), A_SYMBOL, 0);
    class_addmethod(poke_class, (t_method)poke_ft2, gensym("ft2"), A_FLOAT, 0);
/*    class_addmethod(poke_class, (t_method)poke_redraw_rate, gensym("redraw_rate"), A_FLOAT, 0);
    class_addmethod(poke_class, (t_method)poke_redraw_force, gensym("redraw"), 0);*/
}
