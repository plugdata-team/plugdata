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

#ifndef _FLUIDLITE_VERSION_H
#define _FLUIDLITE_VERSION_H


#ifdef __cplusplus
extern "C" {
#endif

#define FLUIDLITE_VERSION       "@fluidlite_VERSION@"
#define FLUIDLITE_VERSION_MAJOR @fluidlite_VERSION_MAJOR@
#define FLUIDLITE_VERSION_MINOR @fluidlite_VERSION_MINOR@
#define FLUIDLITE_VERSION_MICRO @fluidlite_VERSION_PATCH@


FLUIDSYNTH_API void fluid_version(int *major, int *minor, int *micro);

FLUIDSYNTH_API char* fluid_version_str(void);


#ifdef __cplusplus
}
#endif

#endif /* _FLUIDLITE_VERSION_H */
