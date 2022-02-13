/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <s_net.h>
#include <string.h>
#include <assert.h>
#include "x_libpd_multi.h"

static t_class *libpd_multi_receiver_class;

typedef struct _libpd_multi_receiver
{
    t_object    x_obj;
    t_symbol*   x_sym;
    void*       x_ptr;

    t_libpd_multi_banghook      x_hook_bang;
    t_libpd_multi_floathook     x_hook_float;
    t_libpd_multi_symbolhook    x_hook_symbol;
    t_libpd_multi_listhook      x_hook_list;
    t_libpd_multi_messagehook   x_hook_message;
} t_libpd_multi_receiver;

static void libpd_multi_receiver_bang(t_libpd_multi_receiver *x)
{
    if(x->x_hook_bang)
        x->x_hook_bang(x->x_ptr, x->x_sym->s_name);
}

static void libpd_multi_receiver_float(t_libpd_multi_receiver *x, t_float f)
{
    if(x->x_hook_float)
        x->x_hook_float(x->x_ptr, x->x_sym->s_name, f);
}

static void libpd_multi_receiver_symbol(t_libpd_multi_receiver *x, t_symbol *s)
{
    if(x->x_hook_symbol)
        x->x_hook_symbol(x->x_ptr, x->x_sym->s_name, s->s_name);
}

static void libpd_multi_receiver_list(t_libpd_multi_receiver *x, t_symbol *s, int argc, t_atom *argv)
{
    if(x->x_hook_list)
        x->x_hook_list(x->x_ptr, x->x_sym->s_name, argc, argv);
}

static void libpd_multi_receiver_anything(t_libpd_multi_receiver *x, t_symbol *s, int argc, t_atom *argv)
{
    if(x->x_hook_message)
        x->x_hook_message(x->x_ptr, x->x_sym->s_name, s->s_name, argc, argv);
}

static void libpd_multi_receiver_free(t_libpd_multi_receiver *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static void libpd_multi_receiver_setup(void)
{
    sys_lock();
    libpd_multi_receiver_class = class_new(gensym("libpd_multi_receiver"), (t_newmethod)NULL, (t_method)libpd_multi_receiver_free,
                                           sizeof(t_libpd_multi_receiver), CLASS_DEFAULT, A_NULL, 0);
    class_addbang(libpd_multi_receiver_class, libpd_multi_receiver_bang);
    class_addfloat(libpd_multi_receiver_class,libpd_multi_receiver_float);
    class_addsymbol(libpd_multi_receiver_class, libpd_multi_receiver_symbol);
    class_addlist(libpd_multi_receiver_class, libpd_multi_receiver_list);
    class_addanything(libpd_multi_receiver_class, libpd_multi_receiver_anything);
    sys_unlock();
}

void* libpd_multi_receiver_new(void* ptr, char const *s,
                               t_libpd_multi_banghook hook_bang,
                               t_libpd_multi_floathook hook_float,
                               t_libpd_multi_symbolhook hook_symbol,
                               t_libpd_multi_listhook hook_list,
                               t_libpd_multi_messagehook hook_message)
{

    t_libpd_multi_receiver *x = (t_libpd_multi_receiver *)pd_new(libpd_multi_receiver_class);
    if(x)
    {
        sys_lock();
        x->x_sym = gensym(s);
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, x->x_sym);
        x->x_ptr = ptr;
        x->x_hook_bang = hook_bang;
        x->x_hook_float = hook_float;
        x->x_hook_symbol = hook_symbol;
        x->x_hook_list = hook_list;
        x->x_hook_message = hook_message;
    }
    return x;
}


static t_class *libpd_multi_midi_class;

typedef struct _libpd_multi_midi
{
    t_object    x_obj;
    void*       x_ptr;

    t_libpd_multi_noteonhook            x_hook_noteon;
    t_libpd_multi_controlchangehook     x_hook_controlchange;
    t_libpd_multi_programchangehook     x_hook_programchange;
    t_libpd_multi_pitchbendhook         x_hook_pitchbend;
    t_libpd_multi_aftertouchhook        x_hook_aftertouch;
    t_libpd_multi_polyaftertouchhook    x_hook_polyaftertouch;
    t_libpd_multi_midibytehook          x_hook_midibyte;
} t_libpd_multi_midi;

static void libpd_multi_noteon(int channel, int pitch, int velocity)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_noteon)
    {
        x->x_hook_noteon(x->x_ptr, channel, pitch, velocity);
    }
}

static void libpd_multi_controlchange(int channel, int controller, int value)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_controlchange)
    {
        x->x_hook_controlchange(x->x_ptr, channel, controller, value);
    }
}

static void libpd_multi_programchange(int channel, int value)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_programchange)
    {
        x->x_hook_programchange(x->x_ptr, channel, value);
    }
}

