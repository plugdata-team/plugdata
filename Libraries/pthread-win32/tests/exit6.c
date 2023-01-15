/*
 *
 * File: exit6.c
 *
 *  Created on: 14/05/2013
 *      Author: ross
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
 */

#include "test.h"
#ifndef _UWIN
#include <process.h>
#endif

#include <pthread.h>
//#include <stdlib.h>

static pthread_key_t key;
static int where;

static unsigned __stdcall
start_routine(void * arg)
{
  int *val = (int *) malloc(sizeof(int));

  where = 2;
  //printf("start_routine: native thread\n");

  *val = 48;
  pthread_setspecific(key, val);
  return 0;
}

static void
key_dtor(void *arg)
{
  //printf("key_dtor: %d\n", *(int*)arg);
  if (where == 2)
    printf("Library has thread exit POSIX cleanup for native threads.\n");
  else
    printf("Library has process exit POSIX cleanup for native threads.\n");
  free(arg);
}

#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else 
int
test_exit6(void)
#endif
{
  HANDLE hthread;

  where = 1;
  pthread_key_create(&key, key_dtor);
  hthread = (HANDLE)_beginthreadex(NULL, 0, start_routine, NULL, 0, NULL);
  WaitForSingleObject(hthread, INFINITE);
  CloseHandle(hthread);
  where = 3;
  pthread_key_delete(key);

  //printf("main: exiting\n");
  return 0;
}
