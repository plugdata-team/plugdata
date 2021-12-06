// simplified from cyclone matrix~

#include <string.h>
#include <math.h>
#include "m_pd.h"

#define mtx_DEFGAIN  1.
#define mtx_DEFfade  10.

#define mtx_GAINEPSILON  1e-20f
#define mtx_MINfade      .001  /* LATER rethink */

#define mtx_MININLETS 1
#define mtx_MAXINLETS 512
#define mtx_MINOUTLETS 1
#define mtx_MAXOUTLETS 512

typedef struct _mtx{
    t_object     x_obj;
    int        x_numinlets;
    int        x_numoutlets;
    int        x_nblock;
    int        x_maxblock;
    t_float  **x_ivecs;
    t_float  **x_ovecs;
    t_float  **x_osums;
    int        x_ncells;
    int       *x_cells;
    t_outlet  *x_dumpout;
    /* The following fields are specific to nonbinary mode, i.e. we keep them
       unallocated in binary mode.  This is CHECKED to be incompatible:  c74
       always accepts (and reports) gains and fades, although they are actually
       meaningless in binary mode, and switching modes is not supported. */
    float      x_defgain; // gain given as argument
    float     *x_gains;  /* target gains */
    float      x_deffade;
    float     *x_fades;
    float      x_ksr;
    float     *x_coefs;  /* current coefs */
    float     *x_incrs;
    float     *x_bigincrs;
    int       *x_remains;

} t_mtx;

typedef void (*t_mtx_cellfn)(t_mtx *x, int indx, int ondx,
                int onoff, float gain);

static t_class *mtx_class;

/* called only in nonbinary mode;  LATER deal with changing nblock/ksr */
static void mtx_retarget(t_mtx *x, int cellndx){
    float target = (x->x_cells[cellndx] ? x->x_gains[cellndx] : 0.);
    if (x->x_fades[cellndx] < mtx_MINfade){
        x->x_coefs[cellndx] = target;
        x->x_remains[cellndx] = 0;
    }
    else{
        x->x_remains[cellndx] =
        x->x_fades[cellndx] * x->x_ksr + 0.5;  /* LATER rethink */
        x->x_incrs[cellndx] =
            (target - x->x_coefs[cellndx]) / (float)x->x_remains[cellndx];
        x->x_bigincrs[cellndx] = x->x_nblock * x->x_incrs[cellndx];
    }
}

/* called only in nonbinary mode;  LATER deal with changing nblock/ksr */
static void mtx_retarget_connect(t_mtx *x, int cellndx){
    float target = (x->x_cells[cellndx] ? x->x_gains[cellndx] = x->x_defgain : 0.);
    if(x->x_fades[cellndx] < mtx_MINfade){
        x->x_coefs[cellndx] = target;
        x->x_remains[cellndx] = 0;
    }
    else{
        x->x_remains[cellndx] =
        x->x_fades[cellndx] * x->x_ksr + 0.5;  /* LATER rethink */
        x->x_incrs[cellndx] =
        (target - x->x_coefs[cellndx]) / (float)x->x_remains[cellndx];
        x->x_bigincrs[cellndx] = x->x_nblock * x->x_incrs[cellndx];
    }
}

static void mtx_float(t_mtx *x, t_float f){
    pd_error(x, "mtx~: no method for float");
}

