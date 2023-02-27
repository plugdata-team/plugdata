
#include "m_pd.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>

#if _MSC_VER
#define t_complex _Dcomplex
#else
#define t_complex complex double
#endif

// TODO: some of these aren't used
#define LPHASOR      (8*sizeof(uint32_t)) // the phasor logsize
#define VOICES       8 // the number of waveform voices
#define LLENGTH      6 // the loglength of a fractional delayed basic waveform
#define LOVERSAMPLE  6 // the log of the oversampling factor (nb of fract delayed waveforms)
#define LPAD         1 // the log of the time padding factor (to reduce time aliasing)
#define LTABLE       (LLENGTH+LOVERSAMPLE)
#define N            (1<<LTABLE)
#define M            (1<<(LTABLE+LPAD))
#define S            (1<<LOVERSAMPLE)
#define L            (1<<LLENGTH)
#define CUTOFF       0.8 // fraction of nyquist for impulse cutoff
#define NBPERIODS    ((t_float)(L) * CUTOFF / 2.0)
//#define LMASK        (L-1)
//#define WALPHA       0.1 // windowing alpha (0 = cos -> 1 = rect)

// sample buffers
static t_float bli[N]; // band limited impulse
static t_float bls[N]; // band limited step
//static t_float blr[N]; // band limited ramp

t_class *blosc_class;

typedef struct _butter_state{
    // state data
    t_float d1A;
    t_float d2A;
    t_float d1B;
    t_float d2B;
    // pole data
    t_float ai;
    t_float s_ai;
    t_float ar;
    t_float s_ar;
    // zero data
    t_float c0;
    t_float s_c0;
    t_float c1;
    t_float s_c1;
    t_float c2;
    t_float s_c2;
}butter_state;

static void butter_bang_smooth (butter_state states[3], t_float input, t_float* output, t_float smooth){
    for(int i = 0; i < 3; i++){
        butter_state* s = states + i;
        t_float d1t = s->s_ar * s->d1A + s->s_ai * s->d2A + input;
        t_float d2t = s->s_ar * s->d2A - s->s_ai * s->d1A;
        s->s_ar += smooth * (s->ar - s->s_ar);
        s->s_ai += smooth * (s->ai - s->s_ai);
        *output = s->s_c0 * input + s->s_c1 * s->d1A + s->s_c2 * s->d2A;
        s->d1A = d1t;
        s->d2A = d2t;
        s->s_c0 += smooth * (s->c0 - s->s_c0);
        s->s_c1 += smooth * (s->c1 - s->s_c1);
        s->s_c2 += smooth * (s->c2 - s->s_c2);
    }
}

static void butter_init(butter_state states[3]){
    for(int i = 0; i < 3; i++){
        butter_state* s = states + i;
        s->d1A = s->d1B = s->d2A = s->d2B = 0.0;
        s->ai = s->ar = s->c0 = s->c1 = s->c2 = 0.0;
        s->s_ai = s->s_ar = s->s_c0 = s->s_c1 = s->s_c2 = 0.0;
    }
}

static t_complex complex_div_f(t_float in1, t_complex in2) {
    double real = (double)in1 / creal(in2);
    double imag = (double)in1 / cimag(in2);
    
    return (t_complex){real,imag};
}

static t_complex complex_mult(t_complex in1, t_complex in2) {
    double real = creal(in1) * creal(in2) - cimag(in1) * cimag(in2);
    double imag = creal(in1) * cimag(in2) + creal(in2) * cimag(in1);
    return (t_complex){real,imag};
}

static t_complex complex_div(t_complex in1, t_complex in2)
 {
    double real = (creal(in1) * creal(in2) + cimag(in1) * cimag(in2)) / (creal(in2) * creal(in2) + cimag(in2) * cimag(in2));
    double imag = (cimag(in1) * creal(in2) - creal(in1) * cimag(in2)) / (creal(in2) * creal(in2) + cimag(in2) * cimag(in2));
    return (t_complex){real,imag};
 }