static void libpd_multi_pitchbend(int channel, int value)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_pitchbend)
    {
        x->x_hook_pitchbend(x->x_ptr, channel, value);
    }
}

static void libpd_multi_aftertouch(int channel, int value)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_aftertouch)
    {
        x->x_hook_aftertouch(x->x_ptr, channel, value);
    }
}

static void libpd_multi_polyaftertouch(int channel, int pitch, int value)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_polyaftertouch)
    {
        x->x_hook_polyaftertouch(x->x_ptr, channel, pitch, value);
    }
}

static void libpd_multi_midibyte(int port, int byte)
{
    t_libpd_multi_midi* x = (t_libpd_multi_midi*)gensym("#libpd_multi_midi")->s_thing;
    if(x && x->x_hook_midibyte)
    {
        x->x_hook_midibyte(x->x_ptr, port, byte);
    }
}

static void libpd_multi_midi_free(t_libpd_multi_midi *x)
{
    pd_unbind(&x->x_obj.ob_pd, gensym("#libpd_multi_midi"));
}

static void libpd_multi_midi_setup(void)
{
    sys_lock();
    libpd_multi_midi_class = class_new(gensym("libpd_multi_midi"), (t_newmethod)NULL, (t_method)libpd_multi_midi_free,
                                       sizeof(t_libpd_multi_midi), CLASS_DEFAULT, A_NULL, 0);
    sys_unlock();
}

void* libpd_multi_midi_new(void* ptr,
                           t_libpd_multi_noteonhook hook_noteon,
                           t_libpd_multi_controlchangehook hook_controlchange,
                           t_libpd_multi_programchangehook hook_programchange,
                           t_libpd_multi_pitchbendhook hook_pitchbend,
                           t_libpd_multi_aftertouchhook hook_aftertouch,
                           t_libpd_multi_polyaftertouchhook hook_polyaftertouch,
                           t_libpd_multi_midibytehook hook_midibyte)
{

    t_libpd_multi_midi *x = (t_libpd_multi_midi *)pd_new(libpd_multi_midi_class);
    if(x)
    {
        sys_lock();
        t_symbol* s = gensym("#libpd_multi_midi");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook_noteon        = hook_noteon;
        x->x_hook_controlchange = hook_controlchange;
        x->x_hook_programchange = hook_programchange;
        x->x_hook_pitchbend     = hook_pitchbend;
        x->x_hook_aftertouch    = hook_aftertouch;
        x->x_hook_polyaftertouch= hook_polyaftertouch;
        x->x_hook_midibyte      = hook_midibyte;
    }
    return x;
}


static t_class *libpd_multi_print_class;

typedef struct _libpd_multi_print
{
    t_object    x_obj;
    void*       x_ptr;
    t_libpd_multi_printhook x_hook;
} t_libpd_multi_print;

static void libpd_multi_print(const char* message)
{
    t_libpd_multi_print* x = (t_libpd_multi_print*)gensym("#libpd_multi_print")->s_thing;
    if(x && x->x_hook)
    {
        x->x_hook(x->x_ptr, message);
    }
}

static void libpd_multi_print_setup(void)
{
    sys_lock();
    libpd_multi_print_class = class_new(gensym("libpd_multi_print"), (t_newmethod)NULL, (t_method)NULL,
                                       sizeof(t_libpd_multi_print), CLASS_DEFAULT, A_NULL, 0);
    sys_unlock();
}

void* libpd_multi_print_new(void* ptr, t_libpd_multi_printhook hook_print)
{
 
    t_libpd_multi_print *x = (t_libpd_multi_print *)pd_new(libpd_multi_print_class);
    if(x)
    {
        sys_lock();
        t_symbol* s = gensym("#libpd_multi_print");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook = hook_print;
    }
    return x;
}


// font char metric triples: pointsize width(pixels) height(pixels)
static int defaultfontshit[] = {
    8,  5,  11,  10, 6,  13,  12, 7,  16,  16, 10, 19,  24, 14, 29,  36, 22, 44,
    16, 10, 22,  20, 12, 26,  24, 14, 32,  32, 20, 38,  48, 28, 58,  72, 44, 88
}; // normal & zoomed (2x)
#define NDEFAULTFONT (sizeof(defaultfontshit)/sizeof(*defaultfontshit))

static void libpd_defaultfont_init(void)
{
    int i;
    t_atom zz[NDEFAULTFONT+2];
    SETSYMBOL(zz, gensym("."));
    SETFLOAT(zz+1,0);
    for (i = 0; i < (int)NDEFAULTFONT; i++)
    {
        SETFLOAT(zz+i+2, defaultfontshit[i]);
    }
    pd_typedmess(gensym("pd")->s_thing, gensym("init"), 2+NDEFAULTFONT, zz);
}

