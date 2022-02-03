/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include "m_pd.h"
#include <common/api.h>
//#include "cybuf.h"

#define CYCYCLE_FREQ 	0
#define CYCYCLE_OFFSET 0
#define CYCYCLE_TABSIZE  512
#define COS_TABSIZE  16384
#define CYCYCLE_MAX 1728000000
typedef struct _cycle
{
    t_object   x_obj;

	t_float    x_freq;
    double     x_phase;
    double     x_conv;
    int        x_offset; //offset into window
    int        x_cycle_tabsize; //how far to loop
  int        x_user_tabsize; //user tabsize
    int		x_use_all;
    double    *x_costable;
    t_float     *x_usertable;
  t_float     x_usertable_ini[CYCYCLE_TABSIZE + 1];
  // t_cybuf    *x_cybuf;
    int        x_nameset; //if nameset, use usertable else use costable
  t_symbol   * x_name; //name of buffer
  int       x_warn; //flag to not warn about user table on init

	t_inlet    *x_phaselet;
	t_outlet   *x_outlet;
} t_cycle;

static t_class *cycle_class;


//making the cosine table
static double *cycle_makecostab(t_cycle *x){
	/*it looks like the original sickle was rounding up from the specified table length so COS_TABSIZE
	 originally wasn't even reflective of the actual table size... 
	 I've tried stepsize being twopi/(COS_TABSIZE) and also twopi/(COS_TABSIZE+1),.. in the former
	 case the last index would be cos(twopi) while in the latter it would be cos(twopi-1/(COS_TABSIZE+1))...
	 both seem to get no clicks,.. but if I make the wavetable exactly COS_TABSIZE indices,.. clicks....
	 so it has to be the way the tablesize is used in the perform methods... - DXK 
	 
	Matt's comments: yes, it was rounding up in case some other object using the makecostable function
	 asked for something that wasn't a power of 2.
	 
	 The function from sic made a table of COS_TABSIZE+1, and made sure that the waveform was symmetric.
	 In order to do this, the first quarter of the wave table was generated, then the 0 in the next index,
	 which needs to be exactly 0 (and cos(x) doesn't guarantee that even with a precise 2.f*M_PI). Then
	 the first quarter is copied in reverse order with a phase reversal, and then the first half is
	 copied exactly, in reverse order. This ensures that the guard point is the same as the 0th point, which
	 is exactly 1.0. The stepsize has to be twopi/(COS_TABSIZE) to get this to work, since you want a full
	 cycle through the first COS_TABSIZE, and then the beginning of a new cycle at the guard point.
	 
	 One last thing - since this table is exactly the same for each instance of [cycle~], it can be moved
	 out and accessed in one place by all instances, as in [osc~], rather than having each instance own its
	 own table. This saves memory (not that it is breaking the bank), but more importantly load time. So when
	 we call this function it either makes the table and returns a pointer, or if it's already been made it
	 just returns the pointer.
	*/
	 
	static double *costable; // stays allocated as long as pd is running
	 
	if (costable)
	{
		return costable;
	}
	double twopi = 2.0 * M_PI;
	int ndx, sz = COS_TABSIZE;
	int cnt = sz + 1;
	costable = getbytes(cnt * sizeof(*costable));
	double *tp = costable;
	int i;
	double phase = 0, phsinc = twopi / sz;
	if (costable)
	{
		i = sz/4;
		while (i--)
	    {
			*tp++ = cos(phase);
			phase += phsinc;		
	    }
	    *tp++ = 0;
	    i = sz/4;
	    while (i--)
	    {
	    	*tp++ = -costable[i];
	    }
	    i = sz/2;
	    while (i--)
	    {
	    	*tp++ = costable[i];
	    }
	}
	else
	{
		pd_error(x, "could not make cosine table");
	}
	return costable;
}


static t_word *cycle_fetcharray(t_cycle * x, int * tabsz, int indsp){
	//modifying old vefl_get a bit - DXK
	t_symbol * name = x->x_name;
	char buf[MAXPDSTRING];
	t_symbol * chname;
        int warn = x->x_warn;
	if (name && name != &s_){
		t_garray *ap = (t_garray *)pd_findbyclass(name, garray_class);
		if(!ap)
		  {
		    sprintf(buf, "0-%s",  x->x_name->s_name);
		    chname = gensym(buf);
		   ap = (t_garray *)pd_findbyclass(chname, garray_class);

		  };
	if (ap){
			int tabsize;
			t_word *vec;
			if (garray_getfloatwords(ap, &tabsize, &vec)){
				if (indsp)
				{
					garray_usedindsp(ap);
				}
				if (tabsz)
				{
					*tabsz = tabsize;
				}
				return (vec);
			}
			else 
			{ /* always complain */
			  if(warn) pd_error(x, "[cycle~]: bad template of array '%s'", name->s_name);
			}
		}
		else if (x)
		{
		  if(warn) pd_error(x, "[cycle~]: no such array '%s'", name->s_name);
		}
	}
	return (0);

}

