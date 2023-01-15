/*
 * mutex6n.c
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
 * Tests PTHREAD_MUTEX_NORMAL mutex type.
 * Thread locks mutex twice (recursive lock).
 * The thread should deadlock.
 *
 * Depends on API functions:
 *      pthread_create()
 *      pthread_mutexattr_init()
 *      pthread_mutexattr_settype()
 *      pthread_mutexattr_gettype()
 *      pthread_mutex_init()
 *	pthread_mutex_lock()
 *	pthread_mutex_unlock()
 */

#include "test.h"

#if !defined(WINCE)
#  include <signal.h>
#endif

static int lockCount = 0;

static pthread_mutex_t mutex;
static pthread_mutexattr_t mxAttr;

static void * locker(void * arg)
{
  assert(pthread_mutex_lock(&mutex) == 0);
  lockCount++;

  /* Should wait here (deadlocked) */
  assert(pthread_mutex_lock(&mutex) == 0);

  lockCount++;
  assert(pthread_mutex_unlock(&mutex) == 0);

  return (void *) 555;
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_mutex6n(void)
#endif
{
  pthread_t t;
  int mxType = -1;

  assert(pthread_mutexattr_init(&mxAttr) == 0);

  BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
  {
	  lockCount = 0;
	  assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	  assert(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	  assert(mxType == PTHREAD_MUTEX_NORMAL);

	  assert(pthread_mutex_init(&mutex, &mxAttr) == 0);

	  assert(pthread_create(&t, NULL, locker, NULL) == 0);

  while (lockCount < 1)
    {
      Sleep(1);
    }

	  assert(lockCount == 1);

	  assert(pthread_mutex_unlock(&mutex) == (IS_ROBUST ? EPERM : 0));

  while (lockCount < (IS_ROBUST?1:2))
    {
      Sleep(1);
    }

	  assert(lockCount == (IS_ROBUST ? 1 : 2));
	  
	  Sleep(300);

	  // kill the deadlocked thread:
	  assert(pthread_kill(t, SIGABRT) == 0);
	  
	  void* result = (void*)0;

	  /*
	   * The thread does not contain any cancellation points, so
	   * a return value of PTHREAD_CANCELED confirms that async
	   * cancellation succeeded.
	   */
	  assert(pthread_join(t, &result) == 0);
	  assert(result == PTHREAD_CANCELED || result == (void *)555);

	  // mutex is completely clobbered due to deadlock + thread kill:
	  // its owwner is now dead and gone so we lost the mutex forever...
	  (void)pthread_mutex_destroy(&mutex);
	  
	  //assert(mutex == NULL);
  }
  END_MUTEX_STALLED_ROBUST(mxAttr)

  assert(pthread_mutexattr_destroy(&mxAttr) == 0);

  return 0;
}

