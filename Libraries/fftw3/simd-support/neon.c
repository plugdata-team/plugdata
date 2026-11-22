/*
 * Copyright (c) 2003, 2007-14 Matteo Frigo
 * Copyright (c) 2003, 2007-14 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "kernel/ifftw.h"

#if HAVE_NEON

/* plugdata: NEON (Advanced SIMD) is mandatory on AArch64. The probe below
   executes an AArch32 encoding, which on AArch64 is a valid instruction that
   silently clobbers x16 and the flags, so don't run it there. */
#if defined(__aarch64__) || defined(_M_ARM64)
  int X(have_simd_neon)(void)
  {
       return 1;
  }

/* check for an environment where signals are known to work */
#elif defined(unix) || defined(linux)
  # include <signal.h>
  # include <setjmp.h>

  static jmp_buf jb;

  static void sighandler(int x)
  {
       UNUSED(x);
       longjmp(jb, 1);
  }

  static int really_have_neon(void)
  {
       void (*oldsig)(int);
       oldsig = signal(SIGILL, sighandler);
       if (setjmp(jb)) {
	    signal(SIGILL, oldsig);
	    return 0;
       } else {
	    /* paranoia: encode the instruction in binary because the
	       assembler may not recognize it without -mfpu=neon */
	    /*asm volatile ("vand q0, q0, q0");*/
	    asm volatile (".long 0xf2000150");
	    signal(SIGILL, oldsig);
	    return 1;
       }
  }

  int X(have_simd_neon)(void)
  {
       static int init = 0, res;

       if (!init) {
	    res = really_have_neon();
	    init = 1;
       }
       return res;
  }


#else
/* don't know how to autodetect NEON; assume it is present */
  int X(have_simd_neon)(void)
  {
       return 1;
  }
#endif

#endif
