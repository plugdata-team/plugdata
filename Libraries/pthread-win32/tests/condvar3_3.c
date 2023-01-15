/*
 * File: condvar3_3.c
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
 * - Test timeouts and lost signals on a CV.
 *
 * Test Method (Validation or Falsification):
 * - Validation
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
 * - pthread_cond_timedwait returns ETIMEDOUT.
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - pthread_cond_timedwait does not return ETIMEDOUT.
 * - Process returns non-zero exit status.
 */

/* Timur Aydin (taydin@snet.net) */

#include "test.h"

#include <sys/timeb.h>

pthread_cond_t cnd;
pthread_mutex_t mtx;

static const long NANOSEC_PER_SEC = 1000000000L;

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else
int
test_condvar3_3(void)
#endif
{
   int rc;
   struct timespec abstime, reltime = { 0, NANOSEC_PER_SEC/2 };

   assert(pthread_cond_init(&cnd, 0) == 0);
   assert(pthread_mutex_init(&mtx, 0) == 0);

   (void) pthread_win32_getabstime_np(&abstime, &reltime);

   /* Here pthread_cond_timedwait should time out after one second. */

   assert(pthread_mutex_lock(&mtx) == 0);

   assert((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);

   assert(pthread_mutex_unlock(&mtx) == 0);

   /* Here, the condition variable is signalled, but there are no
      threads waiting on it. The signal should be lost and
      the next pthread_cond_timedwait should time out too. */

   assert((rc = pthread_cond_signal(&cnd)) == 0);

   assert(pthread_mutex_lock(&mtx) == 0);

   (void) pthread_win32_getabstime_np(&abstime, &reltime);

   assert((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);

   assert(pthread_mutex_unlock(&mtx) == 0);

   return 0;
}
