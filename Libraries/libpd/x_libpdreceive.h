/*
 * Copyright (c) 2010 Peter Brinkmann (peter.brinkmann@gmail.com)
 * Copyright (c) 2012-2019 libpd team
 *
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/libpd/libpd/wiki for documentation
 *
 */

#ifndef __X_LIBPDREC_H__
#define __X_LIBPDREC_H__

#include "m_pd.h"

// internal "virtual" libpd receive object which forwards events to hooks
// do *not* include this file in a user-facing header

// set up the libpd source receiver class
void libpdreceive_setup(void);

// create a new libpd source receiver with a given name symbol
void *libpdreceive_new(t_symbol *);

#endif
