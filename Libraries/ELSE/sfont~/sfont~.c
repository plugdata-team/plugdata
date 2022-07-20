
// Soundfont player based on FluidSynth (https://www.fluidsynth.org/)
// Copyright by Porres, see https://github.com/porres/pd-sfont

/*
LICENSE:
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See thever
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "m_pd.h"
#include "../shared/elsefile.h"

#include <stdlib.h>
#include <fluidsynth.h>
#include <string.h>


#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define MAXSYSEXSIZE 1024 // Size of sysex data list (excluding the F0 [240] and F7 [247] bytes)

static t_class *sfont_class;
 
static int printed;

typedef struct _sfont{
    t_object            x_obj;
    fluid_synth_t      *x_synth;
    fluid_settings_t   *x_settings;
    fluid_sfont_t      *x_sfont;
    fluid_preset_t     *x_preset;
    t_elsefile         *x_elsefilehandle;
    t_outlet           *x_out_left;
    t_outlet           *x_out_right;
    t_canvas           *x_canvas;
    t_symbol           *x_sfname;
    t_symbol           *x_tune_name;
    t_outlet           *x_info_out;
    float               x_base;
    int                 x_sysex;
    int                 x_tune_ch;
    int                 x_tune_bank;
    int                 x_tune_prog;
    int                 x_ch;
    int                 x_verbosity;
    int                 x_count;
    int                 x_ready;
    int                 x_id;
    int                 x_bank;
    int                 x_pgm;
    t_atom              x_at[MAXSYSEXSIZE];
    unsigned char       x_type;
    unsigned char       x_data;
    unsigned char       x_channel;
}t_sfont;

static void sfont_getversion(void){
    post("[sfont~] version 1.0-rc2 (using fluidsynth %s)", FLUIDSYNTH_VERSION);
}

static void sfont_verbose(t_sfont *x, t_floatarg f){
    x->x_verbosity = f != 0;
}

static void sfont_panic(t_sfont *x){
    if(x->x_synth)
        fluid_synth_system_reset(x->x_synth);
}

static void sfont_transp(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac < 1 || ac > 2)
        return;
    float cents = atom_getfloatarg(0, ac, av);
    int ch = ac == 2 ? atom_getintarg(1, ac, av) : 0;
    fluid_synth_set_gen(x->x_synth, ch, GEN_FINETUNE, cents);
}

static void sfont_pan(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    float f1 = atom_getfloatarg(0, ac, av);
    int ch = atom_getintarg(1, ac, av);
    float pan = f1 < -1 ? -1 : f1 > 1 ? 1 : f1;
    fluid_synth_set_gen(x->x_synth, ch, GEN_PAN, pan*500);
}

static void sfont_note(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int key = atom_getintarg(0, ac, av);
        int vel = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1; // channel starts at zero???
        if(chan > x->x_ch){
            post("[sfont~]: note channel (%d) out of range (max is: %d)", chan, x->x_ch);
            return;
        }
        fluid_synth_noteon(x->x_synth, chan-1, key, vel);
//        if(flag == FLUID_FAILED)
//            post("[sfont~]: note message error (pitch (%d) velocity (%d)");
    }
}

static void sfont_program_change(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int pgm = atom_getintarg(0, ac, av);
        x->x_pgm = pgm < 0 ? 0 : pgm > 127 ? 127 : pgm;
        int ch = ac > 1 ? atom_getintarg(1, ac, av) - 1: 0;
        if(ch > x->x_ch){
            post("[sfont~]: program channel (%d) out of range (max is: %d)", ch, x->x_ch);
            return;
        }
        int fail = fluid_synth_program_change(x->x_synth, ch, x->x_pgm);
        if(!fail){
            fluid_preset_t* preset = fluid_synth_get_channel_preset(x->x_synth, ch);
            int bank = fluid_preset_get_banknum(preset);
            if(preset == NULL){
                if(x->x_verbosity)
                    post("[sfont~]: couldn't load progam %d from bank %d", x->x_pgm, bank);
            }
            else{
                x->x_bank = bank;
                const char* pname = fluid_preset_get_name(preset);
                if(x->x_verbosity)
                    post("[sfont~]: loaded \"%s\" (bank %d, pgm %d) in channel %d",
                        pname, x->x_bank, x->x_pgm, ch + 1);
                t_atom at[1];
                SETSYMBOL(&at[0], gensym(pname));
                outlet_anything(x->x_info_out, gensym("preset"), 1, at);
            }
        }
        else 
            post("[sfont~]: couldn't load progam %d from bank %d into channel %d",
                 x->x_pgm, x->x_bank, ch+1);
    }
}

static void sfont_bank(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int bank = atom_getintarg(0, ac, av);
        if(bank < 0)
            bank = 0;
        int ch = ac > 1 ? atom_getintarg(1, ac, av) - 1 : 0;
        if(ch > x->x_ch){
            post("[sfont~]: bank channel (%d) out of range (max is: %d)", ch, x->x_ch);
            return;
        }
        int fail = fluid_synth_bank_select(x->x_synth, ch, bank);
        if(!fail){
            x->x_bank = bank;
            fluid_preset_t* preset = fluid_sfont_get_preset(x->x_sfont, x->x_bank, x->x_pgm);
            if(preset == NULL){
                if(x->x_verbosity)
                    post("[sfont~]: couldn't load progam %d from bank %d", x->x_pgm, x->x_bank);
            }
            else{
                fluid_synth_program_reset(x->x_synth);
                const char* pname = fluid_preset_get_name(preset);
                if(x->x_verbosity)
                    post("[sfont~]: loaded \"%s\" (bank %d, pgm %d) in channel %d",
                         pname, x->x_bank, x->x_pgm, ch + 1);
                t_atom at[1];
                SETSYMBOL(&at[0], gensym(pname));
                outlet_anything(x->x_info_out, gensym("pname"), 1, at);
            }
        }
        else
            post("[sfont~]: couldn't load bank %d", bank);
    }
}

static void sfont_control_change(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int val = atom_getintarg(0, ac, av);
        int ctrl = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1;
        fluid_synth_cc(x->x_synth, chan-1, ctrl, val);
    }
}

static void sfont_pitch_bend(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int val = atom_getintarg(0, ac, av);
        int chan = ac > 1 ? atom_getintarg(1, ac, av) : 1;
        fluid_synth_pitch_bend(x->x_synth, chan-1, val);
    }
}

/*static void fluid_gen(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2){
        int param = atom_getintarg(1, ac, av);
        float value = atom_getfloatarg(2, ac, av);
        fluid_synth_set_gen(x->x_synth, 0, param, value);
    }
    else if(ac == 3){
        int chan = atom_getintarg(0, ac, av);
        float param = atom_getintarg(1, ac, av);
        float value = atom_getfloatarg(2, ac, av);
        fluid_synth_set_gen(x->x_synth, chan-1, param, value);
    }
}*/

