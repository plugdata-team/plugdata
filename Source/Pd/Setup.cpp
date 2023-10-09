/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */



extern "C"
{
#include <m_pd.h>
#include <z_hooks.h>
}

#include <clocale>
#include <string>
#include <cstring>
#include "Setup.h"

static t_class* plugdata_receiver_class;

typedef struct _plugdata_receiver {
    t_object x_obj;
    t_symbol* x_sym;
    void* x_ptr;

    t_plugdata_banghook x_hook_bang;
    t_plugdata_floathook x_hook_float;
    t_plugdata_symbolhook x_hook_symbol;
    t_plugdata_listhook x_hook_list;
    t_plugdata_messagehook x_hook_message;
} t_plugdata_receiver;

static void plugdata_receiver_bang(t_plugdata_receiver* x)
{
    if (x->x_hook_bang)
        x->x_hook_bang(x->x_ptr, x->x_sym->s_name);
}

static void plugdata_receiver_float(t_plugdata_receiver* x, t_float f)
{
    if (x->x_hook_float)
        x->x_hook_float(x->x_ptr, x->x_sym->s_name, f);
}

static void plugdata_receiver_symbol(t_plugdata_receiver* x, t_symbol* s)
{
    if (x->x_hook_symbol)
        x->x_hook_symbol(x->x_ptr, x->x_sym->s_name, s->s_name);
}

static void plugdata_receiver_list(t_plugdata_receiver* x, t_symbol* s, int argc, t_atom* argv)
{
    if (x->x_hook_list)
        x->x_hook_list(x->x_ptr, x->x_sym->s_name, argc, argv);
}

static void plugdata_receiver_anything(t_plugdata_receiver* x, t_symbol* s, int argc, t_atom* argv)
{
    if (x->x_hook_message)
        x->x_hook_message(x->x_ptr, x->x_sym->s_name, s->s_name, argc, argv);
}

static void plugdata_receiver_free(t_plugdata_receiver* x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static t_class* plugdata_midi_class;

typedef struct _plugdata_midi {
    t_object x_obj;
    void* x_ptr;

    t_plugdata_noteonhook x_hook_noteon;
    t_plugdata_controlchangehook x_hook_controlchange;
    t_plugdata_programchangehook x_hook_programchange;
    t_plugdata_pitchbendhook x_hook_pitchbend;
    t_plugdata_aftertouchhook x_hook_aftertouch;
    t_plugdata_polyaftertouchhook x_hook_polyaftertouch;
    t_plugdata_midibytehook x_hook_midibyte;
} t_plugdata_midi;

static void plugdata_noteon(int channel, int pitch, int velocity)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_noteon) {
        x->x_hook_noteon(x->x_ptr, channel, pitch, velocity);
    }
}

static void plugdata_controlchange(int channel, int controller, int value)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_controlchange) {
        x->x_hook_controlchange(x->x_ptr, channel, controller, value);
    }
}

static void plugdata_programchange(int channel, int value)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_programchange) {
        x->x_hook_programchange(x->x_ptr, channel, value);
    }
}

static void plugdata_pitchbend(int channel, int value)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_pitchbend) {
        x->x_hook_pitchbend(x->x_ptr, channel, value);
    }
}

static void plugdata_aftertouch(int channel, int value)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_aftertouch) {
        x->x_hook_aftertouch(x->x_ptr, channel, value);
    }
}

static void plugdata_polyaftertouch(int channel, int pitch, int value)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_polyaftertouch) {
        x->x_hook_polyaftertouch(x->x_ptr, channel, pitch, value);
    }
}

static void plugdata_midibyte(int port, int byte)
{
    t_plugdata_midi* x = (t_plugdata_midi*)gensym("#plugdata_midi")->s_thing;
    if (x && x->x_hook_midibyte) {
        x->x_hook_midibyte(x->x_ptr, port, byte);
    }
}

static void plugdata_midi_free(t_plugdata_midi* x)
{
    pd_unbind(&x->x_obj.ob_pd, gensym("#plugdata_midi"));
}

static t_class* plugdata_print_class;

typedef struct _plugdata_print {
    t_object x_obj;
    void* x_ptr;
    t_plugdata_printhook x_hook;
} t_plugdata_print;

static void plugdata_print(void* object, char const* message)
{
    t_plugdata_print* x = (t_plugdata_print*)gensym("#plugdata_print")->s_thing;
    if (x && x->x_hook) {
        x->x_hook(x->x_ptr, object, message);
    }
}

