
/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Derek Kwan 2016 
    adding looping, loopinterp, etc. (so pretty much redone, abstracted out interpolated reading of vecs)
    not sure about array sample rate vs pd sample rate and my hacks, can be investigated more
    adding play_play helper func
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"
#include "common/magicbit.h"
#include "common/shared.h"


#define PLAY_MINITIME 0.023 //minimum ms for xfade. 1/44.1 rounded up
//note, loop starts fading in at stms, starts fading out at endms
#define PLAY_LINTIME 50 //default interp time in ms
#define PLAY_LOOP 0 //loop default setting
#define PLAY_LINTERP 0 //loop interp default
/* CHECKME (the refman): if the buffer~ has more channels, channels are mixed */


typedef struct _play
{
    t_object    x_obj;
    t_cybuf   *x_cybuf;
    t_glist  *x_glist;
    int         x_hasfeeders; //if there's a signal coming in the main inlet 
    int         x_npts; //array size in samples
    t_float     x_pdksr; //pd's sample rate in ms
    t_float     x_aksr; //array's sample rate in ms
    double     x_ksrrat; //sample rate ratio (array/pd)
    t_float     x_fadems; //xfade in ms (also known as interptime)

    t_float     x_stms; //start position in ms
    t_float     x_endms; //end pos in ms
    t_float     x_durms; //duration in ms
    
    int     x_stsamp; //start position in samp
    int     x_stxsamp; //end of xfade ramp from start in samp
    int     x_endsamp; //end pos in samp
    int     x_endxsamp; //end of xfade ramp from end in ms
    int     x_rangesamp; //length of loop in samples
    int     x_fadesamp; //length of fade in samples
    
    int         x_isneg; //if rate is negative (start point > end point)
    double     x_rate; //rate of playback
    double      x_phase; //phase for float playback mode

    int         x_looping; //if looping or not
    int         x_linterp; //if using loop interpolation or not
    int         x_playing; //if playing
    int         x_playnew; //if started playing this particular block
    int         x_rfirst; //if this is the first time ramping up (for linterp mode)
    int 	x_numchans;
    t_float     *x_ivec; // input vector
    t_float     **x_ovecs; //output vectors

    t_outlet    *x_donelet;
} t_play;





static t_class *play_class;

static void play_play(t_play *x, int toplay){
    //playing helper function, use this to start playing the array
    int playing = toplay > 0 ? 1 : 0;
    x->x_playing = playing;
    if(playing){
        x->x_playnew = 1;
    };
}


////////////////////////////////////////////////
// STOP
////////////////////////////////////////////////
static void play_stop(t_play *x)
{
    if (x->x_playing){
        x->x_playing = 0;
        x->x_playnew = 0;
        outlet_bang(x->x_donelet);
    };
}


static void play_calcsamp(t_play *x){
    int endsamp;
    //calculate samp values from corresponding ms values
    //use array's sample rate 
    t_float aksr = x->x_aksr;
    t_float stms = x->x_stms;
    t_float endms = x->x_endms;
    t_float rate;
    if(x->x_durms > 0){
        rate = (endms - stms)/x->x_durms;
    }
    else{
        //easy, if durms == 0, just make rate = 1 (or -1 )
        if(endms >= stms){
            rate = 1.;
        }
        else{
            rate = -1.;
        };
    };
    int isneg = rate < 0;
    x->x_rate = rate;
    x->x_isneg = isneg;
    int npts = x->x_npts; //length of array in samples
    int stsamp = x->x_stsamp = (int)(stms * aksr);
    //a bit hacky - DK
    endsamp = x->x_endsamp = endms >= SHARED_FLT_MAX/aksr ? SHARED_INT_MAX : (int)(endms * aksr);
    //some boundschecking
    if(stsamp > npts){
        stsamp = npts;
    }
    else if(stsamp < 0){
        stsamp = 0;
    };

    if(endsamp > npts){
        endsamp = npts;
    }
    else if(endsamp < 0){
        endsamp = 0;
    };

    int rangesamp = x->x_rangesamp = abs(stsamp - endsamp);
    int fadesamp = (int)(x->x_fadems * aksr);

    //boundschecking
    if(fadesamp > rangesamp){
        fadesamp = rangesamp;
    }
    else if(fadesamp < 0){
        fadesamp = 0;
    };

    int stxsamp, endxsamp;
    //is rate is neg (isneg == 1), then end pts of loop come before the loop end points in the buffer,
    //if pos, come after
    if(isneg){
        stxsamp = stsamp - fadesamp;
        endxsamp = endsamp - fadesamp;
        if(stxsamp < 0){
            stxsamp = 0;
        };
        if(endxsamp < 0){
            endxsamp = 0;
        };
    }
    else{
        stxsamp = stsamp + fadesamp;
        endxsamp = endsamp + fadesamp;
        if(stxsamp > npts){
            stxsamp = npts;
        };
        if(endxsamp >= npts){
            endxsamp = npts;
        };
    };

    x->x_stxsamp = stxsamp;
    x->x_endxsamp = endxsamp;
    x->x_fadesamp = fadesamp;
}