static void sfont_unsel_tuning(t_sfont *x,  t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){
        int ch = atom_getfloatarg(0, ac, av);
        fluid_synth_deactivate_tuning(x->x_synth, ch-1, 1);
    }
    else for(int i = 0; i < x->x_ch; i++)
        fluid_synth_deactivate_tuning(x->x_synth, i, 1);
}

static void sfont_sel_tuning(t_sfont *x, t_float bank, t_float pgm, t_float ch){
    fluid_synth_activate_tuning(x->x_synth, ch-1, bank, pgm, 1);
    char scale_name[256];
    fluid_synth_tuning_dump(x->x_synth, bank, pgm, scale_name, 256, NULL);
    t_atom at[1];
    SETSYMBOL(&at[0], gensym(scale_name));
    outlet_anything(x->x_info_out, gensym("scale"), 1, at);
}

static void set_key_tuning(t_sfont *x, double *pitches){
    int ch = x->x_tune_ch, bank = x->x_tune_bank, pgm = x->x_tune_prog;
    const char* name = x->x_tune_name->s_name;
    fluid_synth_activate_key_tuning(x->x_synth, bank, pgm, name, pitches, 1);
    if(ch > 0)
        fluid_synth_activate_tuning(x->x_synth, ch-1, bank, pgm, 1);
    else if(!ch) for(int i = 0; i < x->x_ch; i++)
        fluid_synth_activate_tuning(x->x_synth, i, bank, pgm, 1);
}