extern "C"
{



void pd_init();

// pd-extra objects functions declaration
void bob_tilde_setup();
void bonk_tilde_setup();
void choice_setup();
void fiddle_tilde_setup();
void loop_tilde_setup();
void lrshift_tilde_setup();
void pd_tilde_setup();
void pique_setup();
void sigmund_tilde_setup();
void stdout_setup();

// cyclone objects functions declaration
void cyclone_setup();
void accum_setup();
void acos_setup();
void acosh_setup();
void active_setup();
void anal_setup();
void append_setup();
void asin_setup();
void asinh_setup();
void atanh_setup();
void atodb_setup();
void bangbang_setup();
void bondo_setup();
void borax_setup();
void bucket_setup();
void buddy_setup();
void capture_setup();
void cartopol_setup();
void clip_setup();
void coll_setup();
void cosh_setup();
void counter_setup();
void cycle_setup();
void dbtoa_setup();
void decide_setup();
void decode_setup();
void drunk_setup();
void flush_setup();
void forward_setup();
void fromsymbol_setup();
void funnel_setup();
void funbuff_setup();
void gate_setup();
void grab_setup();
void histo_setup();
void iter_setup();
void join_setup();
void linedrive_setup();
void listfunnel_setup();
void loadmess_setup();
void match_setup();
void maximum_setup();
void mean_setup();
void midiflush_setup();
void midiformat_setup();
void midiparse_setup();
void minimum_setup();
void mousefilter_setup();
void mousestate_setup();
void mtr_setup();
void next_setup();
void offer_setup();
void onebang_setup();
void pak_setup();
void past_setup();
void peak_setup();
void poltocar_setup();
void pong_setup();
void prepend_setup();
void prob_setup();
void pv_setup();
void rdiv_setup();
void rminus_setup();
void round_setup();
void scale_setup();
void seq_setup();
void sinh_setup();
void speedlim_setup();
void spell_setup();
void split_setup();
void spray_setup();
void sprintf_setup();
void substitute_setup();
void sustain_setup();
void switch_setup();
void table_setup();
void tanh_setup();
void thresh_setup();
void togedge_setup();
void tosymbol_setup();
void trough_setup();
void universal_setup();
void unjoin_setup();
void urn_setup();
void uzi_setup();
void xbendin_setup();
void xbendin2_setup();
void xbendout_setup();
void xbendout2_setup();
void xnotein_setup();
void xnoteout_setup();
void zl_setup();
void acos_tilde_setup();
void acosh_tilde_setup();
void allpass_tilde_setup();
void asin_tilde_setup();
void asinh_tilde_setup();
void atan_tilde_setup();
void atan2_tilde_setup();
void atanh_tilde_setup();
void atodb_tilde_setup();
void average_tilde_setup();
void avg_tilde_setup();
void bitand_tilde_setup();
void bitnot_tilde_setup();
void bitor_tilde_setup();
void bitsafe_tilde_setup();
void bitshift_tilde_setup();
void bitxor_tilde_setup();
void buffir_tilde_setup();
void capture_tilde_setup();
void cartopol_tilde_setup();
void change_tilde_setup();
void click_tilde_setup();
void clip_tilde_setup();
void comb_tilde_setup();
void comment_setup();
void cosh_tilde_setup();
void cosx_tilde_setup();
void count_tilde_setup();
void cross_tilde_setup();
void curve_tilde_setup();
void cycle_tilde_setup();
void dbtoa_tilde_setup();
void degrade_tilde_setup();
void delay_tilde_setup();
void delta_tilde_setup();
void deltaclip_tilde_setup();
void downsamp_tilde_setup();
void edge_tilde_setup();
void equals_tilde_setup();
void funbuff_setup();
void frameaccum_tilde_setup();
void framedelta_tilde_setup();
void gate_tilde_setup();
void greaterthan_tilde_setup();
void greaterthaneq_tilde_setup();
void index_tilde_setup();
void kink_tilde_setup();
void lessthan_tilde_setup();
void lessthaneq_tilde_setup();
void line_tilde_setup();
void lookup_tilde_setup();
void lores_tilde_setup();
void matrix_tilde_setup();
void maximum_tilde_setup();
void minimum_tilde_setup();
void minmax_tilde_setup();
void modulo_tilde_setup();
void mstosamps_tilde_setup();
void notequals_tilde_setup();
void numbox_tilde_setup();
void onepole_tilde_setup();
void overdrive_tilde_setup();
void peakamp_tilde_setup();
void peek_tilde_setup();
void phaseshift_tilde_setup();
void phasewrap_tilde_setup();
void pink_tilde_setup();
void play_tilde_setup();
void plusequals_tilde_setup();
void poke_tilde_setup();
void poltocar_tilde_setup();
void pong_tilde_setup();
void pow_tilde_setup();
void rampsmooth_tilde_setup();
void rand_tilde_setup();
void rdiv_tilde_setup();
void record_tilde_setup();
void reson_tilde_setup();
void rminus_tilde_setup();
void round_tilde_setup();
void sah_tilde_setup();
void sampstoms_tilde_setup();
void scale_tilde_setup();
void scope_tilde_setup();
void selector_tilde_setup();
void sinh_tilde_setup();
void sinx_tilde_setup();
void slide_tilde_setup();
void snapshot_tilde_setup();
void spike_tilde_setup();
void svf_tilde_setup();
void tanh_tilde_setup();
void tanx_tilde_setup();
void teeth_tilde_setup();
void thresh_tilde_setup();
void train_tilde_setup();
void trapezoid_tilde_setup();
void triangle_tilde_setup();
void trunc_tilde_setup();
void typeroute_tilde_setup();
void vectral_tilde_setup();
void wave_tilde_setup();
void zerox_tilde_setup();

void above_tilde_setup();
void add_tilde_setup();
void adsr_tilde_setup();
void setup_allpass0x2e2nd_tilde();
void setup_allpass0x2erev_tilde();
void args_setup();
void asr_tilde_setup();
void autofade_tilde_setup();
void autofade2_tilde_setup();
void balance_tilde_setup();
void bandpass_tilde_setup();
void bandstop_tilde_setup();
void setup_bend0x2ein();
void setup_bend0x2eout();
void setup_bl0x2esaw_tilde();
void setup_bl0x2esaw2_tilde();
void setup_bl0x2eimp_tilde();
void setup_bl0x2eimp2_tilde();
void setup_bl0x2esquare_tilde();
void setup_bl0x2etri_tilde();
void setup_bl0x2evsaw_tilde();
void setup_osc0x2eformat();
void setup_osc0x2eparse();
void setup_osc0x2eroute();
void beat_tilde_setup();
void bicoeff_setup();
void bicoeff2_setup();
void bitnormal_tilde_setup();
void biquads_tilde_setup();
void blocksize_tilde_setup();
void break_setup();
void brown_tilde_setup();
void buffer_setup();
void button_setup();
void setup_canvas0x2eactive();
void setup_canvas0x2ebounds();
void setup_canvas0x2eedit();
void setup_canvas0x2egop();
void setup_canvas0x2emouse();
void setup_canvas0x2ename();
void setup_canvas0x2epos();
void setup_canvas0x2esetname();
void setup_canvas0x2evis();
void setup_canvas0x2ezoom();
void setup_canvas0x2efile();
void ceil_setup();
void ceil_tilde_setup();
void cents2ratio_setup();
void cents2ratio_tilde_setup();
void chance_setup();
void chance_tilde_setup();
void changed_setup();
void changed_tilde_setup();
void changed2_tilde_setup();
void click_setup();
void cmul_tilde_setup();
void colors_setup();
void setup_comb0x2efilt_tilde();
void setup_comb0x2erev_tilde();
void cosine_tilde_setup();
void crackle_tilde_setup();
void crossover_tilde_setup();
void setup_ctl0x2ein();
void setup_ctl0x2eout();
void cusp_tilde_setup();
void datetime_setup();
void db2lin_tilde_setup();
void decay_tilde_setup();
void decay2_tilde_setup();
void default_setup();
void del_tilde_setup();
void detect_tilde_setup();
void dir_setup();
void dollsym_setup();
void downsample_tilde_setup();
void drive_tilde_setup();
void dust_tilde_setup();
void dust2_tilde_setup();
void else_setup();
void envgen_tilde_setup();
void eq_tilde_setup();
void factor_setup();
void fader_tilde_setup();
void fbdelay_tilde_setup();
void fbsine_tilde_setup();
void fbsine2_tilde_setup();
void setup_fdn0x2erev_tilde();
void ffdelay_tilde_setup();
void float2bits_setup();
void floor_setup();
void floor_tilde_setup();
void fold_setup();
void fold_tilde_setup();
void fontsize_setup();
void format_setup();
void filterdelay_tilde_setup();
void setup_freq0x2eshift_tilde();
void function_setup();
void function_tilde_setup();
void gate2imp_tilde_setup();
void gaussian_tilde_setup();
void gbman_tilde_setup();
void gcd_setup();
void gendyn_tilde_setup();
void setup_giga0x2erev_tilde();
void glide_tilde_setup();
void glide2_tilde_setup();
void gray_tilde_setup();
void henon_tilde_setup();
void highpass_tilde_setup();
void highshelf_tilde_setup();
void hot_setup();
void hz2rad_setup();
void ikeda_tilde_setup();
void imp_tilde_setup();
void imp2_tilde_setup();
void impseq_tilde_setup();
void impulse_tilde_setup();
void impulse2_tilde_setup();
void initmess_setup();
void keyboard_setup();
void keycode_setup();
void lag_tilde_setup();
void lag2_tilde_setup();
void lastvalue_tilde_setup();
void latoocarfian_tilde_setup();
void lb_setup();
void lfnoise_tilde_setup();
void limit_setup();
void lincong_tilde_setup();
void loadbanger_setup();
void logistic_tilde_setup();
void loop_setup();
void lop2_tilde_setup();
void lorenz_tilde_setup();
void lowpass_tilde_setup();
void lowshelf_tilde_setup();
void match_tilde_setup();
void median_tilde_setup();
void merge_setup();
void message_setup();
void messbox_setup();
void metronome_setup();
void midi_setup();
void mouse_setup();
void setup_mov0x2eavg_tilde();
void setup_mov0x2erms_tilde();
void mtx_tilde_setup();
void note_setup();
void setup_note0x2ein();
void setup_note0x2eout();
void noteinfo_setup();
void nyquist_tilde_setup();
void op_tilde_setup();
void openfile_setup();
void oscope_tilde_setup();
void pack2_setup();
void pad_setup();
void pan2_tilde_setup();
void pan4_tilde_setup();
void panic_setup();
void parabolic_tilde_setup();
void peak_tilde_setup();
void setup_pgm0x2ein();
void setup_pgm0x2eout();
void pic_setup();
void pimp_tilde_setup();
void pimpmul_tilde_setup();
void pink_tilde_setup();
void pluck_tilde_setup();
void plaits_tilde_setup();
void pmosc_tilde_setup();
void power_tilde_setup();
void properties_setup();
void pulse_tilde_setup();
void pulsecount_tilde_setup();
void pulsediv_tilde_setup();
void quad_tilde_setup();
void quantizer_setup();
void quantizer_tilde_setup();
void rad2hz_setup();
void ramp_tilde_setup();
void rampnoise_tilde_setup();
void setup_rand0x2ef();
void setup_rand0x2ehist();
void setup_rand0x2ef_tilde();
void setup_rand0x2ei();
void setup_rand0x2ei_tilde();
void setup_rand0x2eu();
#if ENABLE_SFONT
void sfont_tilde_setup();
#endif
void route2_setup();
void randpulse_tilde_setup();
void randpulse2_tilde_setup();
void range_tilde_setup();
void ratio2cents_setup();
void ratio2cents_tilde_setup();
void rec_setup();
void receiver_setup();
void rescale_setup();
void rescale_tilde_setup();
void resonant_tilde_setup();
void resonant2_tilde_setup();
void retrieve_setup();
void rint_setup();
void rint_tilde_setup();
void rms_tilde_setup();
void rotate_tilde_setup();
void routeall_setup();
void router_setup();
void routetype_setup();
void s2f_tilde_setup();
void saw_tilde_setup();
void saw2_tilde_setup();
void schmitt_tilde_setup();
void selector_setup();
void separate_setup();
void sequencer_tilde_setup();
void sh_tilde_setup();
void shaper_tilde_setup();
void sig2float_tilde_setup();
void sin_tilde_setup();
void sine_tilde_setup();
void slew_tilde_setup();
void slew2_tilde_setup();
void slice_setup();
void sort_setup();
void spread_setup();
void spread_tilde_setup();
void square_tilde_setup();
void sr_tilde_setup();
void standard_tilde_setup();
void status_tilde_setup();
void stepnoise_tilde_setup();
void susloop_tilde_setup();
void suspedal_setup();
void svfilter_tilde_setup();
void symbol2any_setup();
//void table_tilde_setup();
void tabplayer_tilde_setup();
void tabreader_setup();
void tabreader_tilde_setup();
void tabwriter_tilde_setup();
void tempo_tilde_setup();
void setup_timed0x2egate_tilde();
void toggleff_tilde_setup();
void setup_touch0x2ein();
void setup_touch0x2eout();
void tri_tilde_setup();
void setup_trig0x2edelay_tilde();
void setup_trig0x2edelay2_tilde();
void trighold_tilde_setup();
void trunc_setup();
void trunc_tilde_setup();
void unmerge_setup();
void voices_setup();
void vsaw_tilde_setup();
void vu_tilde_setup();
void wt_tilde_setup();
void wavetable_tilde_setup();
void wrap2_setup();
void wrap2_tilde_setup();
void white_tilde_setup();
void xfade_tilde_setup();
void xgate_tilde_setup();
void xgate2_tilde_setup();
void xmod_tilde_setup();
void xmod2_tilde_setup();
void xselect_tilde_setup();
void xselect2_tilde_setup();
void zerocross_tilde_setup();

void nchs_tilde_setup();
void get_tilde_setup();
void pick_tilde_setup();
void sigs_tilde_setup();
void select_tilde_setup();
void setup_xselect0x2emc_tilde();
void merge_tilde_setup();
void unmerge_tilde_setup();
void phaseseq_tilde_setup();
void pol2car_tilde_setup();
void car2pol_tilde_setup();
void lin2db_tilde_setup();
void sum_tilde_setup();
void slice_tilde_setup();
void order_setup();
void repeat_tilde_setup();
void setup_xgate0x2emc_tilde();
void setup_xfade0x2emc_tilde();
void sender_setup();
void setup_ptouch0x2ein();
void setup_ptouch0x2eout();
void setup_spread0x2emc_tilde();
void setup_rotate0x2emc_tilde();
void pipe2_setup();
void circuit_tilde_setup();

void op2_tilde_setup();
void op4_tilde_setup();
void op6_tilde_setup();


#if ENABLE_SFIZZ
void sfz_tilde_setup();
#endif
void knob_setup();

void pdlua_setup(const char *datadir, char *vers, int vers_len);
}