static void mtx_list(t_mtx *x, t_symbol *s, int argc, t_atom *argv){
    int inlet_idx, outlet_idx, cell_idx, onoff;
    float gain, fade;
    //init vals
    inlet_idx = 0;
    outlet_idx = 0;
    cell_idx = 0;
    onoff = 0;
    gain = 0;
    fade = 0;
    if (argc < 3) //ignore if less than 3 args
        return;
    int argnum = 0;
    while(argc > 0){ //argument parsing
        t_float argval = 0; //if not float, set equal to 0, else get value
        if(argv -> a_type == A_FLOAT)
            argval = atom_getfloatarg(0, argc, argv);
        switch(argnum){ //if more than 4 args, then just ignore;
            case 0:
                inlet_idx = (int)argval;
                break;
            case 1:
                outlet_idx = (int)argval;
                break;
            case 2:
                gain = argval != 0;
                break;
            default:
                break;
        };
        argnum++;
        argc--;
        argv++;
    };
    // bound check
    if(inlet_idx < 0 || inlet_idx >= x->x_numinlets){
        pd_error(x, "mtx~: %d is not a valid inlet index!", inlet_idx);
        return;
    };
    if(outlet_idx < 0 || outlet_idx >= x->x_numoutlets){
        pd_error(x, "mtx~: %d is not a valid outlet index!", outlet_idx);
        return;
    };
    cell_idx = inlet_idx * x->x_numoutlets + outlet_idx;
    //negative gain used in nonbinary mode, accepted as 1 in binary (legacy code)
    onoff = (gain < -mtx_GAINEPSILON || gain > mtx_GAINEPSILON);
    x->x_cells[cell_idx] = onoff;
    if(x->x_gains){ //if in nonbinary mode
        if (onoff)
            x->x_gains[cell_idx] = gain;
        mtx_retarget(x, cell_idx);
    };
}

static void mtx_clear(t_mtx *x){
    for(int i = 0; i < x->x_ncells; i++){
        x->x_cells[i] = 0;
        if (x->x_gains)
            mtx_retarget(x, i);
    }
}

static void mtx_fade(t_mtx *x, t_floatarg f){
    if (x->x_fades){
        int i;
        x->x_deffade = (f < mtx_MINfade ? 0. : f); // cell-specific fades are lost
        for (i = 0; i < x->x_ncells; i++)
            x->x_fades[i] = x->x_deffade;
    }
}

static t_int *mtx_perform(t_int *w){
    t_mtx *x = (t_mtx *)(w[1]);
    int nblock = (int)(w[2]);
    t_float **ivecs = x->x_ivecs;
    t_float **ovecs = x->x_ovecs;
    t_float **osums = x->x_osums;
    int *cellp = x->x_cells;
    float *gainp = x->x_gains;
    float *coefp = x->x_coefs;
    float *incrp = x->x_incrs;
    float *bigincrp = x->x_bigincrs;
    int *nleftp = x->x_remains;
    int indx;
    for (indx = 0; indx < x->x_numinlets; indx++){
    t_float *ivec = *ivecs++;
    t_float **ovecp = osums;

    int ondx = x->x_numoutlets;
    while (ondx--){
        t_float *in = ivec;
        t_float *out = *ovecp;
        float nleft = *nleftp;
        int sndx = nblock;
        if (nleft >= nblock){
            float coef = *coefp;
            float incr = *incrp;
            if((*nleftp -= nblock) == 0)
                *coefp = (*cellp ? *gainp : 0.);
            else
                *coefp += *bigincrp;
            while (sndx--)
                    *out++ += *in++ * coef, coef += incr;
        }
        else if (nleft > 0){
            float coef = *coefp;
            float incr = *incrp;
            sndx -= nleft;
            do{
                *out++ += *in++ * coef, coef += incr;
            }while (--nleft);
            if (*cellp){
                coef = *coefp = *gainp;
                while (sndx--)
                    *out++ += *in++ * coef;
            }
            else
                *coefp = 0.;
            *nleftp = 0;
        }
        else if (*cellp){
            float coef = *coefp;
            while (sndx--)
                *out++ += *in++ * coef;
        }
        cellp++;
        ovecp++;
        gainp++;
        coefp++;
        incrp++;
        bigincrp++;
        nleftp++;
    }
    }
    osums = x->x_osums;
    indx = x->x_numoutlets;
    while (indx--){
        t_float *in = *osums++;
        t_float *out = *ovecs++;
        int sndx = nblock;
        while (sndx--){
            *out++ = *in;
            *in++ = 0.;
        }
    }
    return(w + 3);
}

static void mtx_dsp(t_mtx *x, t_signal **sp){
    int i, nblock = sp[0]->s_n;
    t_float **vecp = x->x_ivecs;
    t_signal **sigp = sp;
    for(i = 0; i < x->x_numinlets; i++)
        *vecp++ = (*sigp++)->s_vec;
    vecp = x->x_ovecs;
    for(i = 0; i < x->x_numoutlets; i++)
        *vecp++ = (*sigp++)->s_vec;
    if(nblock != x->x_nblock){
        if(nblock > x->x_maxblock){
            size_t oldsize = x->x_maxblock * sizeof(*x->x_osums[i]),
            newsize = nblock * sizeof(*x->x_osums[i]);
            for(i = 0; i < x->x_numoutlets; i++)
                x->x_osums[i] = resizebytes(x->x_osums[i], oldsize, newsize);
            x->x_maxblock = nblock;
        };
        x->x_nblock = nblock;
    }
    x->x_ksr = sp[0]->s_sr * .001;
    dsp_add(mtx_perform, 2, x, nblock);
}

