/*
 * delay1.c
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
 * Depends on API functions:
 *    pthread_delay_np
 */

#include "test.h"

static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

static void PTW32_CDECL
pthread_mutex_cleanup(void *args)
{
	pthread_mutex_t *cvl = (pthread_mutex_t *)args;
	pthread_mutex_unlock(cvl);
}

static void *
func(void * arg)
{
  struct timespec interval = {5, 500000000L};

  assert(pthread_mutex_lock(&mx) == 0);

#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
  pthread_cleanup_push(pthread_mutex_cleanup, &mx);
  assert(pthread_delay_np(&interval) == 0);
  pthread_cleanup_pop(1);
#ifdef _MSC_VER
#pragma inline_depth()
#endif

  return (void *)(size_t)1;
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_delay2(void)
#endif
{
  pthread_t t;
  void* result = (void*)0;

  assert(pthread_mutex_lock(&mx) == 0);

  assert(pthread_create(&t, NULL, func, NULL) == 0);
  assert(pthread_cancel(t) == 0);

  assert(pthread_mutex_unlock(&mx) == 0);

  assert(pthread_join(t, &result) == 0);
  assert(result == (void*)PTHREAD_CANCELED);

  return 0;
}