static t_complex complex_subtract(t_complex in1, t_complex in2) {
    double real = creal(in1) - creal(in2);
    double imag = cimag(in1) - cimag(in2);
    return (t_complex){real,imag};
}


static t_complex complex_with_angle(const t_float angle){
    return (t_complex){cosf(angle), sinf(angle)};
}

static t_float complex_norm2(const t_complex x){
    return creal(x) * creal(x) + cimag(x) * cimag(x);
}
static t_float complex_norm(const t_complex x){
    return sqrt(complex_norm2(x));
}

static void set_butter_hp(butter_state states[3], t_float freq){
    //  This computes the poles for a highpass butterworth filter, transformed to the
    // digital domain using a bilinear transform. Every biquad section is normalized at NY.
    t_float epsilon = .0001; // stability guard
    t_float min = 0.0 + epsilon;
    t_float max = 0.5 - epsilon;
    int sections = 3;
    if(freq < min)
        freq = min;
    if(freq > max)
        freq = max;
    // prewarp cutoff frequency
    t_float omega = 2.0 * tan(M_PI * freq);
    t_complex pole = complex_with_angle( (2*sections + 1) * M_PI / (4*sections)); // first pole of lowpass filter with omega == 1
    t_complex pole_inc = complex_with_angle(M_PI / (2*sections)); // phasor to get to next pole, see Porat p. 331
    t_complex b = (t_complex){-1.0, 0.0}; // normalize at NY
    t_complex c = (t_complex){1.0, 0.0};  // all zeros will be at DC
    for(int i = 0; i < sections; i++){
        butter_state* s = states + i;
        // setup the biquad with the computed pole and zero and unit gain at NY
        pole = complex_mult(pole, pole_inc);            // comp next (lowpass) pole
        t_complex a = complex_div_f(omega, pole);
        s->ar = creal(a);
        s->ai = cimag(a);
        s->c0 = 1.0;
        s->c1 = 2.0 * (creal(a) - creal(b));
        s->c2 = (complex_norm2(a) - complex_norm2(b) - s->c1 * creal(a)) / cimag(a);
        t_complex invComplexGain = complex_div(
        complex_mult(complex_subtract(c, a), complex_subtract(c, conj(a))),
        complex_mult(complex_subtract(c, b), complex_subtract(c, conj(b)))
        );
        
        t_float invGain = complex_norm(invComplexGain);
        s->c0 *= invGain;
        s->c1 *= invGain;
        s->c2 *= invGain;
    }
}

typedef struct bloscctl{
    t_int c_index[VOICES];      // array of indices in sample table
    t_float c_frac[VOICES];     // array of fractional indices
    t_float c_vscale[VOICES];   // array of scale factors
    t_int c_next_voice;         // next voice to steal (round robin)
    uint32_t c_phase;                // phase of main oscillator
    uint32_t c_phase2;               // phase of secondairy oscillator
    t_float c_state;            // state of the square wave
    t_float c_prev_amp;         // previous input of comparator
    t_float c_phase_inc_scale;
    t_float c_scale;
    t_float c_scale_update;
    butter_state c_butter[3]; // the series filter
    t_symbol* c_waveshape;
}t_bloscctl;

typedef struct blosc{
    t_object x_obj;
    t_float x_f;
    t_bloscctl x_ctl;
    t_int midi;
}t_blosc;

/* phase converters */
static inline uint32_t _float_to_phase(t_float f){return ((uint32_t)(f * 4294967296.0)) & ~(S-1);}

/* flat table: better for linear interpolation */
static inline t_float _play_voice_lint(t_float *table, t_int *index, t_float frac, t_float scale){
    int i = *index;
    /* perform linear interpolation */
    t_float f = (((1.0 - frac) * table[i]) + (table[i+1] * frac)) * scale;
    /* increment phase index if next 2 elements will still be inside table
     if not there's no increment and the voice will keep playing the same sample */
    i += (((i+S+1) >> LTABLE) ^ 1) << LOVERSAMPLE;
    *index = i;
    return(f);
}

