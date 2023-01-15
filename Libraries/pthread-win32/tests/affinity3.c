/*
 * affinity3.c
 *
 *
 * --------------------------------------------------------------------------
 *
 *      pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999-2021 pthreads-win32 / pthreads4w contributors
 *
 *      Homepage1: http://sourceware.org/pthreads-win32/
 *      Homepage2: http://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * --------------------------------------------------------------------------
 *
 * Have the thread switch CPUs.
 *
 */

#if ! defined(WINCE)

#include "test.h"

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else 
int
test_affinity3(void)
#endif
{
  int result;
  unsigned int cpu;
  cpu_set_t newmask;
  cpu_set_t processCpus;
  cpu_set_t mask;
  cpu_set_t switchmask;
  cpu_set_t flipmask;
  pthread_t self = pthread_self();

  CPU_ZERO(&mask);
  CPU_ZERO(&switchmask);
  CPU_ZERO(&flipmask);

  if (pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == ENOSYS)
    {
      printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
      return 0;
    }
  assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
  printf("This thread has a starting affinity with %d CPUs\n", CPU_COUNT(&processCpus));
  assert(!CPU_EQUAL(&mask, &processCpus));

  for (cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2)
    {
	  CPU_SET(cpu, &switchmask);				/* 0b01010101010101010101010101010101 */
    }
  for (cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu++)
    {
	  CPU_SET(cpu, &flipmask);					/* 0b11111111111111111111111111111111 */
    }

  result = pthread_setaffinity_np(self, sizeof(cpu_set_t), &processCpus);
  if (result != 0)
	{
	  assert(result != ESRCH);
	  assert(result != EFAULT);
	  assert(result != EPERM);
	  assert(result != EINVAL);
	  assert(result != EAGAIN);
	  assert(result == ENOSYS);
	  assert(CPU_COUNT(&mask) == 1);
	}
  else
	{
	  if (CPU_COUNT(&mask) > 1)
		{
		  CPU_AND(&newmask, &processCpus, &switchmask); /* Remove every other CPU */
		  assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
		  assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
		  assert(CPU_EQUAL(&mask, &newmask));
		  CPU_XOR(&newmask, &mask, &flipmask);  /* Switch to all alternative CPUs */
		  assert(!CPU_EQUAL(&mask, &newmask));
		  assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
		  assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
		  assert(CPU_EQUAL(&mask, &newmask));
		}
	}

  return 0;
}

#else

#include <stdio.h>

int
main()
{
  fprintf(stderr, "Test N/A for this target environment.\n");
  return 0;
}

#endif