static void sfont_set_tuning(t_sfont *x,  t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac < 3)
        return;
    x->x_tune_bank = atom_getintarg(0, ac, av);
    x->x_tune_prog = atom_getintarg(1, ac, av);
    x->x_tune_ch = atom_getintarg(2, ac, av);
    if(ac == 4)
        x->x_tune_name = atom_getsymbolarg(3, ac, av);
    else
        x->x_tune_name = gensym("Custom-tuning");
        
}

static void sfont_remap(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 128){
        double pitches[128];
        for(int i = 0; i < 128; i++)
            pitches[i] = atom_getfloatarg(i, ac, av) * 100;
        set_key_tuning(x, pitches);
    }
    else
        post("[sinfo~]: remap needs 128 key values");
}

static void sfont_scale(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(!ac){
        set_key_tuning(x, NULL);
        return;
    }
    double* scale = calloc(sizeof(double), ac);
    int n_m1 = ac-1;
    int i;
    for(i = 0; i < ac; i++) // set array{
        scale[i] = atom_getfloatarg(i, ac, av);
    int n1 = -(int)x->x_base, div;
    n1 -= (n_m1-1);
    div = n1 / n_m1;
    float shift_down = div * scale[n_m1] + x->x_base * 100.;
    int count = (int)(-x->x_base) % n_m1;
    if(count < 0)
        count += n_m1;
    float sum = scale[count] + shift_down;
    double pitches[128];
    pitches[0] = sum;
    for(i = 1; i < 128; i++){
        count = (count % n_m1) + 1;
        pitches[i] = (sum += (scale[count] - scale[count-1]));
    }
    set_key_tuning(x, pitches);
    free(scale);
}

static void sfont_base(t_sfont *x, t_float f){
    x->x_base = f < 0 ? 0 : f > 127 ? 127 : f;
}

/*static void fluid_octave_tuning(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(ac == 12){
        double pitches[12];
        for(int i = 0; i < 12; i++)
            pitches[i] = atom_getfloatarg(i, ac, av);
        int tune_bank = 0;
        int tune_prog = 0;
        int ch = 0;
        fluid_synth_activate_octave_tuning(x->x_synth, tune_bank, tune_prog, "oct-tuning", pitches, 1);
        fluid_synth_activate_tuning(x->x_synth, ch, tune_bank, tune_prog, 1);
    }
    else
        post("wrong size");
}*/

static void sfont_aftertouch(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int val = atom_getintarg(0, ac, av);
        int chan = ac > 1 ? atom_getintarg(1, ac, av) : 1;
        fluid_synth_channel_pressure(x->x_synth, chan-1, val);
    }
}

static void sfont_polytouch(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int val = atom_getintarg(0, ac, av);
        int key = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1;
        fluid_synth_key_pressure(x->x_synth, chan-1, key, val);
    }
}

static void sfont_sysex(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac > 0){
        char buf[MAXSYSEXSIZE];
        int len = 0;
        while(len < MAXSYSEXSIZE && len < ac){
            buf[len] = atom_getintarg(len, ac, av);
            len++;
        }
        // TODO: output fluidsynth's response to info outlet
        // in order to handle bulk dump requests
        fluid_synth_sysex(x->x_synth, buf, len, NULL, NULL, NULL, 0);
    }
}