static void play_set(t_play *x, t_symbol *s)
{
    cybuf_setarray(x->x_cybuf, s);
    x->x_npts = x->x_cybuf->c_npts;
    x->x_ksrrat = x->x_aksr/x->x_pdksr;
}

/*static void play_arraysr(t_play *x, t_floatarg f){
    //sample rate of array in samp/sec
    if(f <= 1){
        f = 1;
    };
    x->x_aksr = f * 0.001;
    x->x_ksrrat = x->x_aksr/x->x_pdksr;
    play_calcsamp(x);
}*/

////////////////////////////////////////////////
// START
////////////////////////////////////////////////
static void play_start(t_play *x, t_symbol *s, int argc, t_atom * argv){
    s = NULL;
    //args, start pos in ms, end pos in ms, duration to take between start and end
    //one of the end points shouldn't be inclusive, mb end point
    t_float stms = 0;
    t_float endms = SHARED_FLT_MAX;
    t_float durms = 0;
    int argnum = 0;
    while(argc){
        if(argv -> a_type == A_FLOAT){
            switch(argnum){
                case 0:
                    stms = atom_getfloatarg(0, argc, argv);
                    if(stms < 0){
                        stms = 0;
                    }
                    break;
                case 1:
                    endms = atom_getfloatarg(0, argc, argv);
                    if(endms < 0){
                        endms = 0;
                    }
                    break;
                case 2:
                    durms = atom_getfloatarg(0, argc, argv);
                    break;
                default:
                    break;
            };
            argnum++;
        };
        argc--;
        argv++;
    };
    x->x_stms = stms;
    x->x_endms = endms;
    x->x_durms = durms;
    //calculate sample equivalents
    play_calcsamp(x);
    //use play_play helper func to start playing
    play_play(x, 1);
}

static void play_interptime(t_play *x, t_floatarg fadems){
    x->x_fadems = fadems;
    //calculate sample equivalents
    play_calcsamp(x);
}

////////////////////////////////////////////////
// float
////////////////////////////////////////////////
static void play_float(t_play *x, t_floatarg f){
    int playing = f > 0 ? 1 : 0;
    if(playing){
        //play whole array at original speed
        x->x_stms = 0;
        x->x_endms = SHARED_FLT_MAX;
        x->x_durms = 0;
        play_calcsamp(x);
        //use play_play helper func to start playing
        play_play(x, playing);

    }
    else{
        play_stop(x);
    };
}

////////////////////////////////////////////////
// PAUSE
////////////////////////////////////////////////
static void play_pause(t_play *x){
 //   x->x_pause = 1;
    x->x_playing = 0;
}


////////////////////////////////////////////////
// RESUME
////////////////////////////////////////////////
static void play_resume(t_play *x){
//    x->x_pause = 0;
        x->x_playing = 1;
}


////////////////////////////////////////////////
// RESUME
////////////////////////////////////////////////
static void play_loop(t_play *x, t_floatarg f){
    x->x_looping = f > 0 ? 1 : 0;
}

static void play_loopinterp(t_play *x, t_floatarg f){
    x->x_linterp = f > 0 ? 1 : 0;
}

