/*
 * rwlock6_t.c
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
 * Check writer and reader locking with reader timeouts
 *
 * Depends on API functions:
 *      pthread_rwlock_timedrdlock()
 *      pthread_rwlock_wrlock()
 *      pthread_rwlock_unlock()
 */

#include "test.h"
#include <sys/timeb.h>

static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;

static int bankAccount = 0;

static void * wrfunc(void * arg)
{
  assert(pthread_rwlock_wrlock(&rwlock1) == 0);
  Sleep(2000);
  bankAccount += 10;
  assert(pthread_rwlock_unlock(&rwlock1) == 0);

  return ((void *)(size_t)bankAccount);
}

static void * rdfunc(void * arg)
{
  int ba = -1;
  struct timespec abstime;

  (void) pthread_win32_getabstime_np(&abstime, NULL);

  if ((int) (size_t)arg == 1)
    {
      abstime.tv_sec += 1;
      assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == ETIMEDOUT);
      ba = 0;
    }
  else if ((int) (size_t)arg == 2)
    {
      abstime.tv_sec += 3;
      assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == 0);
      ba = bankAccount;
      assert(pthread_rwlock_unlock(&rwlock1) == 0);
    }

  return ((void *)(size_t)ba);
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_rwlock6_t(void)
#endif
{
  pthread_t wrt1;
  pthread_t wrt2;
  pthread_t rdt1;
  pthread_t rdt2;
  void* wr1Result = (void*)0;
  void* wr2Result = (void*)0;
  void* rd1Result = (void*)0;
  void* rd2Result = (void*)0;

  bankAccount = 0;

  assert(pthread_create(&wrt1, NULL, wrfunc, NULL) == 0);
  Sleep(500);
  assert(pthread_create(&rdt1, NULL, rdfunc, (void *)(size_t)1) == 0);
  Sleep(500);
  assert(pthread_create(&wrt2, NULL, wrfunc, NULL) == 0);
  Sleep(500);
  assert(pthread_create(&rdt2, NULL, rdfunc, (void *)(size_t)2) == 0);

  assert(pthread_join(wrt1, &wr1Result) == 0);
  assert(pthread_join(rdt1, &rd1Result) == 0);
  assert(pthread_join(wrt2, &wr2Result) == 0);
  assert(pthread_join(rdt2, &rd2Result) == 0);

  assert((int)(size_t)wr1Result == 10);
  assert((int)(size_t)rd1Result == 0);
  assert((int)(size_t)wr2Result == 20);
  /*
     [i_a]

	 On Win32 when running in the MSVC2005 DEBUGGER
	 a write lock is a read lock so both rdt2 and wrt2 are waiting
     simultaneously for wrt1 to release the lock on BankAccount. Either
	 thread may grab it first once the 2000 msec delay in wrt1 has
	 expired.

	 Hence rd2Result == 10 for systems which support simultaneous read AND
	 write locks, while rd2Result == 20 for systems which only know about ONE
	 sort of lock, without caring for READ or WRITE.
   */
  if ((int)(size_t)rd2Result == 10)
  {
	  printf("This system supports only one kind of locking, and does not\n"
		     "consider read locks and write locks to be separate things.\n");
  }
  else
  {
      assert((int)(size_t)rd2Result == 20);
  }

  return 0;
}