/* get one sample from the bandlimited discontinuity wavetable playback syth */
static inline t_float _get_bandlimited_discontinuity(t_bloscctl *ctl, t_float *table){
    t_float sum = 0.0;
    /* sum  all voices */
    for(int i = 0; i < VOICES; i++)
        sum += _play_voice_lint(table, ctl->c_index+i, ctl->c_frac[i], ctl->c_vscale[i]);
    return(sum);
}

// advance phasor and update waveplayers on phase wrap
static void _bang_phasor(t_bloscctl *ctl, t_float freq){
    uint32_t phase = ctl->c_phase;
    uint32_t phase_inc;
    uint32_t oldphase;
    int voice;
    t_float scale = ctl->c_scale;
    /* get increment */
    t_float inc = freq * ctl->c_phase_inc_scale;
    /* calculate new phase
     the increment (and the phase) should be a multiple of S */
    if(inc < 0.0) inc = -inc;
    phase_inc = ((uint32_t)inc) & ~(S-1);
    oldphase = phase;
    phase += phase_inc;
    /* check for phase wrap */
    if(phase < oldphase){
        uint32_t phase_inc_decimated = phase_inc >> LOVERSAMPLE;
        uint32_t table_index;
        uint32_t table_phase;
        /* steal the oldest voice if we have a phase wrap */
        voice = ctl->c_next_voice++;
        ctl->c_next_voice &= VOICES-1;
        /* determine the location of the discontinuity (in oversampled coordinates)
         which is S * (new phase) / (increment) */
        table_index = phase / phase_inc_decimated;
        /* determine the fractional part (in oversampled coordinates)
         for linear interpolation */
        table_phase = phase - (table_index * phase_inc_decimated);
        /* use it to initialize the new voice index and interpolation fraction */
        ctl->c_index[voice] = table_index;
        ctl->c_frac[voice] = (t_float)table_phase / (t_float)phase_inc_decimated;
        ctl->c_vscale[voice] = scale;
        scale = scale * ctl->c_scale_update;
    }
    /* save state */
    ctl->c_phase = phase;
    ctl->c_scale = scale;
}

/* the 2 oscillator version:
 the second osc can reset the first osc's phase (hence it determines the pitch)
 the first osc determines the waveform */