// cyclone objects functions declaration
void cyclone_setup(void);
void accum_setup(void);
void acos_setup(void);
void acosh_setup(void);
void active_setup(void);
void anal_setup(void);
void append_setup(void);
void asin_setup(void);
void asinh_setup(void);
void atanh_setup(void);
void atodb_setup(void);
void bangbang_setup(void);
void bondo_setup(void);
void borax_setup(void);
void bucket_setup(void);
void buddy_setup(void);
void capture_setup(void);
void cartopol_setup(void);
void clip_setup(void);
void coll_setup(void);
void cosh_setup(void);
void counter_setup(void);
void cycle_setup(void);
void dbtoa_setup(void);
void decide_setup(void);
void decode_setup(void);
void drunk_setup(void);
void flush_setup(void);
void forward_setup(void);
void fromsymbol_setup(void);
void funnel_setup(void);
void gate_setup(void);
void grab_setup(void);
void histo_setup(void);
void iter_setup(void);
void join_setup(void);
void linedrive_setup(void);
void listfunnel_setup(void);
void loadmess_setup(void);
void match_setup(void);
void maximum_setup(void);
void mean_setup(void);
void midiflush_setup(void);
void midiformat_setup(void);
void midiparse_setup(void);
void minimum_setup(void);
void mousefilter_setup(void);
void mousestate_setup(void);
void next_setup(void);
void offer_setup(void);
void onebang_setup(void);
void pak_setup(void);
void past_setup(void);
void peak_setup(void);
void poltocar_setup(void);
void pong_setup(void);
void prepend_setup(void);
void prob_setup(void);
void pv_setup(void);
void rdiv_setup(void);
void rminus_setup(void);
void round_setup(void);
void scale_setup(void);
void seq_setup(void);
void sinh_setup(void);
void speedlim_setup(void);
void spell_setup(void);
void split_setup(void);
void spray_setup(void);
void sprintf_setup(void);
void substitute_setup(void);
void sustain_setup(void);
void switch_setup(void);
void table_setup(void);
void tanh_setup(void);
void thresh_setup(void);
void togedge_setup(void);
void tosymbol_setup(void);
void trough_setup(void);
void universal_setup(void);
void unjoin_setup(void);
void urn_setup(void);
void uzi_setup(void);
void xbendin_setup(void);
void xbendin2_setup(void);
void xbendout_setup(void);
void xbendout2_setup(void);
void xnotein_setup(void);
void xnoteout_setup(void);
void zl_setup(void);
void acos_tilde_setup(void);
void acosh_tilde_setup(void);
void allpass_tilde_setup(void);
void asin_tilde_setup(void);
void asinh_tilde_setup(void);
void atan_tilde_setup(void);
void atan2_tilde_setup(void);
void atanh_tilde_setup(void);
void atodb_tilde_setup(void);
void average_tilde_setup(void);
void avg_tilde_setup(void);
void bitand_tilde_setup(void);
void bitnot_tilde_setup(void);
void bitor_tilde_setup(void);
void bitsafe_tilde_setup(void);
void bitshift_tilde_setup(void);
void bitxor_tilde_setup(void);
void buffir_tilde_setup(void);
void capture_tilde_setup(void);
void cartopol_tilde_setup(void);
void change_tilde_setup(void);
void click_tilde_setup(void);
void clip_tilde_setup(void);
void comb_tilde_setup(void);
void cosh_tilde_setup(void);
void cosx_tilde_setup(void);
void count_tilde_setup(void);
void cross_tilde_setup(void);
void curve_tilde_setup(void);
void cycle_tilde_setup(void);
void dbtoa_tilde_setup(void);
void degrade_tilde_setup(void);
void delay_tilde_setup(void);
void delta_tilde_setup(void);
void deltaclip_tilde_setup(void);
void downsamp_tilde_setup(void);
void edge_tilde_setup(void);
void equals_tilde_setup(void);
void frameaccum_tilde_setup(void);
void framedelta_tilde_setup(void);
void gate_tilde_setup(void);
void greaterthan_tilde_setup(void);
void greaterthaneq_tilde_setup(void);
void index_tilde_setup(void);
void kink_tilde_setup(void);
void lessthan_tilde_setup(void);
void lessthaneq_tilde_setup(void);
void line_tilde_setup(void);
void lookup_tilde_setup(void);
void lores_tilde_setup(void);
void matrix_tilde_setup(void);
void maximum_tilde_setup(void);
void minimum_tilde_setup(void);
void minmax_tilde_setup(void);
void modulo_tilde_setup(void);
void mstosamps_tilde_setup(void);
void notequals_tilde_setup(void);
void onepole_tilde_setup(void);
void overdrive_tilde_setup(void);
void peakamp_tilde_setup(void);
void peek_tilde_setup(void);
void phaseshift_tilde_setup(void);
void phasewrap_tilde_setup(void);
void pink_tilde_setup(void);
void play_tilde_setup(void);
void plusequals_tilde_setup(void);
void poke_tilde_setup(void);
void poltocar_tilde_setup(void);
void pong_tilde_setup(void);
void pow_tilde_setup(void);
void rampsmooth_tilde_setup(void);
void rand_tilde_setup(void);
void rdiv_tilde_setup(void);
void record_tilde_setup(void);
void reson_tilde_setup(void);
void rminus_tilde_setup(void);
void round_tilde_setup(void);
void sah_tilde_setup(void);
void sampstoms_tilde_setup(void);
void scale_tilde_setup(void);
void scope_tilde_setup(void);
void selector_tilde_setup(void);
void sinh_tilde_setup(void);
void sinx_tilde_setup(void);
void slide_tilde_setup(void);
void snapshot_tilde_setup(void);
void spike_tilde_setup(void);
void svf_tilde_setup(void);
void tanh_tilde_setup(void);
void tanx_tilde_setup(void);
void teeth_tilde_setup(void);
void thresh_tilde_setup(void);
void train_tilde_setup(void);
void trapezoid_tilde_setup(void);
void triangle_tilde_setup(void);
void trunc_tilde_setup(void);
void typeroute_tilde_setup(void);
void vectral_tilde_setup(void);
void wave_tilde_setup(void);
void zerox_tilde_setup(void);