static void cycle_gettable(t_cycle *x)
{   

  //original allocated for array size + 1, was that for a guard point? not including for now...
  //but can uncomment wantsz++ and the x->x_usertable[wantsz] = 0 lines if you want to include it
  int i;
    
    t_word *table = 0;
    int wantsz;
    int warn = x->x_warn;
    if (x->x_name) table = cycle_fetcharray(x, &wantsz, 1);
    //wantsz++;	
	/* CHECKED buffer is copied */
	if (table != NULL)
	  {

	    int cursz = x->x_user_tabsize;
	    int heaped = x->x_usertable != x->x_usertable_ini;


    if(heaped && wantsz <= CYCYCLE_TABSIZE){
        //if x_value is pointing to a heap and incoming list can fit into CYCYCLE_TABSIZE
        //deallocate x_value and point it to stack, update status
        freebytes(x->x_usertable, (cursz) * sizeof(t_float));
        x->x_usertable = x->x_usertable_ini;
        x->x_user_tabsize = CYCYCLE_TABSIZE;
        }
    else if(heaped && wantsz > CYCYCLE_TABSIZE && wantsz > cursz){
        //if already heaped, incoming list can't fit into CYCYCLE_TABSIZE and can't fit into allocated t_atom
        //reallocate to accomodate larger list, update status
        

        //bounds checking for maxsize
        if(wantsz > CYCYCLE_MAX)
            wantsz = CYCYCLE_MAX;
	
        x->x_usertable = (t_float *)resizebytes(x->x_usertable, cursz * sizeof(t_float), wantsz * sizeof(t_float));
        x->x_user_tabsize = wantsz;
    }
    else if(!heaped && wantsz > CYCYCLE_TABSIZE){
        //if not heaped and incoming list can't fit into CYCYCLE_TABSIZE
        //allocate and update status


        //bounds checking for maxsize
        if(wantsz > CYCYCLE_MAX)
            wantsz = CYCYCLE_MAX;
        x->x_usertable = (t_float *)getbytes(wantsz * sizeof(t_float));
        x->x_user_tabsize = wantsz;

	  };
    x->x_nameset = 1;
    //now to copy contents of array into allocated array
    for(i=0; i < wantsz; i++)
      x->x_usertable[i] = table[i].w_float;
    //x->x_usertable[wantsz] = 0;
    if(x->x_use_all) x->x_cycle_tabsize = x->x_user_tabsize;
	  }
    else 
    {
    	if (x->x_name)
    	{
	  x->x_nameset = 0;
	  if(warn) pd_error(x, "[cycle~]: using cosine table");
    	}
    	//x->x_name = 0;
    	x->x_cycle_tabsize = COS_TABSIZE;
    }
}




static void cycle_phase_reset(t_cycle *x){
    x->x_phase = 0.;
}

static void cycle_set(t_cycle *x, t_symbol *s, t_floatarg f)
{
	x->x_use_all = 0;
	x->x_offset = 0;
	x->x_cycle_tabsize = CYCYCLE_TABSIZE;
    if (s && s != &s_)
    {
      //cybuf_setarray(x->x_cybuf, s);
        x->x_name = s;
	x->x_offset = f < 0 ? 0 : (int)f;
	cycle_gettable(x);
	//x->x_nameset = 1;
	
    }
    else{
        if(x->x_nameset > 0){
            //only reset if switching from named buffer
            cycle_phase_reset(x);
        };
        x->x_nameset = 0;
        pd_error(x, "[cycle~]: using cosine table");
    };

}

static void cycle_setall(t_cycle *x, t_symbol *s)
{
	x->x_use_all = 1;
	x->x_offset = 0;
    if (s && s != &s_){
      //cybuf_setarray(x->x_cybuf, s);
      //x->x_nameset = 1;
      x->x_name = s;
	cycle_gettable(x);
	}
    else{
        if(x->x_nameset > 0){
            //only reset if switching from named buffer
            cycle_phase_reset(x);
        };
        x->x_nameset = 0;
        pd_error(x, "[cycle~]: using cosine table");
    };

}

static void cycle_buffer_offset(t_cycle *x, t_floatarg f)
{
    x->x_offset = f < 0 ? 0 : (int) f;
}


