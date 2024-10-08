/*
 * File: cancel8.c
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
 * Test Synopsis: Test cancelling a blocked Win32 thread having created an
 * implicit POSIX handle for it.
 *
 * Test Method (Validation or Falsification):
 * - Validate return value and that POSIX handle is created and destroyed.
 *
 * Requirements Tested:
 * -
 *
 * Features Tested:
 * -
 *
 * Cases Tested:
 * -
 *
 * Description:
 * -
 *
 * Environment:
 * -
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - No output on success.
 *
 * Assumptions:
 * - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
 *   pthread_testcancel, pthread_cancel
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#include "test.h"
#ifndef _UWIN
#include <process.h>
#endif

/*
 * Create NUMTHREADS threads in addition to the Main thread.
 */
enum {
  NUMTHREADS = 4
};

typedef struct bag_t_ bag_t;
struct bag_t_ {
  int threadnum;
  int started;
  /* Add more per-thread state variables here */
  int count;
  pthread_t self;
};

static bag_t threadbag[NUMTHREADS + 1];

static pthread_cond_t CV = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t CVLock = PTHREAD_MUTEX_INITIALIZER;

static void PTW32_CDECL
pthread_mutex_cleanup(void *args)
{
	pthread_mutex_t *cvl = (pthread_mutex_t *)args;
	pthread_mutex_unlock(cvl);
}

static
#if ! defined (__MINGW32__) || defined (__MSVCRT__)
unsigned __stdcall
#else
void
#endif
Win32thread(void * arg)
{
  bag_t * bag = (bag_t *) arg;

  assert(bag->threadnum > 0);
  assert(bag->threadnum <= NUMTHREADS);
  assert(bag == &threadbag[bag->threadnum]);
  assert(bag->started == 0);
  bag->started = 1;

  assert((bag->self = pthread_self()).p != NULL);
  assert(pthread_kill(bag->self, 0) == 0);

  assert(pthread_mutex_lock(&CVLock) == 0);
  pthread_cleanup_push(pthread_mutex_cleanup, &CVLock);
  pthread_cond_wait(&CV, &CVLock);
  pthread_cleanup_pop(1);

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
  return 0;
#endif
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_cancel8(void)
#endif
{
  int failed = 0;
  int i;
  HANDLE h[NUMTHREADS + 1];
  unsigned thrAddr; /* Dummy variable to pass a valid location to _beginthreadex (Win98). */

  memset(threadbag, 0, sizeof(threadbag));
  for (i = 1; i <= NUMTHREADS; i++)
    {
      threadbag[i].started = 0;
      threadbag[i].threadnum = i;
#if ! defined (__MINGW32__) || defined (__MSVCRT__)
      h[i] = (HANDLE) _beginthreadex(NULL, 0, Win32thread, (void *) &threadbag[i], 0, &thrAddr);
#else
      h[i] = (HANDLE) _beginthread(Win32thread, 0, (void *) &threadbag[i]);
#endif
    }

  /*
   * Code to control or manipulate child threads should probably go here.
   */
  Sleep(500);

  /*
   * Cancel all threads.
   */
  for (i = 1; i <= NUMTHREADS; i++)
    {
      assert(pthread_kill(threadbag[i].self, 0) == 0);
      int rv = pthread_cancel(threadbag[i].self);
      if (rv != 0)
          fprintf(stderr, "Thread %d: rv = %d\n", i, rv);
      assert(rv == 0 /* || rv == ESRCH */ );
    }

  /*
   * Give threads time to run.
   */
  Sleep(NUMTHREADS * 100);

  /*
   * Standard check that all threads started.
   */
  for (i = 1; i <= NUMTHREADS; i++)
    {
      if (!threadbag[i].started)
	{
	  failed |= !threadbag[i].started;
	  fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
	}
    }

  assert(!failed);

  /*
   * Check any results here. Set "failed" and only print output on failure.
   */
  failed = 0;
  for (i = 1; i <= NUMTHREADS; i++)
    {
      int fail = 0;
      int result = 0;

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
      assert(GetExitCodeThread(h[i], (LPDWORD) &result) == TRUE);
#else
      /*
       * Can't get a result code.
       */
      result = (int)(size_t)PTHREAD_CANCELED;
#endif

      assert(threadbag[i].self.p != NULL);
      int rv = pthread_kill(threadbag[i].self, 0);
      if (rv != ESRCH)
          fprintf(stderr, "Thread %d: rv = %d\n", i, rv);
      assert(rv == ESRCH /* || rv == 0 */ );

      fail = (result != (intptr_t)PTHREAD_CANCELED);

      if (fail)
	{
	  fprintf(stderr, "Thread %d: started %d: count %d\n",
		  i,
		  threadbag[i].started,
		  threadbag[i].count);
	}
      failed = (failed || fail);
    }

  assert(!failed);

  /*
   * Success.
   */
  return 0;
}