static void sfont_float(t_sfont *x, t_float f){ // raw midi input
    if(f >= 0 && f <= 255){
        unsigned char val = (unsigned char)f;
        if(val > 127){ // not a data type
            if(val == 0xF0){ // start of sysex
                x->x_sysex = 1;
                x->x_count = 0;
            }
            else if(val == 0xF7){ // end of sysex
                sfont_sysex(x, &s_list, x->x_count, x->x_at);
                x->x_sysex = x->x_count = 0;
            }
            else{
                x->x_type = val & 0xF0; // get type
                x->x_channel = (val & 0x0F) + 1; // get channel
                x->x_ready = (x->x_type == 0xC0 || x->x_type == 0xD0); // ready if program or touch
            }
        }
        else if(x->x_sysex){
            SETFLOAT(&x->x_at[x->x_count], (t_float)val);
            x->x_count++;
        }
        else{
            if(x->x_ready){
                switch(x->x_type){
                    case 0x80: // group 128-143 (NOTE OFF)
                    SETFLOAT(&x->x_at[0], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[1], 0); // make it note on with velocity 0
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    sfont_note(x, &s_list, 3, x->x_at);
                    break;
                case 0x90: // group 144-159 (NOTE ON)
                    SETFLOAT(&x->x_at[0], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[1], val); // velocity
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    sfont_note(x, &s_list, 3, x->x_at);
                    break;
                case 0xA0: // group 160-175 (POLYPHONIC AFTERTOUCH)
                    SETFLOAT(&x->x_at[0], val); // aftertouch pressure value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    sfont_polytouch(x, &s_list, 3, x->x_at);
                    break;
                case 0xB0: // group 176-191 (CONTROL CHANGE)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_data); // control number
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    sfont_control_change(x, &s_list, 3, x->x_at);
                    break;
                case 0xC0: // group 192-207 (PROGRAM CHANGE)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
                    sfont_program_change(x, &s_list, 2, x->x_at);
                    break;
                case 0xD0: // group 208-223 (AFTERTOUCH)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
                    sfont_aftertouch(x, &s_list, 2, x->x_at);
                    break;
                case 0xE0: // group 224-239 (PITCH BEND)
                    SETFLOAT(&x->x_at[0], (val << 7) + x->x_data);
                    SETFLOAT(&x->x_at[1], x->x_channel);
                    sfont_pitch_bend(x, &s_list, 2, x->x_at);
                    break;
                default:
                    break;
                }
                x->x_type = x->x_ready = 0; // clear
            }
            else{ // not ready, get data and make it ready
                x->x_data = val;
                x->x_ready = 1;
            }
        }
    }
    else
        x->x_type = x->x_ready = 0; // clear
}

/*static void tuning_dump(t_sfont *x){
    char scale_name[256];
    fluid_synth_tuning_dump(x->x_synth, bank, pgm, scale_name, 256, NULL);
    t_atom at[1];
    SETSYMBOL(&at[0], gensym(scale_name));
}

static void sfont_ch_info(t_sfont *x){
    for(int i = 0; i < x->x_ch; i++){ // get bank and program loaded in channels
        if((preset = fluid_synth_get_channel_preset(x->x_synth, i)) != NULL){
            int bank = fluid_preset_get_banknum(preset), pgm = fluid_preset_get_num(preset);
            const char* name = fluid_preset_get_name(preset);
            post("channel %d: bank (%d) pgm (%d) name (%s)", i, bank, pgm, name);
        }
    }
}*/

static void sfont_info(t_sfont *x){
    if(x->x_sfname == NULL){
        post("[sfont~]: no soundfont loaded, nothing to print");
        return;
    }
    post("Loaded soundfont: %s", fluid_sfont_get_name(x->x_sfont));
    post("------------------- presets -------------------");
    int i = 1;
    fluid_preset_t* preset;
    fluid_sfont_iteration_start(x->x_sfont);
    while((preset = fluid_sfont_iteration_next(x->x_sfont))){
        int bank = fluid_preset_get_banknum(preset), pgm = fluid_preset_get_num(preset);
        const char* name = fluid_preset_get_name(preset);
        post("%03d - bank (%d) pgm (%d) name (%s)", i++, bank, pgm, name);
    }
    post("\n");
}