static void mtx_cellout(t_mtx *x, int indx, int ondx,
               int onoff, float gain)
{
    t_atom atout[3];
    SETFLOAT(&atout[0], (t_float)indx);
    SETFLOAT(&atout[1], (t_float)ondx);
    if (onoff)
        SETFLOAT(&atout[2], gain);
    else
        SETFLOAT(&atout[2], 0.);
    outlet_list(x->x_dumpout, &s_list, 3, atout);
}

static void mtx_cellprint(t_mtx *x, int indx, int ondx,
                 int onoff, float gain)
{
    post("%d %d %g", indx, ondx, (onoff ? gain : 0.));
}


static void mtx_report(t_mtx *x, float *gains, float defgain,
              t_mtx_cellfn cellfn)
{
    if (gains){
        int *cellp = x->x_cells;
        float *gp = gains;
        int indx, ondx;
        for (indx = 0; indx < x->x_numinlets; indx++)
            for (ondx = 0; ondx < x->x_numoutlets; ondx++, cellp++, gp++)
            /* CHECKED all cells are printed */
            (*cellfn)(x, indx, ondx, *cellp, *gp);
    }
    else{  /* CHECKED incompatible: gains confusingly printed in binary mode */
        int *cellp = x->x_cells;
        int indx, ondx;
        for (indx = 0; indx < x->x_numinlets; indx++)
            for (ondx = 0; ondx < x->x_numoutlets; ondx++, cellp++)
            /* CHECKED all cells are printed */
            (*cellfn)(x, indx, ondx, *cellp, defgain);
    }
}

static void mtx_dump(t_mtx *x){
    mtx_report(x, x->x_coefs, 1., mtx_cellout);
}

static void mtx_dumptarget(t_mtx *x){
    mtx_report(x, x->x_gains, 1., mtx_cellout);
}

static void mtx_print(t_mtx *x){
    mtx_report(x, x->x_coefs, 1., mtx_cellprint);
}

static void *mtx_free(t_mtx *x){
    if (x->x_ivecs)
    freebytes(x->x_ivecs, x->x_numinlets * sizeof(*x->x_ivecs));
    if (x->x_ovecs)
    freebytes(x->x_ovecs, x->x_numoutlets * sizeof(*x->x_ovecs));
    if (x->x_osums){
    int i;
    for (i = 0; i < x->x_numoutlets; i++)
        freebytes(x->x_osums[i], x->x_maxblock * sizeof(*x->x_osums[i]));
//    freebytes(x->x_zerovec, x->x_maxblock * sizeof(*x->x_zerovec));
    freebytes(x->x_osums, x->x_numoutlets * sizeof(*x->x_osums));
    }
    if (x->x_cells)
    freebytes(x->x_cells, x->x_ncells * sizeof(*x->x_cells));
    if (x->x_gains)
    freebytes(x->x_gains, x->x_ncells * sizeof(*x->x_gains));
    if (x->x_fades)
    freebytes(x->x_fades, x->x_ncells * sizeof(*x->x_fades));
    if (x->x_coefs)
    freebytes(x->x_coefs, x->x_ncells * sizeof(*x->x_coefs));
    if (x->x_incrs)
    freebytes(x->x_incrs, x->x_ncells * sizeof(*x->x_incrs));
    if (x->x_bigincrs)
    freebytes(x->x_bigincrs, x->x_ncells * sizeof(*x->x_bigincrs));
    if (x->x_remains)
    freebytes(x->x_remains, x->x_ncells * sizeof(*x->x_remains));
    return (void *)x;
}