/*static void _bang_hardsync_phasor(t_bloscctl *ctl, t_float freq, t_float freq2){
    uint32_t phase = ctl->c_phase;
    uint32_t phase2 = ctl->c_phase2;
    uint32_t phase_inc;
    uint32_t phase_inc2;
    uint32_t oldphase;
    uint32_t oldphase2;
    int voice;
    t_float scale = ctl->c_scale;
    // get increment
    t_float inc = freq * ctl->c_phase_inc_scale;
    t_float inc2 = freq2 * ctl->c_phase_inc_scale;
    // calculate new phases the increment (and the phase) should be a multiple of S
    // save previous phases
    oldphase = phase;
    oldphase2 = phase2;
    // update second osc
    if(inc2 < 0.0) inc2 = -inc2;
    phase_inc2 = ((uint32_t)inc2) & ~(S-1);
    phase2 += phase_inc2;
    // update first osc (freq should be >= freq of sync osc
    if(inc < 0.0) inc = -inc;
    phase_inc = ((uint32_t)inc) & ~(S-1);
    if(phase_inc < phase_inc2) phase_inc = phase_inc2;
    phase += phase_inc;
    // check for sync discontinuity (osc 2)
    if(phase2 < oldphase2){
        // adjust phase depending on the location of the discontinuity in phase2:
        // phase/phase_inc == phase2/phase_inc2
        uint64_t pi = phase_inc >> LOVERSAMPLE;
        uint64_t pi2 = phase_inc2 >> LOVERSAMPLE;
        uint64_t lphase = ((uint64_t)phase2 * pi) / pi2;
        phase = lphase & ~(S-1);
    }
    // check for phase discontinuity (osc 1)
    if(phase < oldphase){
        uint32_t phase_inc_decimated = phase_inc >> LOVERSAMPLE;
        uint32_t table_index;
        uint32_t table_phase;
        t_float stepsize;
        // steal the oldest voice if we have a phase wrap
        voice = ctl->c_next_voice++;
        ctl->c_next_voice &= VOICES-1;
        // determine the location of the discontinuity (in oversampled coordinates)
        // which is S * (new phase) / (increment)
        table_index = phase / phase_inc_decimated;
        // determine the fractional part (in oversampled coordinates)
        // for linear interpolation
        table_phase = phase - (table_index * phase_inc_decimated);
        // determine the step size
         as opposed to saw/impulse waveforms, the step is not always equal to one. it is:
         oldphase - phase + phase_inc
         but for the unit step this will overflow to zero, so we
         reduce the bit depth to prevent overflow
        stepsize = _phase_to_float(((oldphase-phase) >> LOVERSAMPLE)
                                   + phase_inc_decimated) * (t_float)S;
        // use it to initialize the new voice index and interpolation fraction
        ctl->c_index[voice] = table_index;
        ctl->c_frac[voice] = (t_float)table_phase / (t_float)phase_inc_decimated;
        ctl->c_vscale[voice] = scale * stepsize;
        scale = scale * ctl->c_scale_update;
    }
    // save state
    ctl->c_phase = phase;
    ctl->c_phase2 = phase2;
    ctl->c_scale = scale;
}*/

/*static t_int *blosc_perform_hardsync_saw(t_int *w){
    t_float *freq     = (t_float *)(w[3]);
    t_float *freq2     = (t_float *)(w[4]);
    t_float *out      = (t_float *)(w[5]);
    t_bloscctl *ctl  = (t_bloscctl *)(w[1]);
    t_int n           = (t_int)(w[2]);
    set_butter_hp(ctl->c_butter, 0.85 * (*freq / sys_getsr()));
    while(n--){
        t_float frequency = *freq++;
        t_float frequency2 = *freq2++;
        // get the bandlimited discontinuity
        t_float sample = _get_bandlimited_discontinuity(ctl, bls);
        // add aliased sawtooth wave
        sample += _phase_to_float(ctl->c_phase) - 0.5;
        // highpass filter output to remove DC offset and low frequency aliasing
        butter_bang_smooth(ctl->c_butter, sample, &sample, 0.05);
        // send to output
        *out++ = sample;
        // advance phasor
        _bang_hardsync_phasor(ctl, frequency2, frequency);
    }
    return(w+6);
}*/

static t_int *blosc_perform_imp(t_int *w){
    t_blosc* x        = (t_blosc*)(w[1]);
    t_bloscctl *ctl   = (t_bloscctl *)(w[2]);
    t_int n           = (t_int)(w[3]);
    t_float *freq     = (t_float *)(w[4]);
    t_float *out      = (t_float *)(w[5]);
    // set postfilter cutoff
    set_butter_hp(ctl->c_butter, 0.85 * (*freq / sys_getsr()));
    while(n--){
        t_float frequency = *freq++;
        if(x->midi)
            frequency = pow(2, (frequency - 69)/12) * 440;
        t_float sample = _get_bandlimited_discontinuity(ctl, bli);
        // highpass filter output to remove DC offset and low frequency aliasing
        butter_bang_smooth(ctl->c_butter, sample, &sample, 0.05);
        *out++ = sample * 2;
        _bang_phasor(ctl, frequency); // advance phasor
    }
    return(w+6);
}

