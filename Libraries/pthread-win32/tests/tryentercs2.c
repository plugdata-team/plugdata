/*
 * tryentercs.c
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
 * See if we have the TryEnterCriticalSection function.
 * Does not use any part of pthreads.
 */

#include <windows.h>
#include <process.h>
#include <stdio.h>


/* [i_a] */
#ifndef WINBASEAPI
#define WINBASEAPI
#endif
#ifndef WINAPI
#define WINAPI
#endif
typedef WINBASEAPI BOOL WINAPI TryEnterCriticalSection_f(LPCRITICAL_SECTION lpCriticalSection);



/*
 * Function pointer to TryEnterCriticalSection if it exists
 * - otherwise NULL
 */
static TryEnterCriticalSection_f *_try_enter_critical_section = NULL;

/*
 * Handle to kernel32.dll
 */
static HINSTANCE _h_kernel32;


#ifndef MONOLITHIC_PTHREAD_TESTS
int
main()
#else 
int
test_tryentercs2(void)
#endif
{
  LPCRITICAL_SECTION lpcs = NULL;

  SetLastError(0);

  printf("Last Error [main enter] %ld\n", (long) GetLastError());

  /*
   * Load KERNEL32 and try to get address of TryEnterCriticalSection
   */
  _h_kernel32 = LoadLibrary(TEXT("KERNEL32.DLL"));
  _try_enter_critical_section =
        (TryEnterCriticalSection_f *)
        GetProcAddress(_h_kernel32,
                         (LPCSTR) "TryEnterCriticalSection");

  if (_try_enter_critical_section != NULL)
    {
      SetLastError(0);

#if defined(_MSC_VER)
  __try
#else /* if defined(__cplusplus) */
  try  /* this generally does not catch access violations, etc. */
#endif
    {
		(*_try_enter_critical_section)(lpcs);
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
		  printf("Expected an access violation and we got one!\n");
		if (GetLastError() == 0)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
		}
	  }

      printf("Last Error [try enter] %ld\n", (long) GetLastError());
    }

  (void) FreeLibrary(_h_kernel32);

  printf("This system %s TryEnterCriticalSection.\n",
         (_try_enter_critical_section == NULL) ? "DOES NOT SUPPORT" : "SUPPORTS");
  printf("POSIX Mutexes will be based on Win32 %s.\n",
         (_try_enter_critical_section == NULL) ? "Mutexes" : "Critical Sections");

  return(0);
}

