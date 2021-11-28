/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#ifndef __X_LIBPD_MULI_H__
#define __X_LIBPD_MULI_H__

#ifdef __cplusplus
extern "C"
{
#endif
    
#include <z_libpd.h>

void libpd_multi_init(void);

typedef void (*t_libpd_multi_banghook)(void* ptr, const char *recv);
typedef void (*t_libpd_multi_floathook)(void* ptr, const char *recv, float f);
typedef void (*t_libpd_multi_symbolhook)(void* ptr, const char *recv, const char *s);
typedef void (*t_libpd_multi_listhook)(void* ptr, const char *recv, int argc, t_atom *argv);
typedef void (*t_libpd_multi_messagehook)(void* ptr, const char *recv, const char *msg, int argc, t_atom *argv);

void* libpd_multi_receiver_new(void* ptr, char const *s,
                               t_libpd_multi_banghook hook_bang,
                               t_libpd_multi_floathook hook_float,
                               t_libpd_multi_symbolhook hook_symbol,
                               t_libpd_multi_listhook hook_list,
                               t_libpd_multi_messagehook hook_message);


typedef void (*t_libpd_multi_noteonhook)(void* ptr, int channel, int pitch, int velocity);
typedef void (*t_libpd_multi_controlchangehook)(void* ptr, int channel, int controller, int value);
typedef void (*t_libpd_multi_programchangehook)(void* ptr, int channel, int value);
typedef void (*t_libpd_multi_pitchbendhook)(void* ptr, int channel, int value);
typedef void (*t_libpd_multi_aftertouchhook)(void* ptr, int channel, int value);
typedef void (*t_libpd_multi_polyaftertouchhook)(void* ptr, int channel, int pitch, int value);
typedef void (*t_libpd_multi_midibytehook)(void* ptr, int port, int byte);

void* libpd_multi_midi_new(void* ptr,
                           t_libpd_multi_noteonhook hook_noteon,
                           t_libpd_multi_controlchangehook hook_controlchange,
                           t_libpd_multi_programchangehook hook_programchange,
                           t_libpd_multi_pitchbendhook hook_pitchbend,
                           t_libpd_multi_aftertouchhook hook_aftertouch,
                           t_libpd_multi_polyaftertouchhook hook_polyaftertouch,
                           t_libpd_multi_midibytehook hook_midibyte);

typedef void (*t_libpd_multi_printhook)(void* ptr, const char *recv);

void* libpd_multi_print_new(void* ptr, t_libpd_multi_printhook hook_print);

#ifdef __cplusplus
}
#endif

#endif