namespace pd
{

static int defaultfontshit[] = {
    8, 5, 11, 10, 6, 13, 12, 7, 16, 16, 10, 19, 24, 14, 29, 36, 22, 44,
    16, 10, 22, 20, 12, 26, 24, 14, 32, 32, 20, 38, 48, 28, 58, 72, 44, 88
}; // normal & zoomed (2x)
constexpr int ndefaultfont = int(sizeof(defaultfontshit) / sizeof(*defaultfontshit));

int Setup::initialisePd()
{
    static int initialized = 0;
    if (!initialized) {
        libpd_noteonhook = reinterpret_cast<t_libpd_noteonhook>(plugdata_noteon);
        libpd_controlchangehook = reinterpret_cast<t_libpd_controlchangehook>(plugdata_controlchange);
        libpd_programchangehook = reinterpret_cast<t_libpd_programchangehook>(plugdata_programchange);
        libpd_pitchbendhook = reinterpret_cast<t_libpd_pitchbendhook>(plugdata_pitchbend);
        libpd_aftertouchhook = reinterpret_cast<t_libpd_aftertouchhook>(plugdata_aftertouch);
        libpd_polyaftertouchhook = reinterpret_cast<t_libpd_polyaftertouchhook>(plugdata_polyaftertouch);
        libpd_midibytehook = reinterpret_cast<t_libpd_midibytehook>(plugdata_midibyte);
        sys_printhook = reinterpret_cast<t_printhook>(plugdata_print);
        
        sys_verbose = 0;
        
        // Initialise Pd
        static int s_initialized = 0;
        if (s_initialized) return -1; // only allow init once (for now)
        s_initialized = 1;
        libpd_start_message(32); // allocate array for message assembly
        sys_externalschedlib = 0;
        sys_printtostderr = 0;
        sys_usestdpath = 0; // don't use pd_extrapath, only sys_searchpath
        sys_debuglevel = 0;
        sys_noloadbang = 0;
        sys_hipriority = 0;
        sys_nmidiin = 0;
        sys_nmidiout = 0;
#ifdef HAVE_SCHED_TICK_ARG
        sys_time = 0;
#endif
        pd_init();
        STUFF->st_soundin = NULL;
        STUFF->st_soundout = NULL;
        STUFF->st_schedblocksize = DEFDACBLKSIZE;
        sys_init_fdpoll();
        
        STUFF->st_searchpath = NULL;
        sys_libdir = gensym("");
        post("pd %d.%d.%d%s", PD_MAJOR_VERSION, PD_MINOR_VERSION,
             PD_BUGFIX_VERSION, PD_TEST_VERSION);
        
        bob_tilde_setup();
        bonk_tilde_setup();
        choice_setup();
        fiddle_tilde_setup();
        loop_tilde_setup();
        lrshift_tilde_setup();
        pd_tilde_setup();
        pique_setup();
        sigmund_tilde_setup();
        stdout_setup();
        setlocale(LC_NUMERIC, "C");
        
        sys_lock();
        
        plugdata_receiver_class = class_new(gensym("plugdata_receiver"), (t_newmethod)NULL, (t_method)plugdata_receiver_free,
                                               sizeof(t_plugdata_receiver), CLASS_DEFAULT, A_NULL, 0);
        class_addbang(plugdata_receiver_class, plugdata_receiver_bang);
        class_addfloat(plugdata_receiver_class, plugdata_receiver_float);
        class_addsymbol(plugdata_receiver_class, plugdata_receiver_symbol);
        class_addlist(plugdata_receiver_class, plugdata_receiver_list);
        class_addanything(plugdata_receiver_class, plugdata_receiver_anything);
        
        plugdata_midi_class = class_new(gensym("plugdata_midi"), (t_newmethod)NULL, (t_method)plugdata_midi_free,
                                           sizeof(t_plugdata_midi), CLASS_DEFAULT, A_NULL, 0);
        
        plugdata_print_class = class_new(gensym("plugdata_print"), (t_newmethod)NULL, (t_method)NULL,
                                            sizeof(t_plugdata_print), CLASS_DEFAULT, A_NULL, 0);
        
        int i;
        t_atom zz[ndefaultfont + 2];
        SETSYMBOL(zz, gensym("."));
        SETFLOAT(zz + 1, 0);
        
        for (i = 0; i < ndefaultfont; i++) {
            SETFLOAT(zz + i + 2, defaultfontshit[i]);
        }
        pd_typedmess(gensym("pd")->s_thing, gensym("init"), 2 + ndefaultfont, zz);
        sys_unlock();
        
        sys_defaultfont = 12;
        sys_verbose = 0;
        initialized = 1;
    }

    return 0;
}

void* Setup::createReceiver(void* ptr, char const* s,
    t_plugdata_banghook hook_bang,
    t_plugdata_floathook hook_float,
    t_plugdata_symbolhook hook_symbol,
    t_plugdata_listhook hook_list,
    t_plugdata_messagehook hook_message)
{
    t_plugdata_receiver* x = (t_plugdata_receiver*)pd_new(plugdata_receiver_class);
    if (x) {
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


void* Setup::createPrintHook(void* ptr, t_plugdata_printhook hook_print)
{
    t_plugdata_print* x = (t_plugdata_print*)pd_new(plugdata_print_class);
    if (x) {
        sys_lock();
        t_symbol* s = gensym("#plugdata_print");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook = hook_print;
    }
    return x;
}


void* Setup::createMIDIHook(void* ptr,
    t_plugdata_noteonhook hook_noteon,
    t_plugdata_controlchangehook hook_controlchange,
    t_plugdata_programchangehook hook_programchange,
    t_plugdata_pitchbendhook hook_pitchbend,
    t_plugdata_aftertouchhook hook_aftertouch,
    t_plugdata_polyaftertouchhook hook_polyaftertouch,
    t_plugdata_midibytehook hook_midibyte)
{

    t_plugdata_midi* x = (t_plugdata_midi*)pd_new(plugdata_midi_class);
    if (x) {
        sys_lock();
        t_symbol* s = gensym("#plugdata_midi");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook_noteon = hook_noteon;
        x->x_hook_controlchange = hook_controlchange;
        x->x_hook_programchange = hook_programchange;
        x->x_hook_pitchbend = hook_pitchbend;
        x->x_hook_aftertouch = hook_aftertouch;
        x->x_hook_polyaftertouch = hook_polyaftertouch;
        x->x_hook_midibyte = hook_midibyte;
    }
    return x;
}

void Setup::parseArguments(char const** argv, size_t argc, t_namelist** sys_openlist, t_namelist** sys_messagelist)
{
    sys_lock();
    t_audiosettings as;
    /* get the current audio parameters.  These are set
    by the preferences mechanism (sys_loadpreferences()) or
    else are the default.  Overwrite them with any results
    of argument parsing, and store them again. */
    sys_get_audio_settings(&as);
    while ((argc > 0) && **argv == '-') {
        /* audio flags */
        if (!strcmp(*argv, "-r") && argc > 1 && sscanf(argv[1], "%d", &as.a_srate) >= 1) {
            argc -= 2;
            argv += 2;
        }
        /*
        else if (!strcmp(*argv, "-inchannels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchindev,
                    as.a_chindevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-outchannels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchoutdev,
                    as.a_choutdevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-channels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchindev,
                    as.a_chindevvec, MAXAUDIOINDEV, argv[1]) ||
                !sys_parsedevlist(&as.a_nchoutdev,
                    as.a_choutdevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-soundbuf") || (!strcmp(*argv, "-audiobuf")))
        {
            if (argc < 2)
                goto usage;

            as.a_advance = atoi(argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-callback"))
        {
            as.a_callback = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nocallback"))
        {
            as.a_callback = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-blocksize"))
        {
            as.a_blocksize = atoi(argv[1]);
            argc -= 2; argv += 2;
        }*/
        else if (!strcmp(*argv, "-sleepgrain")) {
            if (argc < 2)
                goto usage;
            sys_sleepgrain = 1000 * atof(argv[1]);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-nodac")) {
            as.a_noutdev = as.a_nchoutdev = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-noadc")) {
            as.a_nindev = as.a_nchindev = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nosound") || !strcmp(*argv, "-noaudio")) {
            as.a_noutdev = as.a_nchoutdev = as.a_nindev = as.a_nchindev = 0;
            argc--;
            argv++;
        }
        /* MIDI flags */
        else if (!strcmp(*argv, "-nomidiin")) {
            sys_nmidiin = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nomidiout")) {
            sys_nmidiout = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nomidi")) {
            sys_nmidiin = sys_nmidiout = 0;
            argc--;
            argv++;
        }
        /* other flags */
        else if (!strcmp(*argv, "-path")) {
            if (argc < 2)
                goto usage;
            STUFF->st_temppath = namelist_append_files(STUFF->st_temppath, argv[1]);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-nostdpath")) {
            sys_usestdpath = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-stdpath")) {
            sys_usestdpath = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-helppath")) {
            if (argc < 2)
                goto usage;
            STUFF->st_helppath = namelist_append_files(STUFF->st_helppath, argv[1]);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-open")) {
            if (argc < 2)
                goto usage;

            *sys_openlist = namelist_append_files(*sys_openlist, argv[1]);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-lib")) {
            if (argc < 2)
                goto usage;
            
            // TODO: load libraries that we added manually (??)

            STUFF->st_externlist = namelist_append_files(STUFF->st_externlist, argv[1]);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-verbose")) {
            sys_verbose++;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-noverbose")) {
            sys_verbose = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-version")) {
            // sys_version = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-d") && argc > 1 && sscanf(argv[1], "%d", &sys_debuglevel) >= 1) {
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-loadbang")) {
            sys_noloadbang = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-noloadbang")) {
            sys_noloadbang = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nostderr")) {
            sys_printtostderr = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-stderr")) {
            sys_printtostderr = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-send")) {
            if (argc < 2)
                goto usage;

            *sys_messagelist = namelist_append(*sys_messagelist, argv[1], 1);
            argc -= 2;
            argv += 2;
        } else if (!strcmp(*argv, "-batch")) {
            // sys_batch = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nobatch")) {
            // sys_batch = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-autopatch")) {
            // sys_noautopatch = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-noautopatch")) {
            // sys_noautopatch = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-compatibility")) {
            float f;
            if (argc < 2)
                goto usage;

            if (sscanf(argv[1], "%f", &f) < 1)
                goto usage;
            pd_compatibilitylevel = 0.5 + 100. * f; /* e.g., 2.44 --> 244 */
            argv += 2;
            argc -= 2;
        } else if (!strcmp(*argv, "-sleep")) {
            // sys_nosleep = 0;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-nosleep")) {
            // sys_nosleep = 1;
            argc--;
            argv++;
        } else if (!strcmp(*argv, "-noprefs")) /* did this earlier */
            argc--, argv++;
        else if (!strcmp(*argv, "-prefsfile") && argc > 1) /* this too */
            argc -= 2, argv += 2;
        else {
        usage:
            // sys_printusage();
            sys_unlock();
            return;
        }
    }

    sys_set_audio_settings(&as);
    sys_unlock();

    /* load dynamic libraries specified with "-lib" args */
    t_namelist* nl;
    for (nl = STUFF->st_externlist; nl; nl = nl->nl_next)
        if (!sys_load_lib(NULL, nl->nl_string))
            post("%s: can't load library", nl->nl_string);

    return;
}

void Setup::initialisePdLua(const char *datadir, char *vers, int vers_len)
{
    pdlua_setup(datadir, vers, vers_len);
}

void Setup::initialiseELSE()
{
    knob_setup();
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
    setup_bl0x2esaw_tilde();
    setup_bl0x2esaw2_tilde();
    setup_bl0x2eimp_tilde();
    setup_bl0x2eimp2_tilde();
    setup_bl0x2esquare_tilde();
    setup_bl0x2etri_tilde();
    setup_bl0x2evsaw_tilde();
    setup_osc0x2eformat();
    setup_osc0x2eparse();
    setup_osc0x2eroute();
    beat_tilde_setup();
    bicoeff_setup();
    bicoeff2_setup();
    bitnormal_tilde_setup();
    biquads_tilde_setup();
    blocksize_tilde_setup();
    break_setup();
    brown_tilde_setup();
    buffer_setup();
    button_setup();
    setup_canvas0x2eactive();
    setup_canvas0x2ebounds();
    setup_canvas0x2eedit();
    setup_canvas0x2egop();
    setup_canvas0x2emouse();
    setup_canvas0x2ename();
    setup_canvas0x2epos();
    setup_canvas0x2esetname();
    setup_canvas0x2evis();
    setup_canvas0x2ezoom();
    setup_canvas0x2efile();
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
    white_tilde_setup();
    cmul_tilde_setup();
    colors_setup();
    setup_comb0x2efilt_tilde();
    setup_comb0x2erev_tilde();
    cosine_tilde_setup();
    crackle_tilde_setup();
    crossover_tilde_setup();
    setup_ctl0x2ein();
    setup_ctl0x2eout();
    cusp_tilde_setup();
    datetime_setup();
    db2lin_tilde_setup();
    decay_tilde_setup();
    decay2_tilde_setup();
    default_setup();
    del_tilde_setup();
    detect_tilde_setup();
    dir_setup();
    dollsym_setup();
    downsample_tilde_setup();
    drive_tilde_setup();
    dust_tilde_setup();
    dust2_tilde_setup();
    else_setup();
    envgen_tilde_setup();
    eq_tilde_setup();
    factor_setup();
    fader_tilde_setup();
    fbdelay_tilde_setup();
    fbsine_tilde_setup();
    fbsine2_tilde_setup();
    setup_fdn0x2erev_tilde();
    ffdelay_tilde_setup();
    float2bits_setup();
    floor_setup();
    floor_tilde_setup();
    fold_setup();
    fold_tilde_setup();
    fontsize_setup();
    format_setup();
    filterdelay_tilde_setup();
    setup_freq0x2eshift_tilde();
    function_setup();
    function_tilde_setup();
    gate2imp_tilde_setup();
    gaussian_tilde_setup();
    gbman_tilde_setup();
    gcd_setup();
    gendyn_tilde_setup();
    setup_giga0x2erev_tilde();
    glide_tilde_setup();
    glide2_tilde_setup();
    gray_tilde_setup();
    henon_tilde_setup();
    highpass_tilde_setup();
    highshelf_tilde_setup();
    hot_setup();
    hz2rad_setup();
    ikeda_tilde_setup();
    imp_tilde_setup();
    imp2_tilde_setup();
    impseq_tilde_setup();
    impulse_tilde_setup();
    impulse2_tilde_setup();
    initmess_setup();
    keyboard_setup();
    keycode_setup();
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
    lop2_tilde_setup();
    lorenz_tilde_setup();
    lowpass_tilde_setup();
    lowshelf_tilde_setup();
    match_tilde_setup();
    median_tilde_setup();
    merge_setup();
    message_setup();
    messbox_setup();
    metronome_setup();
    midi_setup();
    mouse_setup();
    setup_mov0x2eavg_tilde();
    setup_mov0x2erms_tilde();
    mtx_tilde_setup();
    note_setup();
    setup_note0x2ein();
    setup_note0x2eout();
    noteinfo_setup();
    nyquist_tilde_setup();
    op_tilde_setup();
    openfile_setup();
    oscope_tilde_setup();
    pack2_setup();
    pad_setup();
    pan2_tilde_setup();
    pan4_tilde_setup();
    panic_setup();
    parabolic_tilde_setup();
    peak_tilde_setup();
    setup_pgm0x2ein();
    setup_pgm0x2eout();
    pic_setup();
    pimp_tilde_setup();
    pimpmul_tilde_setup();
    pink_tilde_setup();
    plaits_tilde_setup();
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
    ramp_tilde_setup();
    rampnoise_tilde_setup();
    setup_rand0x2ef();
    setup_rand0x2eu();
    setup_rand0x2ef_tilde();
    setup_rand0x2ehist();
    s2f_tilde_setup();
#if ENABLE_SFONT
    sfont_tilde_setup();
#endif
    setup_rand0x2ei();
    setup_rand0x2ei_tilde();
    numbox_tilde_setup();
    route2_setup();
    randpulse_tilde_setup();
    randpulse2_tilde_setup();
    range_tilde_setup();
    ratio2cents_setup();
    ratio2cents_tilde_setup();
    rec_setup();
    receiver_setup();
    rescale_setup();
    rescale_tilde_setup();
    resonant_tilde_setup();
    resonant2_tilde_setup();
    retrieve_setup();
    rint_setup();
    rint_tilde_setup();
    rms_tilde_setup();
    rotate_tilde_setup();
    routeall_setup();
    router_setup();
    routetype_setup();
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
    slew_tilde_setup();
    slew2_tilde_setup();
    slice_setup();
    sort_setup();
    spread_setup();
    spread_tilde_setup();
    square_tilde_setup();
    sr_tilde_setup();
    standard_tilde_setup();
    status_tilde_setup();
    stepnoise_tilde_setup();
    susloop_tilde_setup();
    suspedal_setup();
    svfilter_tilde_setup();
    symbol2any_setup();
    //table_tilde_setup();
    tabplayer_tilde_setup();
    tabreader_setup();
    tabreader_tilde_setup();
    tabwriter_tilde_setup();
    tempo_tilde_setup();
    setup_timed0x2egate_tilde();
    toggleff_tilde_setup();
    setup_touch0x2ein();
    setup_touch0x2eout();
    tri_tilde_setup();
    setup_trig0x2edelay_tilde();
    setup_trig0x2edelay2_tilde();
    trighold_tilde_setup();
    trunc_setup();
    trunc_tilde_setup();
    unmerge_setup();
    voices_setup();
    vsaw_tilde_setup();
    vu_tilde_setup();
    wt_tilde_setup();
    wavetable_tilde_setup();
    wrap2_setup();
    wrap2_tilde_setup();
    xfade_tilde_setup();
    xgate_tilde_setup();
    xgate2_tilde_setup();
    xmod_tilde_setup();
    xmod2_tilde_setup();
    xselect_tilde_setup();
    xselect2_tilde_setup();
    zerocross_tilde_setup();
    nchs_tilde_setup();
    get_tilde_setup();
    pick_tilde_setup();
    sigs_tilde_setup();
    select_tilde_setup();
    setup_xselect0x2emc_tilde();
    merge_tilde_setup();
    unmerge_tilde_setup();
    phaseseq_tilde_setup();
    pol2car_tilde_setup();
    car2pol_tilde_setup();
    lin2db_tilde_setup();
    sum_tilde_setup();
    slice_tilde_setup();
    order_setup();
    repeat_tilde_setup();
    setup_xgate0x2emc_tilde();
    setup_xfade0x2emc_tilde();
#if ENABLE_SFIZZ
    sfz_tilde_setup();
#endif
    sender_setup();
    setup_ptouch0x2ein();
    setup_ptouch0x2eout();
    setup_spread0x2emc_tilde();
    setup_rotate0x2emc_tilde();
    pipe2_setup();
    circuit_tilde_setup();
    
    /* Not yet!
    op2_tilde_setup();
    op4_tilde_setup();
    op6_tilde_setup(); */
}

void Setup::initialiseCyclone()
{
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
    funbuff_setup();
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
    mtr_setup();
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
    comment_setup();
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
}

}
