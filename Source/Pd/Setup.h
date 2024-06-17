/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

extern "C" {
#include <z_libpd.h>
#include <s_stuff.h>
}

typedef void (*t_plugdata_banghook)(void* ptr, char const* recv);
typedef void (*t_plugdata_floathook)(void* ptr, char const* recv, float f);
typedef void (*t_plugdata_symbolhook)(void* ptr, char const* recv, char const* s);
typedef void (*t_plugdata_listhook)(void* ptr, char const* recv, int argc, t_atom* argv);
typedef void (*t_plugdata_messagehook)(void* ptr, char const* recv, char const* msg, int argc, t_atom* argv);
typedef void (*t_plugdata_noteonhook)(void* ptr, int channel, int pitch, int velocity);
typedef void (*t_plugdata_controlchangehook)(void* ptr, int channel, int controller, int value);
typedef void (*t_plugdata_programchangehook)(void* ptr, int channel, int value);
typedef void (*t_plugdata_pitchbendhook)(void* ptr, int channel, int value);
typedef void (*t_plugdata_aftertouchhook)(void* ptr, int channel, int value);
typedef void (*t_plugdata_polyaftertouchhook)(void* ptr, int channel, int pitch, int value);
typedef void (*t_plugdata_midibytehook)(void* ptr, int port, int byte);
typedef void (*t_plugdata_printhook)(void* ptr, void* obj, char const* recv);

namespace pd {

struct Setup {
    static int initialisePd();

    void parseArguments(char const** args, size_t argc, t_namelist** sys_openlist, t_namelist** sys_messagelist);

    static void initialisePdLua(char const* datadir, char* vers, int vers_len, void (*register_class_callback)(char const*));
    static void initialisePdLuaInstance();
    static void initialiseELSE();
    static void initialiseCyclone();
    static void initialiseGem(std::string const& gemPluginPath);

    static void* createMIDIHook(void* ptr,
        t_plugdata_noteonhook hook_noteon,
        t_plugdata_controlchangehook hook_controlchange,
        t_plugdata_programchangehook hook_programchange,
        t_plugdata_pitchbendhook hook_pitchbend,
        t_plugdata_aftertouchhook hook_aftertouch,
        t_plugdata_polyaftertouchhook hook_polyaftertouch,
        t_plugdata_midibytehook hook_midibyte);

    static void* createReceiver(void* ptr, char const* s,
        t_plugdata_banghook hook_bang,
        t_plugdata_floathook hook_float,
        t_plugdata_symbolhook hook_symbol,
        t_plugdata_listhook hook_list,
        t_plugdata_messagehook hook_message);

    static void* createPrintHook(void* ptr, t_plugdata_printhook hook_print);
};

}