static void cycle_set_buffersize(t_cycle *x, t_floatarg f)
{
	if (f==0.)
	{
		x->x_use_all = 0;
		x->x_cycle_tabsize = CYCYCLE_TABSIZE;
	}
	else if (f == -1.)
	{
		x->x_use_all = 1;
		x->x_cycle_tabsize = x->x_user_tabsize;
	}
	else if (f == (1 << ilog2(f)) && f <= 65536. && f >= 16)
	{
		x->x_use_all = 0;
		x->x_cycle_tabsize = f;
	}
	else
	{
		pd_error(x, "[cycle~]: buffer_sizeinsamps must be a power of two from 16 to 65536");
		return;
	}
}

static void cycle_buffer_sizeinsamps(t_cycle *x, t_floatarg f)
{

	cycle_set_buffersize(x, f);
}

static void cycle_frequency(t_cycle *x, t_floatarg f)
{
    x->x_freq = f;
}

static void cycle_phase(t_cycle *x, t_floatarg f)
{
    f = f < 0 ? 0 : (f > 1 ? 1 : f);
    pd_float((t_pd *)x->x_phaselet, f);
}

static t_int *cycle_perform(t_int *w)
{
	t_cycle *x = (t_cycle *)(w[1]);
	//t_cybuf * c = x->x_cybuf;
	int nblock = (int)(w[2]);
	t_float *in1 = (t_float *)(w[3]);
	t_float *in2 = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	//t_word *tab = c->c_vectors[0];
        t_float *tab = x->x_usertable;
	double  *costab = x->x_costable;
	double dphase = x->x_phase;
	double conv = x->x_conv;
	double wrapphase, tabphase, frac;
	t_float f1, f2, freq, phaseoff, output;
	double df1, df2;
	int i, index;
	int cycle_tabsize = x->x_cycle_tabsize;
	int offset = x->x_offset;
	//int npts = c->c_npts;
	int npts = x->x_user_tabsize;
	//int usetable = x->x_nameset > 0 && c->c_playable; //use user table
	int usetable = x->x_nameset > 0;
	
        for (i=0; i< nblock; i++)
	{
		freq = in1[i];
		phaseoff = in2[i];
	
                wrapphase = dphase + phaseoff;

                while(wrapphase >= 1){
                    wrapphase -= 1.;
                };
                while(wrapphase < 0){
                    wrapphase += 1.;
                };

                if(usetable){	
			tabphase = wrapphase * cycle_tabsize;
		}
		else{
			tabphase = wrapphase * COS_TABSIZE;
		};
		index = (int)tabphase;
		frac = tabphase - index;
		if (usetable)
		{
			f1 = (offset + index) >= npts ? 0. : tab[offset + index];
			index++;
			f2 = (offset + index) >= npts ? 0. : tab[offset + index];
			output = (t_float)(f1 + frac * (f2 - f1));
		}
		else
		{
			df1 = costab[index++];
			df2 = costab[index];
			output = (t_float) (df1 + frac * (df2 - df1));
		};
                out[i] = output;

                //incrementation step
                dphase += ((double)freq * conv);
                while(dphase >= 1){
                    dphase -= 1.;
                };
                while(dphase < 0){
                    dphase += 1.;
                };

	};

        x->x_phase = dphase;
	
        return (w + 6);
}
	
