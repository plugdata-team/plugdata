/* Copyright (c) 2018 Diego Barrios Romero.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* API symbol definitions.
 * Symbols that are part of the library interface need to be exported
 * so that these can be found by the loaders.
 * i.e. *_setup functions need to be exported so that pure-data finds them
 * at load time.
 */

#ifndef __CYCLONE_API_H__
#define __CYCLONE_API_H__

// For use in the cyclone main library
#if defined(_MSC_VER)
// Export the symbol to the DLL interface
#  define CYCLONE_API __declspec(dllexport)
#else
// On UNIX set this to visibility default if you change the default symbol
// visibility to hidden.
#  define CYCLONE_API
#endif

// For use on cyclone objects
#if defined(_MSC_VER) && !defined(CYCLONE_SINGLE_LIBRARY)
// Export the symbol to the DLL interface
#  define CYCLONE_OBJ_API __declspec(dllexport)
#else
// On UNIX set this to visibility default if you change the default symbol
// visibility to hidden and you are not building a single library.
#  define CYCLONE_OBJ_API
#endif


#endif // __CYCLONE_API_H__
