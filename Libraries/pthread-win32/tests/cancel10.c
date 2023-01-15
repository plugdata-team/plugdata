/*
 * File: cancel10.c
 *
 * Code derived from https://www.geeksforgeeks.org/pthread_cancel-c-example/ and corrected (see comments in the code)
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
 * Test Synopsis: Test cancelation of other thread.
 *
 * Test Method (Validation or Falsification):
 * -
 *
 * Requirements Tested:
 * - Cancel thread, which doesn't explicitly check for cancelation token.
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
#include <windows.h>

// To Count
static int counter = 0;

// for temporary thread which will be 
// store thread id of second thread
static pthread_t tmp_thread = { 0 };

static int pthread_is_nil(pthread_t t)
{
    return t.p == NULL;
}

// thread_one call func
static void* func(void* p)
{
    for (;;) 
    {
        fprintf(stderr, "thread number one\n");
        Sleep(100);
        counter++;

        // for exiting if counter == 5
        if (counter >= 2 && !pthread_is_nil(tmp_thread))
        {
            // for cancel thread_two
            pthread_cancel(tmp_thread);

            fprintf(stderr, "thread number one: CANCELED thread number two & EXIT\n");

            // for exit from thread_one 
            pthread_exit(NULL);
        }
    }
}

static void PTW32_CDECL cleanup_after_func2(void* data)
{
    int* counter_ptr = (int*)data;

    fprintf(stderr, "cleaning up after canceled threead Number two in round #%d.\n", *counter_ptr);
}

// thread_two call func2
static void* func2(void* p)
{
    int counter2 = 0;

    // set up a cancelation cleanup handler
    pthread_cleanup_push(cleanup_after_func2, &counter2)
    {
        // store thread_two id to tmp_thread
        tmp_thread = pthread_self();

        for (;;)
        {
            counter2++;
            fprintf(stderr, "thread Number two\n");
            Sleep(100);

            // This is a cancelation point:
            // When the thread has been canceled, this call will ABORT
            // the thread by throwing a system exception.
            //
            // Without a cancelation point i.e. canceelation TEST, 
            // this thread will NEVER stop as it will NEVER notice 
            // the cancelation request!
            // (This very important bit is entirely missing from https://www.geeksforgeeks.org/pthread_cancel-c-example/ )
            pthread_testcancel();
        }
    }
    pthread_cleanup_pop(1 /* always call cleanup handler */);

    return NULL; // ignore: warning C4702
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else 
int
test_cancel10(void)
#endif
{
    // declare two thread
    pthread_t thread_one, thread_two;

    // create thread_one
    assert(pthread_create(&thread_one, NULL, func, NULL) == 0);

    // create thread_two 
    assert(pthread_create(&thread_two, NULL, func2, NULL) == 0);

    // waiting for when thread_one is completed
    assert(pthread_join(thread_one, NULL) == 0);

    // waiting for when thread_two is completed
    assert(pthread_join(thread_two, NULL) == 0);

    return 0;
}