// else objects functions declaration
void above_tilde_setup(void);
void add_tilde_setup(void);
void adsr_tilde_setup(void);
void setup_allpass0x2e2nd_tilde(void);
void setup_allpass0x2erev_tilde(void);
void args_setup(void);
void asr_tilde_setup(void);
void autofade_tilde_setup(void);
void autofade2_tilde_setup(void);
void balance_tilde_setup(void);
void bandpass_tilde_setup(void);
void bandstop_tilde_setup(void);
void setup_bend0x2ein(void);
void setup_bend0x2eout(void);
void bicoeff_setup(void);
void biquads_tilde_setup(void);
void blocksize_tilde_setup(void);
void break_setup(void);
void brown_tilde_setup(void);
void buffer_setup(void);
void button_setup(void);
void setup_canvas0x2eactive(void);
void setup_canvas0x2ebounds(void);
void setup_canvas0x2egop(void);
void setup_canvas0x2eedit(void);
void setup_canvas0x2emouse(void);
void setup_canvas0x2ename(void);
void setup_canvas0x2epos(void);
void setup_canvas0x2esetname(void);
void setup_canvas0x2evis(void);
void setup_canvas0x2ewname(void);
void setup_canvas0x2ezoom(void);
void ceil_setup(void);
void ceil_tilde_setup(void);
void cents2ratio_setup(void);
void cents2ratio_tilde_setup(void);
void chance_setup(void);
void chance_tilde_setup(void);
void changed_setup(void);
void changed_tilde_setup(void);
void changed2_tilde_setup(void);
void click_setup(void);
void clipnoise_tilde_setup(void);
void cmul_tilde_setup(void);
void coin_setup(void);
void coin_tilde_setup(void);
void colors_setup(void);
void setup_comb0x2efilt_tilde(void);
void setup_comb0x2erev_tilde(void);
void setup_common0x2ediv(void);
void cosine_tilde_setup(void);
void crackle_tilde_setup(void);
void crossover_tilde_setup(void);
void setup_ctl0x2ein(void);
void setup_ctl0x2eout(void);
void cusp_tilde_setup(void);
void decay_tilde_setup(void);
void decay2_tilde_setup(void);
void del_tilde_setup(void);
void detect_tilde_setup(void);
void dollsym_setup(void);
void downsample_tilde_setup(void);
void drive_tilde_setup(void);
void dust_tilde_setup(void);
void dust2_tilde_setup(void);
void else_setup(void);
void envgen_tilde_setup(void);
void eq_tilde_setup(void);
void f2s_tilde_setup(void);
void factor_setup(void);
void fader_tilde_setup(void);
void fbdelay_tilde_setup(void);
void fbsine_tilde_setup(void);
void fbsine2_tilde_setup(void);
void setup_fdn0x2erev_tilde(void);
void ffdelay_tilde_setup(void);
void float2bits_setup(void);
void float2sig_tilde_setup(void);
void floor_setup(void);
void floor_tilde_setup(void);
void fold_setup(void);
void fold_tilde_setup(void);
void fontsize_setup(void);
void format_setup(void);
void setup_freq0x2eshift_tilde(void);
void function_setup(void);
void function_tilde_setup(void);
void gate2imp_tilde_setup(void);
void gbman_tilde_setup(void);
void gcd_setup(void);
void setup_giga0x2erev_tilde(void);
void glide_tilde_setup(void);
void glide2_tilde_setup(void);
void gray_tilde_setup(void);
void gui_setup(void);
void henon_tilde_setup(void);
void highpass_tilde_setup(void);
void highshelf_tilde_setup(void);
void hot_setup(void);
void hz2rad_setup(void);
void hz2rad_tilde_setup(void);
void ikeda_tilde_setup(void);
void imp_tilde_setup(void);
void imp2_tilde_setup(void);
void impulse_tilde_setup(void);
void impulse2_tilde_setup(void);
void initmess_setup(void);
void int_tilde_setup(void);
void keyboard_setup(void);
void lag_tilde_setup(void);
void lag2_tilde_setup(void);
void lastvalue_tilde_setup(void);
void latoocarfian_tilde_setup(void);
void lb_setup(void);
void lfnoise_tilde_setup(void);
void limit_setup(void);
void lincong_tilde_setup(void);
void loadbanger_setup(void);
void logistic_tilde_setup(void);
void loop_setup(void);
void lorenz_tilde_setup(void);
void lowpass_tilde_setup(void);
void lowshelf_tilde_setup(void);
void match_tilde_setup(void);
void median_tilde_setup(void);
extern void merge_setup(void);
void message_setup(void);
void messbox_setup(void);
void midi_setup(void);
void mouse_setup(void);
void setup_mov0x2eavg_tilde(void);
void setup_mov0x2erms_tilde(void);
void mtx_tilde_setup(void);
void nbang_setup(void);
void note_setup(void);
void setup_note0x2ein(void);
void setup_note0x2eout(void);
void nyquist_tilde_setup(void);
void op_tilde_setup(void);
void openfile_setup(void);
//void oscilloscope_tilde_setup(void);
extern void pack2_setup(void);
void pad_setup(void);
void pan2_tilde_setup(void);
void pan4_tilde_setup(void);
void panic_setup(void);
void parabolic_tilde_setup(void);
void peak_tilde_setup(void);
void setup_pgm0x2ein(void);
void setup_pgm0x2eout(void);
void pimp_tilde_setup(void);
void pinknoise_tilde_setup(void);
void pluck_tilde_setup(void);
void pmosc_tilde_setup(void);
void power_tilde_setup(void);
void properties_setup(void);
void pulse_tilde_setup(void);
void pulsecount_tilde_setup(void);
void pulsediv_tilde_setup(void);
void quad_tilde_setup(void);
void quantizer_setup(void);
void quantizer_tilde_setup(void);
void rad2hz_setup(void);
void rad2hz_tilde_setup(void);
void ramp_tilde_setup(void);
void rampnoise_tilde_setup(void);
void setup_rand0x2ef(void);
void setup_rand0x2ef_tilde(void);
void setup_rand0x2ei(void);
void setup_rand0x2ei_tilde(void);
void setup_rand0x2eseq(void);
void randpulse_tilde_setup(void);
void randpulse2_tilde_setup(void);
void range_tilde_setup(void);
void ratio2cents_setup(void);
void ratio2cents_tilde_setup(void);
void receiver_setup(void);
void rescale_setup(void);
void rescale_tilde_setup(void);
void resonant_tilde_setup(void);
void resonant2_tilde_setup(void);
void retrieve_setup(void);
void rint_tilde_setup(void);
void rint_setup(void);
void rms_tilde_setup(void );
void rotate_tilde_setup(void);
void routeall_setup(void);
void router_setup(void);
void routetype_setup(void);
void s2f_tilde_setup(void);
void saw_tilde_setup(void);
void saw2_tilde_setup(void);
void schmitt_tilde_setup(void);
void selector_setup(void);
void separate_setup(void);
void sequencer_tilde_setup(void);
void sh_tilde_setup(void);
void shaper_tilde_setup(void);
void sig2float_tilde_setup(void);
void sin_tilde_setup(void);
void sine_tilde_setup(void);
void slice_setup(void);
void sort_setup(void);
void spread_setup(void);
void spread_tilde_setup(void);
void square_tilde_setup(void);
void sr_tilde_setup(void);
void standard_tilde_setup(void);
void status_tilde_setup(void);
void stepnoise_tilde_setup(void);
void store_setup(void);
void susloop_tilde_setup(void);
void suspedal_setup(void);
void svfilter_tilde_setup(void);
void symbol2any_setup(void);
void t2_setup(void);
void table_tilde_setup(void);
void tabplayer_tilde_setup(void);
void tabwriter_tilde_setup(void);
void tempo_tilde_setup(void);
void setup_timed0x2egate_tilde(void);
void toggleff_tilde_setup(void);
void setup_touch0x2ein(void);
void setup_touch0x2eout(void);
void tri_tilde_setup(void);
void setup_trig0x2edelay_tilde(void);
void setup_trig0x2edelay2_tilde(void);
void trigger2_setup(void);
void trighold_tilde_setup(void);
void unmerge_setup(void);
void voices_setup(void);
void vsaw_tilde_setup(void);
void vu_tilde_setup(void );
void wavetable_tilde_setup(void);
void wrap2_setup(void);
void wrap2_tilde_setup(void);
void wt_tilde_setup(void);
void xfade_tilde_setup(void);
void xgate_tilde_setup(void);
void xgate2_tilde_setup(void);
void xmod_tilde_setup(void);
void xmod2_tilde_setup(void);
void xselect_tilde_setup(void);
void xselect2_tilde_setup(void);
void zerocross_tilde_setup(void);
// end else objects functions declaration