static void *mtx_new(t_symbol *s, int argc, t_atom *argv){
    t_mtx *x = (t_mtx *)pd_new(mtx_class);
    t_float fadeval = mtx_DEFfade;
    x->x_numinlets = (int)mtx_MININLETS;
    x->x_numoutlets = (int)mtx_MINOUTLETS;
    x->x_defgain = mtx_DEFGAIN;
    int i;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    if(argval < mtx_MININLETS){
                        x->x_numinlets = (int)mtx_MININLETS;
                    }
                    else if (argval > mtx_MAXINLETS){
                        x->x_numinlets = (int)mtx_MAXINLETS;
                        post("mtx~: resizing to %d signal inlets", (int)mtx_MAXINLETS);
                    }
                    else{
                        x->x_numinlets = (int)argval;
                    };
                    break;
                case 1:
                    if(argval < mtx_MINOUTLETS){
                        x->x_numoutlets = (int)mtx_MINOUTLETS;
                    }
                    else if (argval > mtx_MAXOUTLETS){
                        x->x_numoutlets = (int)mtx_MAXOUTLETS;
                        post("mtx~: resizing to %d signal outlets", (int)mtx_MAXOUTLETS);
                    }
                    else{
                        x->x_numoutlets = (int)argval;
                    };
                    break;
                case 2:
                    fadeval = argval;
                    break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        }
        else if(argv -> a_type == A_SYMBOL){
                goto errstate;
        }
        else
            goto errstate;
    };
    x->x_ncells = x->x_numinlets * x->x_numoutlets;
    x->x_ivecs = getbytes(x->x_numinlets * sizeof(*x->x_ivecs));
    x->x_ovecs = getbytes(x->x_numoutlets * sizeof(*x->x_ovecs));
    x->x_nblock = x->x_maxblock = sys_getblksize();
    x->x_osums = getbytes(x->x_numoutlets * sizeof(*x->x_osums));
    for (i = 0; i < x->x_numoutlets; i++)
        x->x_osums[i] = getbytes(x->x_maxblock * sizeof(*x->x_osums[i]));
    x->x_cells = getbytes(x->x_ncells * sizeof(*x->x_cells));
    mtx_clear(x);
        x->x_gains = getbytes(x->x_ncells * sizeof(*x->x_gains));
        for (i = 0; i < x->x_ncells; i++)
            x->x_gains[i] = x->x_defgain;
        x->x_fades = getbytes(x->x_ncells * sizeof(*x->x_fades));
        mtx_fade(x, fadeval);
        x->x_coefs = getbytes(x->x_ncells * sizeof(*x->x_coefs));
        for(i = 0; i < x->x_ncells; i++)
            x->x_coefs[i] = 0.;
        x->x_ksr = sys_getsr() * .001;
        x->x_incrs = getbytes(x->x_ncells * sizeof(*x->x_incrs));
        x->x_bigincrs = getbytes(x->x_ncells * sizeof(*x->x_bigincrs));
        x->x_remains = getbytes(x->x_ncells * sizeof(*x->x_remains));
        for (i = 0; i < x->x_ncells; i++){
            x->x_remains[i] = 0;
        };
    for (i = 1; i < x->x_numinlets; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    };
    for (i = 0; i < x->x_numoutlets; i++){
         outlet_new(&x->x_obj, gensym("signal"));
    };
    x->x_dumpout = outlet_new((t_object *)x, &s_list);
    return (x);
    errstate:
        pd_error(x, "mtx~: improper args");
        return NULL;
}

void mtx_tilde_setup(void){
    mtx_class = class_new(gensym("mtx~"), (t_newmethod)mtx_new,
        (t_method)mtx_free, sizeof(t_mtx), 0, A_GIMME, 0);
    class_addmethod(mtx_class, nullfn, gensym("signal"), 0);
    class_addfloat(mtx_class, mtx_float);
    class_addlist(mtx_class, mtx_list);
    class_addmethod(mtx_class, (t_method)mtx_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(mtx_class, (t_method)mtx_clear, gensym("clear"), 0);
    class_addmethod(mtx_class, (t_method)mtx_fade, gensym("fade"), A_FLOAT, 0);
    class_addmethod(mtx_class, (t_method)mtx_dump, gensym("dump"), 0);
    class_addmethod(mtx_class, (t_method)mtx_print, gensym("print"), 0);
}