static double play_getvecval(t_play *x, int chidx, double phase, int interp){
    int ndx;
    double output;
    t_word **vectable = x->x_cybuf->c_vectors;
    int npts = x->x_npts;
    int maxindex = (interp ? npts - 3 : npts - 1);
    if (phase < 0 || phase > maxindex)
        phase = 0;  /* CHECKED: a value 0, not ndx 0 */
    ndx = (int)phase;
    float frac,  a,  b,  c,  d, cminusb;
    /* CHECKME: what kind of interpolation? (CHECKED: multi-point) */
    if (ndx < 1)
        ndx = 1, frac = 0;
    else if (ndx > maxindex)
        ndx = maxindex, frac = 1;
    else frac = phase - ndx;
    t_word *vp = vectable[chidx];
    if (vp)
    {
        vp += ndx;
        a = vp[-1].w_float;
        b = vp[0].w_float;
        c = vp[1].w_float;
        d = vp[2].w_float;
        cminusb = c-b;
        output = b + frac * (
            cminusb - 0.1666667f * (1. - frac) * (
                (d - a - 3.0f * cminusb) * frac
                + (d + 2.0f * a - 3.0f * b)
            )
        );
    }
    else output = 0;

    return output;

}

/* LATER optimize */
static t_int *play_perform(t_int *w)
{
    t_play *x = (t_play *)(w[1]);
    t_cybuf *cybuf = x->x_cybuf;
    int nblock = (int)(w[2]);
    int nch = x->x_numchans;
    int chidx; //channel index
    if (cybuf->c_playable)
    {	
        float pdksr = x->x_pdksr;
        int interp = 1;
        int iblock;

        if(x->x_hasfeeders){
            //signal input present, indexing into array
            t_float *xin = x->x_ivec;

            for (iblock = 0; iblock < nblock; iblock++)
            {
                float phase = *xin++ * pdksr; // converts input in ms to samples!
            for(chidx=0; chidx<nch; chidx++){
                t_float *out = *(x->x_ovecs+chidx);
                out[iblock] = play_getvecval(x, chidx, phase, interp);
                };
            };
        }
        else{
            //no signal input present, auto playback mode
            if(x->x_playing){
                //post("%f", x->x_phase);
                double output;
                double gain;
                int npts = x->x_npts;
                int stsamp = x->x_stsamp;
                int endsamp = x->x_endsamp;
                int isneg = x->x_isneg;
                int looping = x->x_looping;
                int linterp = x->x_linterp;
                int rangesamp = x->x_rangesamp;
                int ramping = 0; //if we're ramping

                if(x->x_playnew){
                    //if starting to play this block 
                    x->x_phase = (double)stsamp;
                    if(looping && linterp){
                        x->x_rfirst = 1;
                        //the first ramp up shouldn't have the end of the loop fading out
                    };
                    x->x_playnew = 0; 
                };
                //let's handle this in two cases, forwards playing (isneg == 0), backwards playing (isneg == 1)
                for (iblock = 0; iblock < nblock; iblock++){
                    double phase = x->x_phase;
                    double fadephase, fadegain;
                    if(isneg){

                    //[0---endxsamp---endsamp----stxsamp----stsamp----npts]
                    //backward 
                    
                    //boundschecking for phase
                    
                        if(phase <= endsamp || phase < 0. || phase >= npts){
                            if(looping){
                                //if looping, go back to the beginning
                                phase = (double)stsamp;
                            }
                            else{
                                //we're done playing, just output zeroes!
                                x->x_playing = 0;
                                //done bang
                                outlet_bang(x->x_donelet);
                            };
                        }
                        else if(phase > stsamp){
                            phase = (double)stsamp;
                        };

                        //check looping conditions
                        if(looping && linterp){
                            //if in the xfade period, calculate gain of fading in loop
                            if(phase > x->x_stxsamp && phase <= stsamp){
                                gain = (double)(stsamp - phase)/(double)x->x_fadesamp;
                                ramping = 1;
                                if(x->x_rfirst == 0){
                                    //if not the first ramp, need the fade out ramp
                                    fadephase = phase - (double)rangesamp;
                                    fadegain = 1.-gain;

                                    //boundschecking
                                    if(fadephase < 0){
                                        fadephase = 0;
                                    }
                                    else if(fadephase >= stsamp){
                                        fadephase = (double)stsamp;
                                    };
                                    if(fadegain < 0){
                                        fadegain = 0.;
                                    };
                                };
                            }
                            else if(x->x_rfirst){
                                //if we just finished the first ramp, flag it
                                x->x_rfirst = 0;
                            };
                        };
                   
                    }
                    else{

                        //[0----stsamp---stxsamp-----endsamp---endxsamp----npts]
                        //forward 
                        
                        //boundschecking for phase
                        if(phase >= endsamp || phase < 0.|| phase >= npts){
                            if(looping){
                                //if looping, go back to the beginning
                                phase = (double)stsamp;
                            }
                            else{
                                //we're done playing, just output zeroes!
                                x->x_playing = 0;
                                //done bang
                                outlet_bang(x->x_donelet);
                            };
                        }
                        else if(phase < stsamp){
                            phase = (double)stsamp;
                        };

                        //check looping conditions
                        if(looping && linterp){
                            //if in the xfade period, calculate gain of fading in loop
                            if(phase < x->x_stxsamp && phase >= stsamp){
                                gain = (double)(phase-stsamp)/(double)x->x_fadesamp;
                                ramping = 1;
                                if(x->x_rfirst == 0){
                                    //if not the first ramp, need the fade out ramp
                                    fadephase = phase + (double)rangesamp;
                                    fadegain = 1.-gain;

                                    //boundschecking
                                    if(fadephase >= npts){
                                        fadephase = npts - 1;
                                    }
                                    else if(fadephase <= stsamp){
                                        fadephase = (double)stsamp;
                                    };
                                    if(fadegain < 0){
                                        fadegain = 0.;
                                    };
                                };
                            }
                            else if(x->x_rfirst){
                                //if we just finished the first ramp, flag it
                                x->x_rfirst = 0;
                            };
                        };
                    }; 
                        //reading output vals (for both forwards and backwards)
                        int rfirst = x->x_rfirst;
                        
                            for(chidx =0; chidx<nch;chidx++){
                                t_float *out = *(x->x_ovecs+chidx);
                                //get current val
                                if(x->x_playing){
                                    output = play_getvecval(x, chidx, phase, interp);
                                    if(ramping){
                                        output *= gain;
                                        if(!rfirst){
                                            //if not the first ramp, need the fading out ramp
                                            double out2 = play_getvecval(x, chidx, fadephase, interp);
                                            out2 *= fadegain;
                                            output += out2;
                                        };
                                    };
                                }
                                else{
                                    output = 0.;
                                };
                                out[iblock] = output;

                             };
                        //incrementation time (for both forwards and backwards)!
                            x->x_phase = phase + x->x_ksrrat*x->x_rate;
                    };
            }
            else{
                //not playing, just output zeros
                goto nullstate;
            };

        };
    }
    else
    {
        nullstate:
            for(chidx =0; chidx<nch;chidx++)
            {
                t_float *out = *(x->x_ovecs+chidx);
                int n = nblock;
                while (n--) *out++ = 0;
            };
    };
    return (w + 3);
}

