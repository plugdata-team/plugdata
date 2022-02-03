/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __SHARED_H__
#define __SHARED_H__

/* Microsoft Visual Studio is not C99, it does not provide stdint.h */
#ifdef _MSC_VER
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef signed __int64    int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;
#elif defined(IRIX)
typedef long int32_t;
typedef unsigned long uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uchar;
#else
#include <stdint.h>
#endif


/* LATER find a proper place for #include <limits.h> */
#ifdef INT_MAX
#define SHARED_INT_MAX  INT_MAX
#else
#define SHARED_INT_MAX  0x7FFFFFFF
#endif
#ifdef INT_MIN
#define SHARED_INT_MIN  INT_MIN
#else
#define SHARED_INT_MIN  ((int)0x80000000)
#endif
/* LATER find a proper place for #include <float.h> */
#ifdef FLT_MAX
#define SHARED_FLT_MAX  FLT_MAX
#else
#define SHARED_FLT_MAX  1E+36
#endif


/* this is for GNU/Linux, GNU/Hurd, GNU/kFreeBSD */
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__)
#include <endian.h>
# if !defined(__BYTE_ORDER) || !defined(__LITTLE_ENDIAN)                         
#  error No byte order defined                                                    
# endif                                                                          
# if __BYTE_ORDER == __LITTLE_ENDIAN                                             
#  define SHARED_HIOFFSET   1
#  define SHARED_LOWOFFSET  0
# else
#  define SHARED_HIOFFSET   0
#  define SHARED_LOWOFFSET  1
# endif
#endif

#ifdef _WIN32
#define SHARED_HIOFFSET   1
#define SHARED_LOWOFFSET  0
#endif

#ifdef IRIX
#define SHARED_HIOFFSET   0
#define SHARED_LOWOFFSET  1
#endif

#if defined __FreeBSD__ || defined(__APPLE__)
#include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#  define SHARED_HIOFFSET   1
#  define SHARED_LOWOFFSET  0
# else
#  define SHARED_HIOFFSET   0
#  define SHARED_LOWOFFSET  1
# endif
#endif

#define SHARED_UNITBIT32  1572864.  /* 3*(2^19) gives 32 fractional bits */
#define SHARED_UNITBIT0  6755399441055744.  /* 3*(2^51), no fractional bits */
#define SHARED_UNITBIT0_HIPART  0x43380000

typedef union _shared_wrappy
{
    double  w_d;
    int32_t w_i[2];
} t_shared_wrappy;

typedef union _shared_floatint
{
    t_float  fi_f;
    int32_t  fi_i;
} t_shared_floatint;

#define SHARED_TRUEBITS  0x3f800000  /* t_float f = 1; *(int32_t *)&f */

#define SHARED_PI   3.14159265359
#define SHARED_2PI  6.28318530718

#ifndef PD_BADFLOAT
#ifdef __i386__
#define PD_BADFLOAT(f) ((((*(unsigned int*)&(f))&0x7f800000)==0) || \
    (((*(unsigned int*)&(f))&0x7f800000)==0x7f800000))
#else
#define PD_BADFLOAT(f) 0
#endif
#endif

#ifndef PD_BIGORSMALL
#ifdef __i386__
#define PD_BIGORSMALL(f) ((((*(unsigned int*)&(f))&0x60000000)==0) || \
    (((*(unsigned int*)&(f))&0x60000000)==0x60000000))
#else
#define PD_BIGORSMALL(f) 0
#endif
#endif

#endif