static void fluid_do_load(t_sfont *x, t_symbol *name){
    const char* filename = name->s_name;
    const char* ext = strrchr(filename, '.');
    char realdir[MAXPDSTRING], *realname = NULL;
    int fd;
    if(ext && !strchr(ext, '/')){ // extension already supplied, no default extension
        ext = "";
        fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
        if(fd < 0){
            pd_error(x, "[sfont~]: can't find soundfont %s", filename);
            return;
        }
    }
    else{
        ext = ".sf2"; // let's try sf2
        fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
        if(fd < 0){ // failed
            ext = ".sf3"; // let's try sf3 then
            fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
            if(fd < 0){ // also failed
                pd_error(x, "[sfont~]: can't find soundfont %s", filename);
                return;
            }
        }
    }
    sys_close(fd);
    chdir(realdir);
    int id = fluid_synth_sfload(x->x_synth, realname, 0);
    if(id >= 0){
        fluid_synth_program_reset(x->x_synth);
        x->x_sfont = fluid_synth_get_sfont_by_id(x->x_synth, id);
        x->x_sfname = name;
        if(x->x_verbosity)
            sfont_info(x);
        fluid_preset_t* preset = fluid_sfont_get_preset(x->x_sfont, x->x_bank = 0, x->x_pgm = 0);
        if(preset){
            const char* pname = fluid_preset_get_name(preset);
            t_atom at[1];
            SETSYMBOL(&at[0], gensym(pname));
            outlet_anything(x->x_info_out, gensym("pname"), 1, at);
        }
    }
    else
        post("[sfont~]: couldn't load %d", realname);
}

static void sfont_readhook(t_pd *z, t_symbol *fn, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    fluid_do_load((t_sfont *)z, fn);
}

static void sfont_click(t_sfont *x){
    panel_click_open(x->x_elsefilehandle);
}

static void sfont_open(t_sfont *x, t_symbol *s){
    if(s && s != &s_)
        fluid_do_load(x, s);
    else
        panel_click_open(x->x_elsefilehandle);
}

t_int *sfont_perform(t_int *w){
    t_sfont *x = (t_sfont *)(w[1]);
    t_sample *left = (t_sample *)(w[2]);
    t_sample *right = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    fluid_synth_write_float(x->x_synth, n, left, 0, 1, right, 0, 1);
    return(w+5);
}