static void play_dsp(t_play *x, t_signal **sp)
{
      cybuf_checkdsp(x->x_cybuf);
      int npts = x->x_cybuf->c_npts;
    x->x_hasfeeders = magic_inlet_connection((t_object *)x, x->x_glist, 0, &s_signal);
    t_float pdksr= sp[0]->s_sr * 0.001;
        x->x_pdksr = pdksr;
        x->x_ksrrat = (double)(x->x_aksr/x->x_pdksr);

    if(npts != x->x_npts){
        x->x_npts = npts;
        //need to recalculate sample equivalents
        play_calcsamp(x);

    };
    int i, nblock = sp[0]->s_n;

    t_signal **sigp = sp;
    x->x_ivec = (*sigp++)->s_vec;
    for (i = 0; i < x->x_numchans; i++){ //input vectors first
		*(x->x_ovecs+i) = (*sigp++)->s_vec;
	};
	dsp_add(play_perform, 2, x, nblock);


}

static void *play_free(t_play *x)
{
    cybuf_free(x->x_cybuf);
    freebytes(x->x_ovecs, x->x_numchans * sizeof(*x->x_ovecs));
    outlet_free(x->x_donelet);
    return (void *)x;
}

static void *play_new(t_symbol * s, int argc, t_atom * argv){
    t_symbol *arrname = s = NULL;
    t_float channels = 1;
    t_float lintime = PLAY_LINTIME; //default interp time in ms
    int looping = PLAY_LOOP; //loop default setting
    int linterp = PLAY_LINTERP; //loop interp default
    int nameset = 0; //flag if name is set
    while(argc){
        if(!nameset){
            if(argv->a_type == A_SYMBOL)
                arrname = atom_getsymbolarg(0, argc, argv);
            argc--;
            argv++;
            nameset = 1;
        }
        else{
            if(argv->a_type == A_SYMBOL){ // treat as attribute
                t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
                argc--;
                argv++;
                if(argc){
                    //if there's an arg left, parse it
                    t_float curfloat = atom_getfloatarg(0, argc, argv);
                    argc--;
                    argv++;
                    if(strcmp(cursym->s_name, "@interptime") == 0){
                        lintime = curfloat > PLAY_MINITIME ? curfloat : PLAY_MINITIME;
                    }
                    else if(strcmp(cursym->s_name,"@loop") == 0){
                        looping = curfloat > 0 ? 1 : 0;
                    }
                    else if(strcmp(cursym -> s_name, "@loopinterp") == 0){
                        linterp = curfloat > 0 ? 1 : 0;
                    }
                    else{
                        goto errstate;
                    };
                }
                else{
                    goto errstate;
                };
            }
            else{ // float
                channels = atom_getfloatarg(0, argc, argv);
                argc--;
                argv++;
            };
        };
    };
    /* one auxiliary signal:  position input */
    int chn_n = (int)channels > 64 ? 64 : (int)channels;

    t_play *x = (t_play *)pd_new(play_class);
    x->x_glist = canvas_getcurrent();
    x->x_hasfeeders = 0;
    x->x_pdksr = (float)sys_getsr() * 0.001;
    //set sample rate of array as pd's sample rate for now
    x->x_aksr = x->x_pdksr;
    x->x_cybuf = cybuf_init((t_class *)x, arrname, chn_n, 0);
    t_cybuf * c = x->x_cybuf;
    
    if (c)
    {
        int nch = c->c_numchans;
        x->x_npts = c->c_npts;
        x->x_numchans = nch;
        x->x_ovecs = getbytes(x->x_numchans * sizeof(*x->x_ovecs));
	while (nch--)
	    outlet_new((t_object *)x, &s_signal);
        x->x_donelet = outlet_new(&x->x_obj, &s_bang);
        x->x_playing = 0;
        x->x_playnew = 0;
        x->x_looping = looping;
        x->x_linterp = linterp;
        x->x_fadems = lintime;
        //defaults for start, end, and duration
        x->x_stms = 0;
        x->x_endms = SHARED_FLT_MAX;
        x->x_durms = 0;
    }
    return(x);
    errstate:
		pd_error(x, "play~: improper args");
		return NULL;
}

CYCLONE_OBJ_API void play_tilde_setup(void){
    play_class = class_new(gensym("play~"), (t_newmethod)play_new, (t_method)play_free,
            sizeof(t_play), 0, A_GIMME, 0);
    class_domainsignalin(play_class, -1);
    class_addfloat(play_class, play_float);
    class_addmethod(play_class, (t_method)play_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(play_class, (t_method)play_set, gensym("set"), A_SYMBOL, 0);
    class_addmethod(play_class, (t_method)play_stop, gensym("stop"), 0);
    class_addmethod(play_class, (t_method)play_pause, gensym("pause"), 0);
    class_addmethod(play_class, (t_method)play_resume, gensym("resume"), 0);
    class_addmethod(play_class, (t_method)play_loop, gensym("loop"), A_FLOAT, 0);
    class_addmethod(play_class, (t_method)play_interptime, gensym("interptime"), A_FLOAT, 0);
    class_addmethod(play_class, (t_method)play_loopinterp, gensym("loopinterp"), A_FLOAT, 0);
//    class_addmethod(play_class, (t_method)play_arraysr, gensym("arraysr"), A_FLOAT, 0);
    class_addmethod(play_class, (t_method)play_start, gensym("start"), A_GIMME, 0);
}
