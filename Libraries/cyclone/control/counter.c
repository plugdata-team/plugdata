/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* rewritten version of Joseph A. Sarlo's code fom pb-lib 
   (important changes listed in "pd-lib-notes.txt" file.  */

/* krzYszcz circa (2002?)
   Beware -- the max reference manual page for the counter object
   reflects mostly opcode max features.  Apparently counter works
   differently in cycling max (e.g. inlets 3 and 4).  But I am sick
   of checking -- I will not bother, until there is some feedback. */

// Porres in 2016/2017 checked the inconsistencies and fixed them

// Adding attributes, p_id, counter_proxy_state, editing existing counter_proxy methods - Derek Kwan 2016

#include "m_pd.h"
#include <common/api.h>
#include <string.h>

#define COUNTER_UP      0
#define COUNTER_DOWN    1
#define COUNTER_UPDOWN  2
#define COUNTER_DEFMAX  16777216  // 2ˆ24
#define COUNTER_CARRY 0 // defaut for carryflag
#define COUNTER_COMPAT 0 // default for compatmode

typedef struct _counter
{
    t_object   x_ob;
    int        x_count;
    int        x_maxcount;
    int        x_dir;
    int        x_inc;
    int        x_min;
    int        x_setmin;
    int        x_setmax;
    int        x_max;
    int        x_compatflag;
    int        x_carrybang;
    int        x_minhitflag;
    int        x_maxhitflag;
    int        x_startup;
    t_pd      *x_proxies[4];
    t_outlet  *x_out2;
    t_outlet  *x_out3;
    t_outlet  *x_out4;
} t_counter;

typedef struct _counter_proxy
{
    t_object    p_ob;
    int         p_id; //inlet id - DK
    t_counter  *p_master;
    void      (*p_bangmethod)(t_counter *x);
    void      (*p_floatmethod)(t_counter *x, t_float f);
} t_counter_proxy;

static t_class *counter_class;
static t_class *counter_proxy_class;

static void counter_up(t_counter *x)
{
    x->x_dir = COUNTER_UP;
    x->x_inc = 1;
}

static void counter_down(t_counter *x)
{
    /* CHECKED: no explicit minimum needed */
    x->x_dir = COUNTER_DOWN;
    x->x_inc = -1;
}

static void counter_updown(t_counter *x)
{
    /* CHECKED: neither explicit maximum, nor minimum needed */
    x->x_dir = COUNTER_UPDOWN;
    /* CHECKED: x->x_inc unchanged (continuation) */
}

static void counter_dir(t_counter *x, t_floatarg f)
{
    switch ((int)f)
    {
    case COUNTER_UP:
        counter_up(x);
	break;
    case COUNTER_DOWN:
        counter_down(x);
	break;
    case COUNTER_UPDOWN:
        counter_updown(x);
	break;
    default:
        counter_up(x);  /* CHECKED: invalid == default */
	/* CHECKED: no warning */
    }
}

static void counter_dobang(t_counter *x, int notjam)
{
    if(x->x_startup) x->x_startup = 0;
    int offmin = 0, offmax = 0, onmin = 0, onmax = 0; // ALL 0!!!
    
    // CHECKED: carry-off not sent if min >= max (BUT IT IS) <= Hack
    if (x->x_min < x->x_max) offmin = x->x_minhitflag, offmax = x->x_maxhitflag; // why???
    // if it did hit before min or max, now it is off
    
    x->x_minhitflag = x->x_maxhitflag = 0;
    
    if (x->x_count < x->x_min) // WRAP!!!?!?!?
        {
            if (x->x_dir == COUNTER_UPDOWN)
            {
                x->x_inc = 1; // Pong!
                if ((x->x_count = x->x_min + 1) > x->x_max) x->x_count = x->x_min; // ???
            }
            else if ((x->x_count = x->x_max) < x->x_min) x->x_count = x->x_min; // ???
        }
    else if (x->x_count > x->x_max)
        {
        if (x->x_inc == -1) //
            {
            // <=== ???
            }
        else if (x->x_dir == COUNTER_UPDOWN)
            {
            x->x_inc = -1;
            if ((x->x_count = x->x_max - 1) < x->x_min) x->x_count = x->x_min; // ???
            }
        else x->x_count = x->x_min; // wrap for dir = up!!!
        }

    if (x->x_count <= x->x_min && x->x_inc == -1)
        {
        // CARRY MIN!!! count <= min, downwards
        if (notjam) onmin = 1; // CHECKED: jam inhibits mid outlets (hacky)
        }
    
// OUTLETS!!!!
    
    // outlet 4) maxcount
    else if (x->x_count >= x->x_max && x->x_inc == 1)
        {
        x->x_maxcount++;
        outlet_float(x->x_out4, x->x_maxcount); // count how many times max was reached
         
        // CARRY MAX!!! count >= max, upwards
        if (notjam) onmax = 1;  // CHECKED: jam inhibits mid outlets (hacky)
        }

    // outlet 3) carry max
    if (onmax)
        {
        if (x->x_carrybang)
            {
            x->x_min = x->x_setmin;
            x->x_max = x->x_setmax;
            outlet_bang(x->x_out3);
            }
        else
            {
            x->x_min = x->x_setmin;
            x->x_max = x->x_setmax;
            outlet_float(x->x_out3, 1);
            x->x_maxhitflag = 1;
            }
        }
    else if (offmax) outlet_float(x->x_out3, 0);
    
    // outlet 2) carry min
    else if (onmin)
        {
        if (x->x_carrybang)
            {
            x->x_min = x->x_setmin;
            x->x_max = x->x_setmax;
            outlet_bang(x->x_out2);
            }
        else
            {
            x->x_min = x->x_setmin;
            x->x_max = x->x_setmax;
            outlet_float(x->x_out2, 1);
            x->x_minhitflag = 1;
            }
        }
    else if (offmin) outlet_float(x->x_out2, 0);

    // outlet 1) count
    outlet_float(((t_object *)x)->ob_outlet, x->x_count);
    
    if (onmin && x->x_min > x->x_max) x->x_count = x->x_max;
}

static void counter_bang(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    x->x_count += x->x_inc;
    counter_dobang(x, 1);
}

static void counter_float(t_counter *x, t_float dummy)
{
    counter_bang(x);
}

// CHECKED: out-of-range values are ignored
static void counter_set(t_counter *x, t_floatarg f)
{
    if(x->x_startup) x->x_startup = 0;
    int i = (int)f;
    if (i >= x->x_min && i <= x->x_max)
	x->x_count = i - x->x_inc;
}

static void counter_setmin(t_counter *x, t_floatarg f)
{
    x->x_setmin = (int)f;
    if (x->x_min > x->x_max) x->x_min = x->x_setmin;
}

// CHECKED: out-of-range values are ignored
static void counter_jam(t_counter *x, t_floatarg f)
{
    if(x->x_startup) x->x_startup = 0;
    int i = (int)f;
    if (i >= x->x_min && i <= x->x_max)
    {
	x->x_count = i;
	counter_dobang(x, 0);
    }
}

/* CHECKED: sends max carry on/off in any mode  */
static void counter_inc(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    int tempdir = x->x_dir;
    int tempinc = x->x_inc;
    counter_up(x);
    counter_bang(x);
    x->x_dir = tempdir;
    x->x_inc = tempinc;
}

/* CHECKED: sends min carry on/off in any mode */
static void counter_dec(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    int tempdir = x->x_dir;
    int tempinc = x->x_inc;
    counter_down(x);
    counter_bang(x);
    x->x_dir = tempdir;
    x->x_inc = tempinc;
}

/* CHECKED: min can be set over max */
static void counter_min(t_counter *x, t_floatarg f)
{
    if(x->x_startup) x->x_startup = 0;
    /* CHECKED: min change always sets count to min and bangs */
    /* do not use counter_jam() here -- avoid range checking */
    x->x_count = x->x_min = x->x_setmin = (int)f;
    counter_dobang(x, 0);
}

/* CHECKED: max can be set below min */
static void counter_max(t_counter *x, t_floatarg f)
{
    x->x_max = x->x_setmax = (int)f;
}

static void counter_carrybang(t_counter *x)
{
    x->x_carrybang = 1;
}

static void counter_carryint(t_counter *x)
{
    x->x_carrybang = 0;
}

static void counter_carryflag(t_counter *x, t_floatarg f)
    //editing to handle non 0/1 input - Derek Kwan
{
    int i = (int)f;
    if (i == 1)  x->x_carrybang = 1;
    if (i == 0)  x->x_carrybang = 0;
}

static void counter_compatmode(t_counter *x, t_floatarg f)
{
// editing to handle non 0/1 input - Derek Kwan
    int i = (int)f;
    if(i <= 0){
        x->x_compatflag = 0;
    }
    else{
        x->x_compatflag = 1;
    };
}

static void counter_flags(t_counter *x, t_floatarg f1, t_floatarg f2)
{
    //simplifying to rely on other methods - Derek Kwan
    counter_carryflag(x, f1);
    counter_compatmode(x, f2);
}


static void counter_proxy_state(t_counter_proxy *x)
{
    //since main inlet isn't a proxy, have to do it this way - Derek Kwan
    t_counter *m = x->p_master; 
    post("-=%% CounterState %%=-");
    post("x_mincount: %d", m->x_min);
    post("x_maxcount: %d", m->x_max);
    post("x_direction: %d", m->x_dir);
    post("x_curcount:  %d", m->x_count);
    post("x_curdir: %d", m->x_inc < 0);
    post("x_carrycount: %d", m->x_maxcount);
    post("x_carry: %d", m->x_maxhitflag);
    post("x_under: %d", m->x_minhitflag);
    post("x_carrymode: %d", m->x_carrybang);
    post("x_compat: %d", m->x_compatflag);
    post("x_startup: %d", m->x_startup); 
    post("x_inletnum: %d", x->p_id); // print which inlet the message came in
}




static void counter_state(t_counter *x)
{
    post("-=%% CounterState %%=-");
    post("x_mincount: %d", x->x_min);
    post("x_maxcount: %d", x->x_max);
    post("x_direction: %d", x->x_dir);
    post("x_curcount:  %d", x->x_count);
    post("x_curdir: %d", x->x_inc < 0);
    post("x_carrycount: %d", x->x_maxcount);
    post("x_carry: %d", x->x_maxhitflag);
    post("x_under: %d", x->x_minhitflag);
    post("x_carrymode: %d", x->x_carrybang);
    post("x_compat: %d", x->x_compatflag);
    post("x_startup: %d", x->x_startup); 
    post("x_inletnum: 0"); // print which inlet the message came in
}

/* CHECKED: up/down switch */
static void counter_bang1(t_counter *x)
{
    if (x->x_dir == COUNTER_UP)
        counter_down(x);
    else if (x->x_dir == COUNTER_DOWN)
        counter_up(x);
    else
	x->x_inc = -x->x_inc;  /* CHECKED */
}

/* CHECKED */
static void counter_bang2(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    counter_set(x, x->x_min);
}

static void counter_float2(t_counter *x, t_floatarg f)
{
    if(x->x_startup) x->x_startup = 0;
    int i = (int)f;
    if (x->x_compatflag)     // ancient
    {
        x->x_count = x->x_min = x->x_setmin= i;
        counter_set(x, i);
    }
    else  { // current
        x->x_count = i;
        if(x->x_count < x->x_min) x->x_min = x->x_count;
        if(x->x_count > x->x_max) x->x_max = x->x_count;
        counter_set(x, i);
    }
}

static void counter_bang3(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    x->x_min = x->x_setmin;
    counter_jam(x, x->x_min);
}


static void counter_float3(t_counter *x, t_floatarg f)
{
    if(x->x_startup) x->x_startup = 0;
    int i = (int)f;
    if (x->x_compatflag)     // ancient
        {
        x->x_count = x->x_min = x->x_setmin = i;
        counter_dobang(x, 0);
        }
    else {    // current
        x->x_count = i;
        if(x->x_count < x->x_min) x->x_min = x->x_count;
        if(x->x_count > x->x_max) x->x_max = x->x_count;
        counter_set(x, i);
        counter_bang(x);
        }
}

/* CHECKED */
static void counter_bang4(t_counter *x)
{
    if(x->x_startup) x->x_startup = 0;
    x->x_count = x->x_max;
    counter_dobang(x, 1);
}

static void counter_proxy_bang(t_counter_proxy *x)
{
    t_counter *m = x->p_master; 
    x->p_bangmethod(m);
}

static void counter_proxy_float(t_counter_proxy *x, t_float f)
{
    t_counter *m = x->p_master; 
    x->p_floatmethod(m, f);
}

static void counter_free(t_counter *x)
{
    int i;
    for (i = 0; i < 4; i++)
	if (x->x_proxies[i]) pd_free(x->x_proxies[i]);
}

static void *counter_new(t_symbol * s, int argc, t_atom * argv)
{
    t_counter *x = (t_counter *)pd_new(counter_class);
    t_counter_proxy **pp = (t_counter_proxy **)x->x_proxies;
    //defaults
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
    int carry = COUNTER_CARRY;
    int compat = COUNTER_COMPAT;
    
    int argnum = 0; //argument number
    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curfloat = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    i1 = (int)curfloat;
                    break;
                case 1:
                    i2 = (int)curfloat;
                    break;
                case 2:
                    i3 = (int)curfloat;
                    break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        }
        else{
            if(argc >= 2){
                t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
                t_float curfloat = atom_getfloatarg(1, argc, argv);
                if(strcmp(cursym->s_name, "@carryflag") == 0){
                    carry = (int)curfloat;
                }
                else if(strcmp(cursym->s_name, "@compatmode") == 0){
                    compat = (int)curfloat;
                }
                else
                    {
                    goto errstate;
                    };
                argc -= 2;
                argv += 2;
            }
            else
                {
                goto errstate;
                };
        };
    };
    
    counter_carryflag(x, carry);
    counter_compatmode(x, compat);
    int i;
    x->x_dir = COUNTER_UP;
    x->x_inc = 1;  /* previous value required by counter_dir() */
    x->x_min = x->x_setmin = 0;
    x->x_startup = 1;
    x->x_max = x->x_setmax = COUNTER_DEFMAX;
    if (argnum == 3)
        {
        x->x_dir = i1;
        x->x_min = x->x_setmin = i3 < i2 ? i3: i2;
        x->x_max = x->x_setmax = i2 > i3? i2 : i3;
        }
    else if (argnum == 2)
        {
        x->x_min = x->x_setmin = i2 < i1 ? i2: i1;
        x->x_max = x->x_setmax = i1 > i2? i1 : i2;
        }
    else if (argnum == 1) x->x_max = x->x_setmax = i1;
    x->x_minhitflag = x->x_maxhitflag = 0;
    x->x_maxcount = 0;
    counter_dir(x, x->x_dir);
    x->x_count = (x->x_dir == COUNTER_DOWN ? x->x_max - x->x_inc : x->x_min - x->x_inc);
    for (i = 0; i < 4; i++)
    {
        x->x_proxies[i] = pd_new(counter_proxy_class);
        ((t_counter_proxy *)x->x_proxies[i])->p_master = x;
        ((t_counter_proxy *)x->x_proxies[i])->p_id = i + 1;
        inlet_new((t_object *)x, x->x_proxies[i], 0, 0);
    };
    (*pp)->p_bangmethod = counter_bang1;
    (*pp++)->p_floatmethod = counter_dir;  /* CHECKED: same as dir */
    (*pp)->p_bangmethod = counter_bang2;
    (*pp++)->p_floatmethod = counter_float2;
    (*pp)->p_bangmethod = counter_bang3;
    (*pp++)->p_floatmethod = counter_float3;
    (*pp)->p_bangmethod = counter_bang4;
    (*pp++)->p_floatmethod = counter_max;  /* CHECKED: same as max */
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_anything);  /* float/bang */
    x->x_out3 = outlet_new((t_object *)x, &s_anything);  /* float/bang */
    x->x_out4 = outlet_new((t_object *)x, &s_float);
    return (x);