void libpd_multi_init(void)
{
    static int initialized = 0;
    if(!initialized)
    {
        libpd_set_noteonhook(libpd_multi_noteon);
        libpd_set_controlchangehook(libpd_multi_controlchange);
        libpd_set_programchangehook(libpd_multi_programchange);
        libpd_set_pitchbendhook(libpd_multi_pitchbend);
        libpd_set_aftertouchhook(libpd_multi_aftertouch);
        libpd_set_polyaftertouchhook(libpd_multi_polyaftertouch);
        libpd_set_midibytehook(libpd_multi_midibyte);
        libpd_set_printhook(libpd_multi_print);

        libpd_set_verbose(0);
        libpd_init();
        
        libpd_multi_receiver_setup();
        libpd_multi_midi_setup();
        libpd_multi_print_setup();
        libpd_defaultfont_init();
        libpd_set_verbose(4);

        socket_init();
        
        // cyclone objects
        cyclone_setup();
        accum_setup();
        acos_setup();
        acosh_setup();
        active_setup();
        anal_setup();
        append_setup();
        asin_setup();
        asinh_setup();
        atanh_setup();
        atodb_setup();
        bangbang_setup();
        bondo_setup();
        borax_setup();
        bucket_setup();
        buddy_setup();
        capture_setup();
        cartopol_setup();
        clip_setup();
        coll_setup();
        cosh_setup();
        counter_setup();
        cycle_setup();
        dbtoa_setup();
        decide_setup();
        decode_setup();
        drunk_setup();
        flush_setup();
        forward_setup();
        fromsymbol_setup();
        funnel_setup();
        gate_setup();
        grab_setup();
        histo_setup();
        iter_setup();
        join_setup();
        linedrive_setup();
        listfunnel_setup();
        loadmess_setup();
        match_setup();
        maximum_setup();
        mean_setup();
        midiflush_setup();
        midiformat_setup();
        midiparse_setup();
        minimum_setup();
        mousefilter_setup();
        mousestate_setup();
        next_setup();
        offer_setup();
        onebang_setup();
        pak_setup();
        past_setup();
        peak_setup();
        poltocar_setup();
        pong_setup();
        prepend_setup();
        prob_setup();
        pv_setup();
        rdiv_setup();
        rminus_setup();
        round_setup();
        scale_setup();
        seq_setup();
        sinh_setup();
        speedlim_setup();
        spell_setup();
        split_setup();
        spray_setup();
        sprintf_setup();
        substitute_setup();
        sustain_setup();
        switch_setup();
        table_setup();
        tanh_setup();
        thresh_setup();
        togedge_setup();
        tosymbol_setup();
        trough_setup();
        universal_setup();
        unjoin_setup();
        urn_setup();
        uzi_setup();
        xbendin_setup();
        xbendin2_setup();
        xbendout_setup();
        xbendout2_setup();
        xnotein_setup();
        xnoteout_setup();
        zl_setup();
        
        acos_tilde_setup();
        acosh_tilde_setup();
        allpass_tilde_setup();
        asin_tilde_setup();
        asinh_tilde_setup();
        atan_tilde_setup();
        atan2_tilde_setup();
        atanh_tilde_setup();
        atodb_tilde_setup();
        average_tilde_setup();
        avg_tilde_setup();
        bitand_tilde_setup();
        bitnot_tilde_setup();
        bitor_tilde_setup();
        bitsafe_tilde_setup();
        bitshift_tilde_setup();
        bitxor_tilde_setup();
        buffir_tilde_setup();
        capture_tilde_setup();
        cartopol_tilde_setup();
        change_tilde_setup();
        click_tilde_setup();
        clip_tilde_setup();
        comb_tilde_setup();
        cosh_tilde_setup();
        cosx_tilde_setup();
        count_tilde_setup();
        cross_tilde_setup();
        curve_tilde_setup();
        cycle_tilde_setup();
        dbtoa_tilde_setup();
        degrade_tilde_setup();
        delay_tilde_setup();
        delta_tilde_setup();
        deltaclip_tilde_setup();
        downsamp_tilde_setup();
        edge_tilde_setup();
        equals_tilde_setup();
        frameaccum_tilde_setup();
        framedelta_tilde_setup();
        gate_tilde_setup();
        greaterthan_tilde_setup();
        greaterthaneq_tilde_setup();
        index_tilde_setup();
        kink_tilde_setup();
        lessthan_tilde_setup();
        lessthaneq_tilde_setup();
        line_tilde_setup();
        lookup_tilde_setup();
        lores_tilde_setup();
        matrix_tilde_setup();
        maximum_tilde_setup();
        minimum_tilde_setup();
        minmax_tilde_setup();
        modulo_tilde_setup();
        mstosamps_tilde_setup();
        notequals_tilde_setup();
        onepole_tilde_setup();
        overdrive_tilde_setup();
        peakamp_tilde_setup();
        peek_tilde_setup();
        phaseshift_tilde_setup();
        phasewrap_tilde_setup();
        pink_tilde_setup();
        play_tilde_setup();
        plusequals_tilde_setup();
        poke_tilde_setup();
        poltocar_tilde_setup();
        pong_tilde_setup();
        pow_tilde_setup();
        rampsmooth_tilde_setup();
        rand_tilde_setup();
        rdiv_tilde_setup();
        record_tilde_setup();
        reson_tilde_setup();
        rminus_tilde_setup();
        round_tilde_setup();
        sah_tilde_setup();
        sampstoms_tilde_setup();
        scale_tilde_setup();
        scope_tilde_setup();
        selector_tilde_setup();
        sinh_tilde_setup();
        sinx_tilde_setup();
        slide_tilde_setup();
        snapshot_tilde_setup();
        spike_tilde_setup();
        svf_tilde_setup();
        tanh_tilde_setup();
        tanx_tilde_setup();
        teeth_tilde_setup();
        thresh_tilde_setup();
        train_tilde_setup();
        trapezoid_tilde_setup();
        triangle_tilde_setup();
        trunc_tilde_setup();
        typeroute_tilde_setup();
        vectral_tilde_setup();
        wave_tilde_setup();
        zerox_tilde_setup();

        // else objects initialization
        above_tilde_setup();
        add_tilde_setup();
        adsr_tilde_setup();
        setup_allpass0x2e2nd_tilde();
        setup_allpass0x2erev_tilde();
        args_setup();
        asr_tilde_setup();
        autofade_tilde_setup();
        autofade2_tilde_setup();
        balance_tilde_setup();
        bandpass_tilde_setup();
        bandstop_tilde_setup();
        setup_bend0x2ein();
        setup_bend0x2eout();
        bicoeff_setup();
        biquads_tilde_setup();
        blocksize_tilde_setup();
        break_setup();
        brown_tilde_setup();
        buffer_setup();
        button_setup();
        setup_canvas0x2eactive();
        setup_canvas0x2ebounds();
        setup_canvas0x2egop();
        setup_canvas0x2eedit();
        setup_canvas0x2emouse();
        setup_canvas0x2ename();
        setup_canvas0x2epos();
        setup_canvas0x2esetname();
        setup_canvas0x2evis();
        setup_canvas0x2ewname();
        setup_canvas0x2ezoom();
        ceil_setup();
        ceil_tilde_setup();
        cents2ratio_setup();
        cents2ratio_tilde_setup();
        chance_setup();
        chance_tilde_setup();
        changed_setup();
        changed_tilde_setup();
        changed2_tilde_setup();
        click_setup();
        clipnoise_tilde_setup();
        cmul_tilde_setup();
        coin_setup();
        coin_tilde_setup();
        colors_setup();
        setup_comb0x2efilt_tilde();
        setup_comb0x2erev_tilde();
        setup_common0x2ediv();
        cosine_tilde_setup();
        crackle_tilde_setup();
        crossover_tilde_setup();
        setup_ctl0x2ein();
        setup_ctl0x2eout();
        cusp_tilde_setup();
        decay_tilde_setup();
        decay2_tilde_setup();
        del_tilde_setup();
        detect_tilde_setup();
        dollsym_setup();
        downsample_tilde_setup();
        drive_tilde_setup();
        dust_tilde_setup();
        dust2_tilde_setup();
        else_setup();
        envgen_tilde_setup();
        eq_tilde_setup();
        f2s_tilde_setup();
        factor_setup();
        fader_tilde_setup();
        fbdelay_tilde_setup();
        fbsine_tilde_setup();
        fbsine2_tilde_setup();
        setup_fdn0x2erev_tilde();
        ffdelay_tilde_setup();
        float2bits_setup();
        float2sig_tilde_setup();
        floor_setup();
        floor_tilde_setup();
        fold_setup();
        fold_tilde_setup();
        fontsize_setup();
        format_setup();
        setup_freq0x2eshift_tilde();
        function_setup();
        function_tilde_setup();
        gate2imp_tilde_setup();
        gbman_tilde_setup();
        gcd_setup();
        setup_giga0x2erev_tilde();
        glide_tilde_setup();
        glide2_tilde_setup();
        gray_tilde_setup();
        gui_setup();
        henon_tilde_setup();
        highpass_tilde_setup();
        highshelf_tilde_setup();
        hot_setup();
        hz2rad_setup();
        hz2rad_tilde_setup();
        ikeda_tilde_setup();
        imp_tilde_setup();
        imp2_tilde_setup();
        impulse_tilde_setup();
        impulse2_tilde_setup();
        initmess_setup();
        int_tilde_setup();
        keyboard_setup();
        lag_tilde_setup();
        lag2_tilde_setup();
        lastvalue_tilde_setup();
        latoocarfian_tilde_setup();
        lb_setup();
        lfnoise_tilde_setup();
        limit_setup();
        lincong_tilde_setup();
        loadbanger_setup();
        logistic_tilde_setup();
        loop_setup();
        lorenz_tilde_setup();
        lowpass_tilde_setup();
        lowshelf_tilde_setup();
        match_tilde_setup();
        median_tilde_setup();
        merge_setup();
        message_setup();
        messbox_setup();
        midi_setup();
        mouse_setup();
        setup_mov0x2eavg_tilde();
        setup_mov0x2erms_tilde();
        mtx_tilde_setup();
        nbang_setup();
        note_setup();
        setup_note0x2ein();
        setup_note0x2eout();
        nyquist_tilde_setup();
        op_tilde_setup();
        openfile_setup();
        //oscilloscope_tilde_setup();
        pack2_setup();
        pad_setup();
        pan2_tilde_setup();
        pan4_tilde_setup();
        panic_setup();
        parabolic_tilde_setup();
        peak_tilde_setup();
        setup_pgm0x2ein();
        setup_pgm0x2eout();
        pimp_tilde_setup();
        pinknoise_tilde_setup();
        pluck_tilde_setup();
        pmosc_tilde_setup();
        power_tilde_setup();
        properties_setup();
        pulse_tilde_setup();
        pulsecount_tilde_setup();
        pulsediv_tilde_setup();
        quad_tilde_setup();
        quantizer_setup();
        quantizer_tilde_setup();
        rad2hz_setup();
        rad2hz_tilde_setup();
        ramp_tilde_setup();
        rampnoise_tilde_setup();
        setup_rand0x2ef();
        setup_rand0x2ef_tilde();
        setup_rand0x2ei();
        setup_rand0x2ei_tilde();
        setup_rand0x2eseq();
        randpulse_tilde_setup();
        randpulse2_tilde_setup();
        range_tilde_setup();
        ratio2cents_setup();
        ratio2cents_tilde_setup();
        receiver_setup();
        rescale_setup();
        rescale_tilde_setup();
        resonant_tilde_setup();
        resonant2_tilde_setup();
        retrieve_setup();
        rint_tilde_setup();
        rint_setup();
        rms_tilde_setup();
        rotate_tilde_setup();
        routeall_setup();
        router_setup();
        routetype_setup();
        s2f_tilde_setup();
        saw_tilde_setup();
        saw2_tilde_setup();
        schmitt_tilde_setup();
        selector_setup();
        separate_setup();
        sequencer_tilde_setup();
        sh_tilde_setup();
        shaper_tilde_setup();
        sig2float_tilde_setup();
        sin_tilde_setup();
        sine_tilde_setup();
        slice_setup();
        sort_setup();
        spread_setup();
        spread_tilde_setup();
        square_tilde_setup();
        sr_tilde_setup();
        standard_tilde_setup();
        status_tilde_setup();
        stepnoise_tilde_setup();
        store_setup();
        susloop_tilde_setup();
        suspedal_setup();
        svfilter_tilde_setup();
        symbol2any_setup();
        t2_setup();
        table_tilde_setup();
        tabplayer_tilde_setup();
        tabwriter_tilde_setup();
        tempo_tilde_setup();
        setup_timed0x2egate_tilde();
        toggleff_tilde_setup();
        setup_touch0x2ein();
        setup_touch0x2eout();
        tri_tilde_setup();
        setup_trig0x2edelay_tilde();
        setup_trig0x2edelay2_tilde();
        trigger2_setup();
        trighold_tilde_setup();
        unmerge_setup();
        voices_setup();
        vsaw_tilde_setup();
        vu_tilde_setup();
        wavetable_tilde_setup();
        wrap2_setup();
        wrap2_tilde_setup();
        wt_tilde_setup();
        xfade_tilde_setup();
        xgate_tilde_setup();
        xgate2_tilde_setup();
        xmod_tilde_setup();
        xmod2_tilde_setup();
        xselect_tilde_setup();
        xselect2_tilde_setup();
        zerocross_tilde_setup();
        // end else objects initialization
        

        initialized = 1;
    }
}
