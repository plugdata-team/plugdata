/*
 * Copyright (c) 2013 Dan Wilcox (danomatika@gmail.com)
 * Copyright (c) 2013-2019 libpd team
 *
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/libpd/libpd/wiki for documentation
 *
 */

#include "z_hooks.h"

t_libpd_banghook libpd_banghook = NULL;
t_libpd_floathook libpd_floathook = NULL;
t_libpd_symbolhook libpd_symbolhook = NULL;
t_libpd_listhook libpd_listhook = NULL;
t_libpd_messagehook libpd_messagehook = NULL;

t_libpd_noteonhook libpd_noteonhook = NULL;
t_libpd_controlchangehook libpd_controlchangehook = NULL;
t_libpd_programchangehook libpd_programchangehook = NULL;
t_libpd_pitchbendhook libpd_pitchbendhook = NULL;
t_libpd_aftertouchhook libpd_aftertouchhook = NULL;
t_libpd_polyaftertouchhook libpd_polyaftertouchhook = NULL;
t_libpd_midibytehook libpd_midibytehook = NULL;
