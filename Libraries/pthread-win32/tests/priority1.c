/*
 * File: priority1.c
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
 * Test Synopsis:
 * - Test thread priority explicit setting using thread attribute.
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
 * -
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#include "test.h"

enum {
  PTW32TEST_THREAD_INIT_PRIO = 0,
  PTW32TEST_MAXPRIORITIES = 512
};

static int minPrio;
static int maxPrio;
static int validPriorities[PTW32TEST_MAXPRIORITIES];

static void *
func(void * arg)
{
  int policy;
  struct sched_param param;
  pthread_t threadID = pthread_self();

  assert(pthread_getschedparam(threadID, &policy, &param) == 0);
  assert(policy == SCHED_OTHER);
  return (void *) (size_t)(param.sched_priority);
}

static void *
getValidPriorities(void * arg)
{
  int prioSet;
  HANDLE process = GetCurrentProcess();
  pthread_t threadID = pthread_self();
  HANDLE threadH = pthread_getw32threadhandle_np(threadID);
  DWORD mode;

	mode = GetPriorityClass(process);
	if (!mode)
	{
		printf("ERROR: Failed to fetch process Priority Class (%d)\n", (int)GetLastError());
      exit(1);
	}
	/*
	   From the Windows documentation:

	   If the thread has the REALTIME_PRIORITY_CLASS base class, this function can also
	   return one of the following values:
	     -7, -6, -5, -4, -3, 3, 4, 5, or 6.
	   For more information, see Scheduling Priorities.


	   It turns out during testing from the MSVC2005 IDE, that the process is started in
	   REALTIME_PRIORITY_CLASS, but that any new created threads here-in are NOT in that
	   class by default, so the prio-compare assert() in the test routine will fail as
	   the validPriorities[] array will have been filled with 'REAL_TIME' class allowed
	   values here if we didn't change our own class first...
     */
	if (mode == REALTIME_PRIORITY_CLASS)
	{
		if (!SetPriorityClass(process, NORMAL_PRIORITY_CLASS))
		{
			printf("ERROR: Failed to set process Priority Class (%d)\n", (int)GetLastError());
		  exit(1);
		}
	}
	mode = GetPriorityClass(process);

	printf("Using GetThreadPriority (class = $%05X: %s)\n", (unsigned int)mode, (mode & REALTIME_PRIORITY_CLASS ? "REAL_TIME" : ""));
  printf("%10s %10s\n", "Set value", "Get value");

  for (prioSet = minPrio;
       prioSet <= maxPrio;
       prioSet++)
    {
	/*
       * If prioSet is invalid then the threads priority is unchanged
       * from the previous value. Make the previous value a known
       * one so that we can check later.
       */
        if (prioSet < 0)
	  SetThreadPriority(threadH, THREAD_PRIORITY_LOWEST);
        else
	  SetThreadPriority(threadH, THREAD_PRIORITY_HIGHEST);
	SetThreadPriority(threadH, prioSet);
	validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
	printf("%10d %10d\n", prioSet, validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)]);
    }

  return (void *) 0;
}


#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_priority1(void)
#endif
{
  pthread_t t;
  pthread_attr_t attr;
  void * result = NULL;
  struct sched_param param;

  assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
  assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);

  assert(pthread_create(&t, NULL, getValidPriorities, NULL) == 0);
  assert(pthread_join(t, &result) == 0);

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);

  /* Set the thread's priority to a known initial value. */
  SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()),
                    PTW32TEST_THREAD_INIT_PRIO);

  printf("Using pthread_getschedparam\n");
  printf("%10s %10s %10s\n", "Set value", "Get value", "Win priority");

  for (param.sched_priority = minPrio;
       param.sched_priority <= maxPrio;
       param.sched_priority++)
    {
      int prio;

      assert(pthread_attr_setschedparam(&attr, &param) == 0);
      assert(pthread_create(&t, &attr, func, (void *) &attr) == 0);

	  assert(param.sched_priority+(PTW32TEST_MAXPRIORITIES/2) >= 0);
	  assert(param.sched_priority+(PTW32TEST_MAXPRIORITIES/2) < PTW32TEST_MAXPRIORITIES);
	  prio = GetThreadPriority(pthread_getw32threadhandle_np(t));
#if 0
	  printf("DBG: %10d %10d %10d\n", param.sched_priority, prio, validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
#endif
	  /*
	     See additional info in the getValidPriorities() function above: when run from
		 the MSVC2005 IDE this test would fail unless we'd set the PROCESS class back to
		 NORMAL first up there!
       */
      assert(prio == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);

      assert(pthread_join(t, &result) == 0);

      assert(param.sched_priority == (int)(size_t) result);
      printf("%10d %10d %10d\n", param.sched_priority, (int)(size_t) result, prio);
    }

  return 0;
}