static void sfont_dsp(t_sfont *x, t_signal **sp){
    dsp_add(sfont_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void sfont_free(t_sfont *x){
    if(x->x_synth)
        delete_fluid_synth(x->x_synth);
    if(x->x_settings)
        delete_fluid_settings(x->x_settings);
    if(x->x_elsefilehandle)
        elsefile_free(x->x_elsefilehandle);
}

static void *sfont_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_sfont *x = (t_sfont *)pd_new(sfont_class);
    x->x_elsefilehandle = elsefile_new((t_pd *)x, sfont_readhook, 0);
    x->x_synth = NULL;
    x->x_settings = NULL;
    x->x_sfname = NULL;
    x->x_tune_name = gensym("custom-tuning");
    x->x_base = 60;
    x->x_tune_ch = 1;
    x->x_tune_bank = x->x_tune_prog = 0;
    x->x_canvas = canvas_getcurrent();
    x->x_sysex = x->x_ready = x->x_count = 0;
    x->x_data = x->x_channel = x->x_type = 0;
    x->x_out_left = outlet_new(&x->x_obj, &s_signal);
    x->x_out_right = outlet_new(&x->x_obj, &s_signal);
    x->x_info_out = outlet_new((t_object *)x, gensym("list"));
    x->x_settings = new_fluid_settings();
    if(x->x_settings == NULL){
        pd_error(x, "[sfont~]: bug couldn't create synth settings\n");
        return(NULL);
    }
    x->x_ch = 16;
    int arg = 0;
    double g = 0.4;
    t_symbol *filename = NULL;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-v") && !arg){
                x->x_verbosity = 1;
                if(!printed){
                    sfont_getversion();
                    printed = 1;
                }
                ac--, av++;
            }
            else if(sym == gensym("-ch") && !arg){
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){
                    int ch = atom_getfloatarg(0, ac, av);
                    x->x_ch = ch < 16 ? 16 : ch > 256 ? 256 : ch;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(sym == gensym("-g") && !arg){
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){
                    g = (double)atom_getfloatarg(0, ac, av);
                    if(g < 0.1)
                        g = 0.1;
                    if(g > 1)
                        g = 1;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(!arg){
                arg = 1;
                filename = sym;
                ac--, av++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    fluid_settings_setint(x->x_settings, "synth.ladspa.active", 0);
    fluid_settings_setint(x->x_settings, "synth.midi-channels", x->x_ch);
    fluid_settings_setnum(x->x_settings, "synth.gain", g);
    fluid_settings_setnum(x->x_settings, "synth.sample-rate", sys_getsr());
    fluid_settings_setnum(x->x_settings, "synth.sample-rate", sys_getsr());
//  fluid_settings_setint(x->x_settings, "synth.polyphony", 256);
//    fluid_settings_setstr(x->x_settings, "synth.midi-bank-select", "gs");
    x->x_synth = new_fluid_synth(x->x_settings); // Create fluidsynth instance:
    if(x->x_synth == NULL){
        pd_error(x, "[sfont~]: bug couldn't create fluidsynth instance");
        return(NULL);
    }
    if(filename)
        fluid_do_load(x, filename);
    return(x);
errstate:
    pd_error(x, "[sfont~]: wrong args");
    return(NULL);
}
 
void sfont_tilde_setup(void){
    sfont_class = class_new(gensym("sfont~"), (t_newmethod)sfont_new,
        (t_method)sfont_free, sizeof(t_sfont), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_dsp, gensym("dsp"), A_CANT, 0);
    //    class_addmethod(sfont_class, (t_method)fluid_gen, gensym("gen"), A_GIMME, 0);
    class_addfloat(sfont_class, (t_method)sfont_float); // raw midi input
    class_addlist(sfont_class, (t_method)sfont_note); // list is the same as "note"
    class_addmethod(sfont_class, (t_method)sfont_note, gensym("note"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_open, gensym("open"), A_SYMBOL, 0);
    class_addmethod(sfont_class, (t_method)sfont_bank, gensym("bank"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_control_change, gensym("ctl"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_program_change, gensym("pgm"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_aftertouch, gensym("touch"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_polytouch, gensym("polytouch"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_pitch_bend, gensym("bend"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_sysex, gensym("sysex"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_panic, gensym("panic"), 0);
    class_addmethod(sfont_class, (t_method)sfont_transp, gensym("transp"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_pan, gensym("pan"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_scale, gensym("scale"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_set_tuning, gensym("set-tuning"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_sel_tuning, gensym("sel-tuning"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)sfont_unsel_tuning, gensym("unsel-tuning"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_base, gensym("base"), A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)sfont_remap, gensym("remap"), A_GIMME, 0);
//    class_addmethod(sfont_class, (t_method)sfont_a4, gensym("a4"), A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)sfont_getversion, gensym("version"), 0);
    class_addmethod(sfont_class, (t_method)sfont_verbose, gensym("verbose"), A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)sfont_info, gensym("info"), 0);
    class_addmethod(sfont_class, (t_method)sfont_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    elsefile_setup();
}

// https://www.fluidsynth.org/api/
// http://www.synthfont.com/sfspec24.pdf
