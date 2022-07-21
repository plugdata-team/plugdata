/*
 * Copyright (c) 2013 Dan Wilcox (danomatika@gmail.com) &
 *                    Peter Brinkmann (peter.brinkmann@gmail.com)
 *
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/libpd/libpd/wiki for documentation
 *
 */

#ifndef __Z_PRINT_UTIL_H__
#define __Z_PRINT_UTIL_H__

#include "z_libpd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// concatenate print messages into single lines before returning them to the
/// print hook:
///    ie. line "hello 123\n" is received in 1 part -> "hello 123"
/// for comparison, the default behavior receives individual words and spaces:
///
/// ie. line "hello 123" is sent in 3 parts -> "hello", " ", "123\n"

/// assign the pointer to your print handler
EXTERN void libpd_set_concatenated_printhook(const t_libpd_printhook hook);

/// assign this function pointer to libpd_printhook or libpd_queued_printhook,
/// depending on whether you're using queued messages, to intercept and
/// concatenate print messages:
///     libpd_set_printhook(libpd_print_concatenator);
///     or
///     libpd_set_concatenated_printhook(your_print_handler);
/// note: the char pointer argument is only good for the duration of the print
///       callback; if you intend to use the argument after the callback has
///       returned, you need to make a defensive copy
EXTERN void libpd_print_concatenator(const char *s);

#ifdef __cplusplus
}
#endif

#endif