static void blosc_midi(t_blosc *x, t_floatarg f){
    x->midi = (int)(f != 0);
}

static void blosc_phase(t_blosc *x, t_float f){
    x->x_ctl.c_phase = _float_to_phase(f);
    x->x_ctl.c_phase2 = _float_to_phase(f);
}

/*static void blosc_phase1(t_blosc *x, t_float f){
    x->x_ctl.c_phase = _float_to_phase(f);
}*/

static void blosc_phase2(t_blosc *x, t_float f){
    x->x_ctl.c_phase2 = _float_to_phase(f);
}


/* CLASS DATA INIT (tables) */

/* some vector ops */

/* clear a buffer */
static inline void _clear(t_float *array, int size){
    memset(array, 0, sizeof(t_float)*size);
}

/* compute complex log */
static inline void _clog(t_float *real, t_float *imag, int size){
    for(int k = 0; k<size; k++){
        t_float r = real[k];
        t_float i = imag[k];
        t_float radius = sqrt(r*r+i*i);
        real[k] = log(radius);
        imag[k] = atan2(i,r);
    }
}

/* compute complex exp */
static inline void _cexp(t_float *real, t_float *imag, int size){
    for(int k = 0; k < size; k++){
        t_float r = exp(real[k]);
        t_float i = imag[k];
        real[k] = r * cos(i);
        imag[k] = r * sin(i);
    }
}

/* compute fft */
static inline void _fft(t_float *real, t_float *imag, int size){
    t_float scale = 1.0 / sqrt((t_float)size);
    for(int i = 0; i<size; i++){
        real[i] *= scale;
        imag[i] *= scale;
        // if(isnan(real[i])) post("fftpanic %d", i);
    }
    mayer_fft(size, real, imag);
}

/* compute ifft */
static inline void _ifft(t_float *real, t_float *imag, int size){
    t_float scale = 1.0 / sqrt((t_float)size);
    for(int i = 0; i<size; i++){
        real[i] *= scale;
        imag[i] *= scale;
        // if(isnan(real[i])) post("ifftpanic %d", i);
    }
    mayer_ifft(size, real, imag);
}

/* convert an integer index to a phase: [0 -> pi, -pi -> 0] */
static inline t_float _i2theta(int i, int size){
    t_float p = 2.0 * M_PI * (t_float)i / (t_float)size;
    if(p >= M_PI) p -= 2.0 * M_PI;
    return p;
}

/* print matlab array */
/*static void _printm(t_float *array, char *name, int size){
    fprintf(stderr, "%s = [", name);
    for(int i = 0; i<size; i++)
        fprintf(stderr, "%f;", array[i]);
    fprintf(stderr, "];\n");
}*/

/* store oversampled waveform as decimated chunks */
/*static void _store_decimated(t_float *dst, t_float *src, t_float scale, int size){
    for(int i = 0; i<size; i++){
        int offset = (i % S) * L;
        int index = i / S;
        dst[offset+index] = scale * src[i];
    }
}*/

/* store waveform as one chunk */
static void _store(t_float *dst, t_float *src, t_float scale, int size){
    for(int i = 0; i<size; i++)
        dst[i] = scale * src[i];
}

