/* FluidLite - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *  
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#ifndef _FLUIDLITE_H
#define _FLUIDLITE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
#if defined(WIN32)
#if defined(FLUIDSYNTH_DLL_EXPORTS)
#define FLUIDSYNTH_API __declspec(dllexport)
#elif defined(FLUIDSYNTH_NOT_A_DLL)
#define FLUIDSYNTH_API
#else
#define FLUIDSYNTH_API __declspec(dllimport)
#endif

#elif defined(MACOS9)
#define FLUIDSYNTH_API __declspec(export)

#else
#define FLUIDSYNTH_API
#endif
*/
#if defined(__WATCOMC__) && defined(FLUIDSYNTH_DLL_EXPORTS)
#define FLUIDSYNTH_API __declspec(dllexport) /* watcom needs the dllexport */
#else
#define FLUIDSYNTH_API
#endif

/**
 * @file fluidlite.h
 * @brief FluidLite is a real-time synthesizer designed for SoundFont(R) files.
 *
 * This is the header of the fluidlite library and contains the
 * synthesizer's public API.
 *
 * Depending on how you want to use or extend the synthesizer you
 * will need different API functions. You probably do not need all
 * of them. Here is what you might want to do:
 *
 * o Embedded synthesizer: create a new synthesizer and send MIDI
 *   events to it. The sound goes directly to the audio output of
 *   your system.
 *
 * o Plugin synthesizer: create a synthesizer and send MIDI events
 *   but pull the audio back into your application.
 *
 * o SoundFont plugin: create a new type of "SoundFont" and allow
 *   the synthesizer to load your type of SoundFonts.
 *
 * o MIDI input: Create a MIDI handler to read the MIDI input on your
 *   machine and send the MIDI events directly to the synthesizer.
 *
 * o MIDI files: Open MIDI files and send the MIDI events to the
 *   synthesizer.
 *
 * o Command lines: You can send textual commands to the synthesizer.
 *
 * SoundFont(R) is a registered trademark of E-mu Systems, Inc.
 */

#include "fluidlite/types.h"
#include "fluidlite/settings.h"
#include "fluidlite/synth.h"
#include "fluidlite/sfont.h"
#include "fluidlite/ramsfont.h"
#include "fluidlite/log.h"
#include "fluidlite/misc.h"
#include "fluidlite/mod.h"
#include "fluidlite/gen.h"
#include "fluidlite/voice.h"
#include "fluidlite/version.h"


#ifdef __cplusplus
}
#endif

#endif /* _FLUIDLITE_H */