static void cycle_dsp(t_cycle *x, t_signal **sp)
{
  //cybuf_checkdsp(x->x_cybuf);
  cycle_gettable(x);
    x->x_conv = 1.0 / sp[0]->s_sr;
    cycle_phase_reset(x);
    //post("cycle tabsize = %d", x->x_cycle_tabsize);
    dsp_add(cycle_perform, 5, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *cycle_new(t_symbol *s, int argc, t_atom *argv)
{
    t_cycle *x = (t_cycle *)pd_new(cycle_class);
    
	t_symbol * name = NULL;
	t_float phaseoff, freq, offset, bufsz;

	phaseoff = 0;
        freq = CYCYCLE_FREQ;
	offset = CYCYCLE_OFFSET;
	bufsz = CYCYCLE_TABSIZE;
	int argnum = 0;
	int pastargs = 0; //flag if attributes are specified, then don't accept array name anymore
	int bufferattrib = 0; //flag if @buffer attribute is set; needs to default to whole buffer
	int buffersizeattrib = 0; // flag if @buffer_sizeinsamps attribute is set
	while(argc > 0){
		if(argv -> a_type == A_FLOAT){
			if(!pastargs){
				t_float argval = atom_getfloatarg(0, argc, argv);
				switch(argnum){
					case 0:
						freq = argval;
						break;
					case 1:
						offset = argval;
						break;
					default:
						break;
					};
					argnum++;
				};
				argc--;
				argv++;
			}

		else if(argv -> a_type == A_SYMBOL){
			t_symbol * curarg = atom_getsymbolarg(0, argc, argv);
			if(strcmp(curarg->s_name, "@buffer")==0){
				if(argc >= 2){
					if((argv+1) -> a_type == A_SYMBOL){
						name = atom_getsymbolarg(1, argc, argv);
						argc-=2;
						argv+=2;
						pastargs = 1;
						bufferattrib = 1;
					}
					else{
						goto errstate;
					};
				}
				else{
					goto errstate;
				};
			}
			else if(strcmp(curarg->s_name, "@buffer_offset")==0){
				if(argc >= 2){
					offset = atom_getfloatarg(1, argc, argv);
					argc-=2;
					argv+=2;
					pastargs = 1;
				}
				else{
					goto errstate;
				};
			}
			else if(strcmp(curarg->s_name, "@buffer_sizeinsamps")==0){
				if(argc >= 2){
					bufsz = atom_getfloatarg(1, argc, argv);
					argc-=2;
					argv+=2;
					pastargs = 1;
					if(bufsz > 0) buffersizeattrib = 1;
				}
				else{
					goto errstate;
				};
			}
			else if(strcmp(curarg->s_name, "@phase")==0){
				if(argc >= 2){
					phaseoff = atom_getfloatarg(1, argc, argv);
					argc-=2;
					argv+=2;
					pastargs = 1;
				}
				else{
					goto errstate;
				};
			}
			else if(strcmp(curarg->s_name, "@frequency")==0){
				if(argc >= 2){
					freq  = atom_getfloatarg(1, argc, argv);
					argc-=2;
					argv+=2;
					pastargs = 1;
				}
				else{
					goto errstate;
				};
			}
			else if(!pastargs){
				//specifying buffer name through argt
					name = curarg;
					argc--;
					argv++;
			}
			else{
				goto errstate;
			};
		}
		else{
			goto errstate;
		};
	};

    x->x_costable = cycle_makecostab(x);
    x->x_phaselet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    cycle_phase(x, phaseoff);

    x->x_outlet = outlet_new(&x->x_obj, gensym("signal"));
    if(offset < 0)  offset = 0;
    x->x_offset = offset;
    x->x_nameset = name ? 1 : 0;
    //x->x_cybuf = cybuf_init((t_class *)x, name, 1, 0);
    x->x_usertable = x->x_usertable_ini;
    x->x_user_tabsize = CYCYCLE_TABSIZE;
    x->x_warn = 0;
    if(x->x_nameset == 1)
      cycle_set(x, name, offset);
    x->x_warn = 1;
    x->x_freq = freq;
    x->x_phase = 0;
    x->x_conv = 0.;
    cycle_set_buffersize(x, bufsz);
    if(bufferattrib && !buffersizeattrib) x->x_use_all = 1;
    return (x);
	errstate:
		pd_error(x, "[cycle~]: improper args");
		return NULL;
}

static void *cycle_free(t_cycle *x)
{
  //cybuf_free(x->x_cybuf);
  if (x->x_usertable != x->x_usertable_ini)
    freebytes(x->x_usertable, (x->x_user_tabsize + 1) * sizeof(*x->x_usertable));
	//free(x->x_costable);
	outlet_free(x->x_outlet);
	inlet_free(x->x_phaselet);
	return (void *)x;
}

CYCLONE_OBJ_API void cycle_tilde_setup(void)
{    
    cycle_class = class_new(gensym("cycle~"), (t_newmethod)cycle_new, (t_method)cycle_free,
        sizeof(t_cycle), 0, A_GIMME, 0);
	CLASS_MAINSIGNALIN(cycle_class, t_cycle, x_freq); 
	class_addmethod(cycle_class, (t_method)cycle_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(cycle_class, (t_method)cycle_set,gensym("set"),
        A_DEFSYMBOL, A_DEFFLOAT, 0);
	class_addmethod(cycle_class, (t_method)cycle_setall, gensym("buffer"),
		A_DEFSYMBOL, 0);
    class_addmethod(cycle_class, (t_method)cycle_buffer_offset,gensym("buffer_offset"),
        A_DEFFLOAT, 0);
    class_addmethod(cycle_class, (t_method)cycle_buffer_sizeinsamps,gensym("buffer_sizeinsamps"),
        A_DEFFLOAT, 0);
    class_addmethod(cycle_class, (t_method)cycle_setall,gensym("setall"),
        A_DEFSYMBOL, 0);
    class_addmethod(cycle_class, (t_method)cycle_frequency, gensym("frequency"), A_DEFFLOAT, 0);
    class_addmethod(cycle_class, (t_method)cycle_phase, gensym("phase"), A_DEFFLOAT, 0);
}