/* create a minimum phase bandlimited impulse */
static void build_tables(void){
    /* table size = M>=N (time padding to reduce time aliasing) */
    /* we work in the complex domain to eliminate the need to avoid
     negative spectral components */
    t_float real[M];
    t_float imag[M];
    t_float sum,scale;
    int i;
    /* create windowed sinc */
    _clear(imag, M);
    real[0] = 1.0;
    for(i = 1; i<M; i++){
        t_float tw = _i2theta(i,M);
        t_float ts = tw * NBPERIODS * (t_float)(M) / (t_float)(N);
        /* sinc */
        real[i] = sin(ts)/ts;
        /* blackman window */
        real[i] *= 0.42 + 0.5 * (cos(tw)) + 0.08 * (cos(2.0*tw));
        //real[i] *= 0.5f * (1.0f + WALPHA) + 0.5f * (1.0f - WALPHA) * (cos(tw));
        /* check for nan */
        //if(isnan(real[i])) post("sinc NaN panic %d", i);
        //if(isinf(real[i])) post("sinc Inf panic %d", i);
    }
    /* compute cepstrum */
    _fft(real, imag, M);
    _clog(real, imag, M);
    _ifft(real, imag, M);
    /* kill anti-causal part (contribution of non minimum phase zeros) */
    /* should we kill nyquist too ?? */
    for(i=M/2+1; i<M; i++){
        real[i] *= 0.0000;
        imag[i] *= 0.0000;
    }
    /* compute inverse cepstrum */
    _fft(real, imag, M);
    _cexp(real, imag, M);
    _ifft(real, imag, M);
    /* from here on, discard the padded part [N->M-1]
     and work with the first N samples */
    
    /* normalize impulse (integral = 1) */
    sum = 0.0;
    for(i = 0; i<N; i++){sum += real[i];}
    scale = 1.0 / sum;
    for(i = 0; i<N; i++)
        real[i] *= scale;
    /* store bli table */
    _store(bli, real, (t_float)S, N);
    //_printm(bli, "h", N);
    /* integrate impulse and invert to produce a step function
     from 1->0 */
    sum = 0.0;
    for(i = 0; i < N; i++){
        sum += real[i];
        real[i] = (1.0 - sum);
    }
    /* store decimated bls tables */
    _store(bls, real, 1.0, N);
}

static void blosc_dsp(t_blosc *x, t_signal **sp){
    // set sampling rate scaling for phasors
    x->x_ctl.c_phase_inc_scale = 4.0 * ((t_float)(1<<(LPHASOR-2))) / sys_getsr();
    x->x_ctl.c_phase_inc_scale *= 2;
    x->x_ctl.c_scale = 1.0;
    x->x_ctl.c_scale_update = -1.0;
    dsp_add(blosc_perform_imp, 5, x, &x->x_ctl, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *blosc_new(t_symbol *s, int ac, t_atom *av){
    t_blosc *x = (t_blosc *)pd_new(blosc_class);
    x->midi = 0;

    butter_init(x->x_ctl.c_butter);
    // init oscillators
    for(int i = 0; i < VOICES; i++){
      x->x_ctl.c_index[i] = N-2;
      x->x_ctl.c_frac[i] = 0.0;
    }
    // init rest of state data
    blosc_phase(x, 0);
    blosc_phase2(x, 0);
    x->x_ctl.c_state = 0.0;
    x->x_ctl.c_prev_amp = 0.0;
    x->x_ctl.c_next_voice = 0;
    x->x_ctl.c_scale = 1.0;
    x->x_ctl.c_scale_update = 1.0;
    x->x_ctl.c_waveshape = s;
    
    if(ac && av->a_type == A_SYMBOL){
        if(atom_getsymbol(av) == gensym("-midi"))
            x->midi = 1;
        ac--, av++;
    }
    if(ac && av->a_type == A_FLOAT){
        x->x_f = av->a_w.w_float;
        ac--; av++;
    }
    
    outlet_new(&x->x_obj, gensym("signal"));
    return(void *)x;
}

void setup_bl0x2eimp2_tilde(void){
    build_tables();
    blosc_class = class_new(gensym("bl.imp2~"), (t_newmethod)blosc_new,
        0, sizeof(t_blosc), 0, A_GIMME, A_NULL);
    CLASS_MAINSIGNALIN(blosc_class, t_blosc, x_f);
    class_addmethod(blosc_class, (t_method)blosc_midi, gensym("midi"), A_DEFFLOAT, 0);
    class_addmethod(blosc_class, (t_method)blosc_dsp, gensym("dsp"), A_NULL);
}