errstate:
    pd_error(x, "counter: improper args");
    return NULL;
}

CYCLONE_OBJ_API void counter_setup(void)
{
    counter_class = class_new(gensym("counter"), (t_newmethod)counter_new,
            (t_method)counter_free, sizeof(t_counter), 0,
            A_GIMME, 0);
    class_addbang(counter_class, counter_bang);
    class_addfloat(counter_class, counter_float);
    class_addmethod(counter_class, (t_method)counter_bang,
		    gensym("next"), 0);
    class_addmethod(counter_class, (t_method)counter_set,
		    gensym("set"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_set,
		    gensym("goto"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_jam,
		    gensym("jam"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_up,
		    gensym("up"), 0);
    class_addmethod(counter_class, (t_method)counter_down,
		    gensym("down"), 0);
    class_addmethod(counter_class, (t_method)counter_updown,
		    gensym("updown"), 0);
    class_addmethod(counter_class, (t_method)counter_inc,
		    gensym("inc"), 0);
    class_addmethod(counter_class, (t_method)counter_dec,
		    gensym("dec"), 0);
    class_addmethod(counter_class, (t_method)counter_min,
		    gensym("min"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_setmin,
                    gensym("setmin"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_max,
		    gensym("max"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_carrybang,
		    gensym("carrybang"), 0);
    class_addmethod(counter_class, (t_method)counter_carryint,
		    gensym("carryint"), 0);
    class_addmethod(counter_class, (t_method)counter_flags,
                    gensym("flags"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_compatmode,
                    gensym("compatmode"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_carryflag,
                    gensym("carryflag"), A_FLOAT, 0);
    class_addmethod(counter_class, (t_method)counter_state,
                    gensym("state"), 0);
    counter_proxy_class = class_new(gensym("_counter_proxy"), 0, 0,
            sizeof(t_counter_proxy), CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(counter_proxy_class, counter_proxy_bang);
    class_addfloat(counter_proxy_class, counter_proxy_float);
    class_addmethod(counter_proxy_class, (t_method)counter_proxy_state, gensym("state"), 0);
}
