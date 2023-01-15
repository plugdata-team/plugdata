/*
 * File: cancel2.c
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
 * Test Synopsis: Test SEH or C++ cancel exception handling within
 * application exception blocks.
 *
 * Test Method (Validation or Falsification):
 * -
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
 *   pthread_testcancel, pthread_cancel, pthread_join
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#include "test.h"

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
};

static bag_t threadbag[NUMTHREADS + 1];

static pthread_barrier_t go = NULL;

static void *
mythread(void * arg)
{
  int result = 0;
  bag_t * bag = (bag_t *) arg;

  assert(bag == &threadbag[bag->threadnum]);
  assert(bag->started == 0);
  bag->started = 1;

  /* Set to known state and type */

  assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
  assert(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
  result = 1;

#if !defined(__cplusplus)
  __try
#else
  try
#endif
    {
      /* Wait for go from main */
      pthread_barrier_wait(&go);
      pthread_barrier_wait(&go);

      pthread_testcancel();
    }
#if !defined(__cplusplus)
  __except(EXCEPTION_EXECUTE_HANDLER)
#else
#if defined(__PtW32CatchAll)
  __PtW32CatchAll
#else
  catch(...)
#endif
#endif
    {
      /*
       * Should not get into here.
       */
      result += 100;
    }

  /*
   * Should not get to here either.
   */
  result += 1000;

  return (void *)(size_t)result;
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_cancel2(void)
#endif
{
  int failed = 0;
  int i;
  pthread_t t[NUMTHREADS + 1];

  assert((t[0] = pthread_self()).p != NULL);
  assert(pthread_barrier_init(&go, NULL, NUMTHREADS + 1) == 0);

  for (i = 1; i <= NUMTHREADS; i++)
    {
      threadbag[i].started = 0;
      threadbag[i].threadnum = i;
      assert(pthread_create(&t[i], NULL, mythread, (void *) &threadbag[i]) == 0);
    }

  /*
   * Code to control or manipulate child threads should probably go here.
   */

  pthread_barrier_wait(&go);

  for (i = 1; i <= NUMTHREADS; i++)
    {
      assert(pthread_cancel(t[i]) == 0);
    }

  pthread_barrier_wait(&go);

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
      void* result = (void*)0;

      assert(pthread_join(t[i], &result) == 0);
      fail = (result != PTHREAD_CANCELED);
      if (fail)
	{
	  fprintf(stderr, "Thread %d: started %d: location %d\n",
		  i,
		  threadbag[i].started,
		  (int)(size_t)result);
	}
      failed |= fail;
    }

  assert(!failed);
  assert(pthread_barrier_destroy(&go) == 0);

  /*
   * Success.
   */
  return 0;
}

