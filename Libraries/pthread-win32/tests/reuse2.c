/*
 * File: reuse2.c
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
 * - Test that thread reuse works for detached threads.
 * - Analyse thread struct reuse.
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
 * - This test is implementation specific
 * because it uses knowledge of internals that should be
 * opaque to an application.
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

/*
 */

enum {
	NUMTHREADS = 10000
};


static long done = 0;

static void * func(void * arg)
{
  sched_yield();

  InterlockedIncrement(&done);

  return (void *) 0; 
}
 
#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else 
int
test_reuse2(void)
#endif
{
  pthread_t t[NUMTHREADS];
  pthread_attr_t attr;
  int i;
  unsigned int notUnique = 0, totalHandles = 0;
  size_t reuseMax = 0, reuseMin = NUMTHREADS;
  int actual_count = NUMTHREADS;

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);

  /*
     [i_a] when running this code from the MSVC2005 debugger, the number of successfully created
	 threads is lower than the initially configured 10K: 3132 on this system...

	 Hence the stupid heuristics in there...
   */
  for (i = 0; i < actual_count; i++)
    {
		int ret = pthread_create(&t[i], &attr, func, NULL);
		if (i <= 2048)
		{
			assert(ret == 0);
		}
		else if (ret == EAGAIN)
		{
			actual_count = i;
			break;
		}
		else
		{
			assert(ret == 0);
		}
    }

  while (actual_count > InterlockedExchangeAdd((LPLONG)&done, 0L))
    Sleep(100);

  Sleep(100);

  /*
   * Analyse reuse by computing min and max number of times pthread_create()
   * returned the same pthread_t value.
   */
  for (i = 0; i < actual_count; i++)
    {
      if (t[i].p != NULL)
        {
          int j;
		  size_t thisMax;

          thisMax = t[i].x;

          for (j = i+1; j < actual_count; j++)
            if (t[i].p == t[j].p)
              {
				if (t[i].x == t[j].x)
				  notUnique++;
                if (thisMax < t[j].x)
                  thisMax = t[j].x;
                t[j].p = NULL;
              }

          if (reuseMin > thisMax)
            reuseMin = thisMax;

          if (reuseMax < thisMax)
            reuseMax = thisMax;
        }
    }

  for (i = 0; i < actual_count; i++)
  {
    if (t[i].p != NULL)
	{
		totalHandles++;
	}
  }

  /*
   * pthread_t reuse counts start at 0, so we need to add 1
   * to the max and min values derived above.
   */
  printf("For %u total threads:\n", actual_count);
  printf("Non-unique IDs = %u\n", notUnique);
  printf("Reuse maximum  = %zu\n", reuseMax + 1);
  printf("Reuse minimum  = %zu\n", reuseMin + 1);
  printf("Total handles  = %u\n", totalHandles);

  return 0;
}
