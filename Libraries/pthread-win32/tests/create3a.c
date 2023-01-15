/*
 * File: create3a.c
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
 * Test Synopsis: Test passing NULL as thread id arg to pthread_create.
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


#ifdef __GNUC__
#include <stdlib.h>
#endif

#include "test.h"

/*
 * Create NUMTHREADS threads in addition to the Main thread.
 */
enum {
  NUMTHREADS = 1
};


void *
threadFunc(void * arg)
{
  return (void *) 0;
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main(int argc, char **argv)
#else 
int
test_create3a(int argc, char **argv)
#endif
{
  int i;
  pthread_t mt;

  if (argc >= 2 && (!strcmp(argv[1], "-?") || !strcmp(argv[1], "-h")))
    {
      int result;

      printf("You should see an application memory write error message\n");
      fflush(stdout);
      result = system("create3a.exe die");
      exit(result);
    }

  assert((mt = pthread_self()).p != NULL);

  for (i = 0; i < NUMTHREADS; i++)
    {
#if defined(_MSC_VER)
  __try
#else /* if defined(__cplusplus) */
  try  /* this generally does not catch access violations, etc. */
#endif
    {
	      assert(pthread_create(NULL, NULL, threadFunc, NULL) == 0);

		  // Should never get here!
		  return EXIT_FAILURE;
    }
#if defined(_MSC_VER)
  __except(EXCEPTION_EXECUTE_HANDLER)
#else /* if defined(__cplusplus) */
#if defined(PtW32CatchAll)
  PtW32CatchAll
#else
  catch(...)
#endif
#endif
	  {
		  printf("Expected an access violation and we got one for thread %d - it has NOT been created!\n", i);
	  }
    }

  /*
   * Success.
   */
  return EXIT_SUCCESS;
}

