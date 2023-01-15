/*
 * File: sequence1.c
 *
 *
 * --------------------------------------------------------------------------
 *
 *      pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999-2021 pthreads-win32 / pthreads4w contributors
 *
 *      Contact Email: rpj@callisto.canberra.edu.au
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
 * - that unique thread sequence numbers are generated.
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
 * - analysis output on success.
 *
 * Assumptions:
 * -
 *
 * Pass Criteria:
 * - unique sequence numbers are generated for every new thread.
 *
 * Fail Criteria:
 * -
 */

#include "test.h"
/* Cheating here - sneaking a peek at library internals */
#include "../config.h"
#include "../implement.h"

#include <stdlib.h>
#include <string.h>


/*
 */

#define PTW_ASSERT(expr)								\
	if (ptw_assert((expr), #expr, __FILE__, __LINE__))	\
	{													\
		return 1;	    								\
	}

enum {
	NUMTHREADS = 20000
};


static long done = 0;
/*
 * seqmap should have 1 in every element except [0]
 * Thread sequence numbers start at 1 and we will also
 * include this main thread so we need NUMTHREADS+2
 * elements.
 */
static UINT64 seqmap[NUMTHREADS+2];

static int ptw_assert(int boolean_expr, const char *expr_str, const char *fname, int lineno)
{
	if (!boolean_expr)
	{
		const char *p;

		p = strrchr(fname, '/');
		if (!p)
		{
			p = strrchr(fname, '\\');
		}
		if (!p)
		{
			p = fname;
		}
		else
		{
			p++;
		}
		fprintf(stderr, "[%s @ line %d] ASSERT failed: %s\n", p, lineno, expr_str);
	}
	return !boolean_expr;
}

static int seqmap_set_value = 1;

static void * func(void * arg)
{
  sched_yield();
  if (seqmap_set_value)
  {
		/*
		 * [i_a] here we use the reuse link list internals, so worst case
		 *       we'll have more then NUMTHREADS+2 threads used before 
		 *       we went in here (when this test is part of a 'monolithic' 
		 *       test app), so we apply a modulo to the sequence number
		 *       to prevent out-of-bounds array addressing of seqmap[].
		 */
	seqmap[(int)pthread_getunique_np(pthread_self()) % (NUMTHREADS+2)]++;
  }
  InterlockedIncrement(&done);

  return (void *) 0;
}

static int run_a_sequence(int numthreads)
{
  pthread_t t[NUMTHREADS];
  pthread_attr_t attr;
  int i;

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);

  for (i = 0; i < NUMTHREADS+2; i++)
    {
      seqmap[i] = 0;
    }

  for (i = 0; i < numthreads; i++)
    {
      if (numthreads/2 == i)
        {
			/* Include this main thread, which will be an implicit pthread_t */
			if (seqmap_set_value)
			{
				seqmap[(int)pthread_getunique_np(pthread_self()) % (NUMTHREADS+2)]++;
			}
        }
      PTW_ASSERT(pthread_create(&t[i], &attr, func, NULL) == 0);
    }

  while (numthreads > InterlockedExchangeAdd((LPLONG)&done, 0L))
    Sleep(100);

  Sleep(100);

  PTW_ASSERT(seqmap[0] == 0);
  for (i = 1; i < numthreads+2; i++)
    {
      PTW_ASSERT(seqmap[i] == (UINT64)seqmap_set_value);
    }
  for ( ; i < NUMTHREADS+2; i++)
    {
      PTW_ASSERT(seqmap[i] == 0);
    }

  return 0;
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_sequence2(void)
#endif
{
	printf("phase 1: pollute reuse linked list, i.e. fill it!\n");
	fflush(stdout);
	
	seqmap_set_value = 0;
	assert(0 == run_a_sequence(NUMTHREADS));

	printf("phase 2: notice the reuse linked list will trigger a fault\n");
	printf("         NOTE: the next ASSERT is EXPECTED!\n");
	fflush(stdout);
	
	seqmap_set_value = 1;
	assert(1 == run_a_sequence(NUMTHREADS / 2));

	printf("phase 3: clean the reuse linked list and verify the cleaning\n");
	fflush(stdout);

	pthread_win32_process_detach_np(); // ptw32_processTerminate();
	pthread_win32_process_attach_np(); // ptw32_processInitialize();

	seqmap_set_value = 1;
	assert(0 == run_a_sequence(NUMTHREADS / 2));

	return 0;
}
