// porres

#include "m_pd.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

typedef struct _bicoeff2{
    t_object  x_obj;
    t_float   x_freq;
    t_float   x_q_s;
    t_float   x_gain;
    t_float   x_type;
}t_bicoeff2;

static t_class *bicoeff2_class;

static void bicoeff2_bang(t_bicoeff2 *x){
    t_atom at[5];
    double a0 = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0;
// off
    if(x->x_type == 0){
        a0 = 1.;
        a1 = a2 = b1 = b2 = 0.;
    }
// allpass
    else if (x->x_type == 1){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1 - alphaQ) * g/b0;
        a1 = -2*cos_w  * g/b0;
        a2 = g;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// bandpass
    else if (x->x_type == 2){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = alphaQ  * g/b0;
        a1 = 0.;
        a2 = -a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// bandstop
    else if (x->x_type == 3){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = g / b0;
        a1 = -2*cos_w * g/b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// eq
    else if (x->x_type == 4){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ/amp + 1;
        a0 = (1 + alphaQ*amp) / b0;
        a1 = -2*cos_w / b0;
        a2 = (1 - alphaQ*amp) / b0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ/amp - 1) / b0;
    }
// highpass
    else if (x->x_type == 5){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1+cos_w)  * g/(2*b0);
        a1 = -(1+cos_w)  * g/b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// highshelf
    else if (x->x_type == 6){
        double omega, alphaS, cos_w, b0;
        double hz = (double)x->x_freq;
        double slope = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (slope < 0.000001)
            slope = 0.000001;
        if (slope > 1)
            slope = 1;
        omega = hz * PI/nyq;
        
        alphaS = sin(omega) * sqrt((amp*amp + 1) * (1/slope - 1) + 2*amp);
        cos_w = cos(omega);
        b0 = (amp+1) - (amp-1)*cos_w + alphaS;
        a0 = amp*(amp+1 + (amp-1)*cos_w + alphaS) / b0;
        a1 = -2*amp*(amp-1 + (amp+1)*cos_w) / b0;
        a2 = amp*(amp+1 + (amp-1)*cos_w - alphaS) / b0;
        b1 = -2*(amp-1 - (amp+1)*cos_w) / b0;
        b2 = -(amp+1 - (amp-1)*cos_w - alphaS) / b0;
    }
// lowpass
    else if (x->x_type == 7){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1-cos_w)  * g/(2*b0);
        a1 = (1-cos_w)  * g/b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// lowshelf
    else if (x->x_type == 8){
        double omega, alphaS, cos_w, b0;
        double hz = (double)x->x_freq;
        double slope = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (slope < 0.000001)
            slope = 0.000001;
        if (slope > 1)
            slope = 1;
        omega = hz * PI/nyq;
        
        alphaS = sin(omega) * sqrt((amp*amp + 1) * (1/slope - 1) + 2*amp);
        cos_w = cos(omega);
        b0 = (amp+1) + (amp-1)*cos_w + alphaS;
        a0 = amp*(amp+1 - (amp-1)*cos_w + alphaS) / b0;
        a1 = 2*amp*(amp-1 - (amp+1)*cos_w) / b0;
        a2 = amp*(amp+1 - (amp-1)*cos_w - alphaS) / b0;
        b1 = 2*(amp-1 + (amp+1)*cos_w) / b0;
        b2 = -(amp+1 + (amp-1)*cos_w - alphaS) / b0;
    }
// resonant
    else if(x->x_type == 9){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double g = pow(10, x->x_gain / 20);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
    
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = alphaQ*q  * g/b0;
        a1 = 0;
        a2 = -a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;  
    }
    SETFLOAT(at, b1);
    SETFLOAT(at+1, b2);
    SETFLOAT(at+2, a0);
    SETFLOAT(at+3, a1);
    SETFLOAT(at+4, a2);
    outlet_list(x->x_obj.ob_outlet, &s_list, 5, at);
}

void bicoeff2_allpass(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 1;
    bicoeff2_bang(x);
}

void bicoeff2_bandpass(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 2;
    bicoeff2_bang(x);
}

void bicoeff2_bandstop(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 3;
    bicoeff2_bang(x);
}

void bicoeff2_eq(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 4;
    bicoeff2_bang(x);
}

void bicoeff2_highpass(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 5;
    bicoeff2_bang(x);
}

void bicoeff2_highshelf(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 6;
    bicoeff2_bang(x);
}

void bicoeff2_lowpass(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 7;
    bicoeff2_bang(x);
}

void bicoeff2_lowshelf(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 8;
    bicoeff2_bang(x);
}

void bicoeff2_resonant(t_bicoeff2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    x->x_type = 9;
    bicoeff2_bang(x);
}

/////////////////////////////////

static void bicoeff2_freq(t_bicoeff2 *x, t_floatarg val){
    x->x_freq = val;
    bicoeff2_bang(x);
}

static void bicoeff2_list(t_bicoeff2 *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    if(!ac){
        bicoeff2_bang(x);
        return;
    }
    if(ac == 1){
        if(av->a_type == A_FLOAT)
            bicoeff2_freq(x, atom_getfloat(av));
        return;
    }
    int argnum = 0; //current argument
    while(ac){
        if(av -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_freq = curf;
                    break;
                case 1:
                    x->x_q_s = curf;
                    break;
                case 2:
                    x->x_gain = curf;
                default:
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    bicoeff2_bang(x);
}

static void bicoeff2_Q_S(t_bicoeff2 *x, t_floatarg val){
    x->x_q_s = val;
    bicoeff2_bang(x);
}

static void bicoeff2_gain(t_bicoeff2 *x, t_floatarg val){
    x->x_gain = val;
    bicoeff2_bang(x);
}

static void *bicoeff2_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_bicoeff2 *x = (t_bicoeff2 *)pd_new(bicoeff2_class);
    float freq = 0;
    float q_or_s = 1;
    float db = 0;
    int type = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(ac > 0){
        if (av -> a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, ac, av);
            if(strcmp(curarg->s_name, "allpass")==0){
                type = 1;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "bandpass")==0){
                type = 2;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "bandstop")==0){
                type = 3;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "eq")==0){
                type = 4;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "highpass")==0){
                type = 5;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "highshelf")==0){
                type = 6;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "lowpass")==0){
                type = 7;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "lowshelf")==0){
                type = 8;
                ac--;
                av++;
            }
            else if(strcmp(curarg->s_name, "resonant")==0){
                type = 9;
                ac--;
                av++;
            }
            else
                goto errstate;
        }
        if(av -> a_type == A_FLOAT) {
            t_float aval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    freq = aval;
                    break;
                case 1:
                    q_or_s = aval;
                    break;
                case 2:
                    db = aval;
                    break;
                default:
                    break;
                };
            argnum++;
            ac--;
            av++;
            }
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_freq = freq;
    x->x_q_s = q_or_s;
    x->x_gain = db;
    x->x_type = type;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("qs"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("gain"));
    outlet_new((t_object *)x, &s_list);
    return (x);
    errstate:
        pd_error(x, "[bicoeff2]: improper args");
        return(NULL);
}

void bicoeff2_setup(void){
    bicoeff2_class = class_new(gensym("bicoeff2"), (t_newmethod)bicoeff2_new, 0,
        sizeof(t_bicoeff2), 0, A_GIMME, 0);
    class_addlist(bicoeff2_class, (t_method)bicoeff2_list);
    class_addmethod(bicoeff2_class, (t_method)bicoeff2_Q_S, gensym("qs"), A_FLOAT, 0);
    class_addmethod(bicoeff2_class, (t_method)bicoeff2_gain, gensym("gain"), A_FLOAT, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_lowpass, gensym("lowpass"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_highpass, gensym("highpass"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_bandpass, gensym("bandpass"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_bandstop, gensym("bandstop"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_resonant, gensym("resonant"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_eq, gensym("eq"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_lowshelf, gensym("lowshelf"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_highshelf, gensym("highshelf"), A_GIMME, 0);
    class_addmethod(bicoeff2_class, (t_method) bicoeff2_allpass, gensym("allpass"), A_GIMME, 0);
}
