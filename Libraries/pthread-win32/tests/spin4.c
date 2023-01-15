/*
 * spin4.c
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
 * Declare a static spinlock object, lock it, spin on it,
 * and then unlock it again.
 */

#include "test.h"
#include <sys/timeb.h>

static pthread_spinlock_t lock = PTHREAD_SPINLOCK_INITIALIZER;
static PTW32_STRUCT_TIMEB currSysTimeStart;
static PTW32_STRUCT_TIMEB currSysTimeStop;

/* [i_a] */
#define GetDurationMilliSecs(_TStart, _TStop) ((long)((_TStop.time*1000LL+_TStop.millitm) \
                                               - (_TStart.time*1000LL+_TStart.millitm)))

static int washere = 0;

static void * func(void * arg)
{
  PTW32_FTIME(&currSysTimeStart);
  washere = 1;
  assert(pthread_spin_lock(&lock) == 0);
  assert(pthread_spin_unlock(&lock) == 0);
  PTW32_FTIME(&currSysTimeStop);

  return (void *)(size_t)GetDurationMilliSecs(currSysTimeStart, currSysTimeStop);
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_spin4(void)
#endif
{
  void* result = (void*)0;
  pthread_t t;
  int CPUs;
  PTW32_STRUCT_TIMEB sysTime;

  if ((CPUs = pthread_num_processors_np()) == 1)
    {
      printf("Test not run - it requires multiple CPUs.\n");
	return 0;
    }

  assert(pthread_spin_lock(&lock) == 0);

  assert(pthread_create(&t, NULL, func, NULL) == 0);

  while (washere == 0)
    {
      sched_yield();
    }

  do
    {
      sched_yield();
      PTW32_FTIME(&sysTime);
    }
  while (GetDurationMilliSecs(currSysTimeStart, sysTime) <= 1000);

  assert(pthread_spin_unlock(&lock) == 0);

  assert(pthread_join(t, &result) == 0);
  assert((int)(size_t)result > 1000);

  assert(pthread_spin_destroy(&lock) == 0);

  assert(washere == 1);

  return 0;
}
