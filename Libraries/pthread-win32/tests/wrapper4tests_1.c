/*
   Ger Hobbelt ([i_a]) : wrapper code for multiple pthreads library tests; for use with MSVC2003/5 project.
 */

#include "../pthread.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>


#ifndef MONOLITHIC_PTHREAD_TESTS

int main(int argc, char **argv)
{
	return 0;
}

#else

int test_affinity1(void);
int test_affinity2(void);
int test_affinity3(void);
int test_affinity4(void);
int test_affinity5(void);
int test_affinity6(void);
int test_barrier1(void);
int test_barrier2(void);
int test_barrier3(void);
int test_barrier4(void);
int test_barrier5(void);
int test_barrier6(void);
int test_benchtest1(void);
int test_benchtest2(void);
int test_benchtest3(void);
int test_benchtest4(void);
int test_benchtest5(void);
int test_cancel1(void);
int test_cancel2(void);
int test_cancel3(void);
int test_cancel4(void);
int test_cancel5(void);
int test_cancel6a(void);
int test_cancel6d(void);
int test_cancel7(void);
int test_cancel8(void);
int test_cancel9(void);
int test_cancel10(void);
int test_cleanup0(void);
int test_cleanup1(void);
int test_cleanup2(void);
int test_cleanup3(void);
int test_condvar1(void);
int test_condvar1_1(void);
int test_condvar1_2(void);
int test_condvar2(void);
int test_condvar2_1(void);
int test_condvar3(void);
int test_condvar3_1(void);
int test_condvar3_2(void);
int test_condvar3_3(void);
int test_condvar4(void);
int test_condvar5(void);
int test_condvar6(void);
int test_condvar7(void);
int test_condvar8(void);
int test_condvar9(void);
int test_context1(void);
int test_context2(void);
int test_count1(void);
int test_create1(void);
int test_create2(void);
int test_create3(void);
int test_create3a(int argc, char **argv);
int test_delay1(void);
int test_delay2(void);
int test_detach1(void);
int test_equal0(void);
int test_equal1(void);
int test_errno0(void);
int test_errno1(void);
int test_exception1(void);
int test_exception2(int argc, char **argv);
int test_exception3(void);
int test_exception3_0(void);
int test_exit1(void);
int test_exit2(void);
int test_exit3(void);
int test_exit4(void);
int test_exit5(void);
int test_exit6(void);
int test_eyal1(void);
int test_inherit1(void);
int test_join0(void);
int test_join1(void);
int test_join2(void);
int test_join3(void);
int test_join4(void);
int test_kill1(void);
int test_kill2(void);
int test_loadfree(void);
int test_mutex1(void);
int test_mutex1e(void);
int test_mutex1n(void);
int test_mutex1r(void);
int test_mutex2(void);
int test_mutex2e(void);
int test_mutex2r(void);
int test_mutex3(void);
int test_mutex3e(void);
int test_mutex3r(void);
int test_mutex4(void);
int test_mutex5(void);
int test_mutex6(void);
int test_mutex6e(void);
int test_mutex6es(void);
int test_mutex6n(void);
int test_mutex6r(void);
int test_mutex6rs(void);
int test_mutex6s(void);
int test_mutex7(void);
int test_mutex7e(void);
int test_mutex7n(void);
int test_mutex7r(void);
int test_mutex8(void);
int test_mutex8e(void);
int test_mutex8n(void);
int test_mutex8r(void);
int test_namenp1(void);
int test_namenp2(void);
int test_once1(void);
int test_once2(void);
int test_once3(void);
int test_once4(void);
int test_openmp1(int argc, char *argv[]);
int test_priority1(void);
int test_priority2(void);
int test_reinit1(void);
int test_reuse1(void);
int test_reuse2(void);
int test_robust1(void);
int test_robust2(void);
int test_robust3(void);
int test_robust4(void);
int test_robust5(void);
int test_rwlock1(void);
int test_rwlock2(void);
int test_rwlock2_t(void);
int test_rwlock3(void);
int test_rwlock3_t(void);
int test_rwlock4(void);
int test_rwlock4_t(void);
int test_rwlock5(void);
int test_rwlock5_t(void);
int test_rwlock6(void);
int test_rwlock6_t(void);
int test_rwlock6_t2(void);
int test_rwlock7(void);
int test_rwlock8(void);
int test_self1(void);
int test_self2(void);
int test_semaphore1(void);
int test_semaphore2(void);
int test_semaphore3(void);
int test_semaphore4(void);
int test_semaphore4t(void);
int test_semaphore5(void);
int test_sequence1(void);
int test_sequence2(void);
int test_sizes(void);
int test_spin1(void);
int test_spin2(void);
int test_spin3(void);
int test_spin4(void);
int test_stress1(void);
int test_threestage(int argc, char* argv[]);
int test_timeouts(void);
int test_tryentercs(void);
int test_tryentercs2(void);
int test_tsd1(void);
int test_tsd2(void);
int test_tsd3(void);
int test_valid1(void);
int test_valid2(void);



typedef int f_f(void);
typedef int fwa_f(int argc, char **argv);

#define TEST_WRAPPER(f)				\
	ret = Test_Wrapper(&f, #f);		\
	if (ret)						\
	{								\
		return EXIT_FAILURE;		\
	}

#define TEST_WRAPPER_W_ARGV(f)					    \
	ret = Test_Wrapper_w_argv(&f, #f, argc, argv);	\
	if (ret)										\
	{												\
		return EXIT_FAILURE;						\
	}

static int Test_Wrapper(f_f * fp, const char *f_n)
{
	int ret;

	printf("\n=============================================================================\n"
		"   Test: %s\n",
		f_n);
	fflush(stdout);
	ret = (*fp)();
	if (ret != 0)
	{
		printf("TEST %s FAILED\n", f_n);
	}
	Sleep(10);
	pthread_win32_process_detach_np(); // ptw32_processTerminate();
	pthread_win32_process_attach_np(); // ptw32_processInitialize();
	return ret;
}


static int Test_Wrapper_w_argv(fwa_f * fp, const char *f_n, int argc, char **argv)
{
	int ret;

	printf("\n=============================================================================\n"
		"   Test: %s\n",
		f_n);
	fflush(stdout);
	ret = (*fp)(argc, argv);
	if (ret != 0)
	{
		printf("TEST %s FAILED\n", f_n);
	}
	Sleep(10);
	pthread_win32_process_detach_np(); // ptw32_processTerminate();
	pthread_win32_process_attach_np(); // ptw32_processInitialize();
	return ret;
}


void exit_handler(void)
{
	printf("\nDone.\n");
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int ret;

	atexit(&exit_handler);

	/*
	  fails when run down below; does not fail when run at start of run - turns out to be
	  due to the thread reuse logic kicking in and the test code not anticipating that
	  -->
	  introduction of test sequence2.c above to showcase the fix for this, including
	  augmentation of pthread_win32_process_attach_np() to prevent crashes.
	*/
	TEST_WRAPPER(test_loadfree);
	TEST_WRAPPER(test_cancel8);
	TEST_WRAPPER(test_sequence2);

	TEST_WRAPPER(test_reuse1);

	TEST_WRAPPER(test_sequence1); /* fails when run down below; does not fail when run at start of run - same issue as test reuse1 */

	TEST_WRAPPER(test_cancel7);
	TEST_WRAPPER(test_cleanup1);
	TEST_WRAPPER(test_condvar7);
	TEST_WRAPPER(test_condvar9);
	TEST_WRAPPER(test_exception1);
	TEST_WRAPPER(test_sequence1);

	TEST_WRAPPER(test_affinity1);
	TEST_WRAPPER(test_affinity2);
	TEST_WRAPPER(test_affinity3);
	TEST_WRAPPER(test_affinity4);
	TEST_WRAPPER(test_affinity5);
	TEST_WRAPPER(test_affinity6);
	TEST_WRAPPER(test_barrier1);
	TEST_WRAPPER(test_barrier2);
	TEST_WRAPPER(test_barrier3);
	TEST_WRAPPER(test_barrier4);
	TEST_WRAPPER(test_barrier5);
	TEST_WRAPPER(test_barrier6);
	TEST_WRAPPER(test_cancel1);
	TEST_WRAPPER(test_cancel2);
	TEST_WRAPPER(test_cancel3);
	TEST_WRAPPER(test_cancel4);
	TEST_WRAPPER(test_cancel5);
	TEST_WRAPPER(test_cancel6a);
	TEST_WRAPPER(test_cancel6d);
//	TEST_WRAPPER(test_cancel7);
//	TEST_WRAPPER(test_cancel8);
	TEST_WRAPPER(test_cancel9);
	TEST_WRAPPER(test_cancel10);
	TEST_WRAPPER(test_cleanup0);
//	TEST_WRAPPER(test_cleanup1);
	TEST_WRAPPER(test_cleanup2);
	TEST_WRAPPER(test_cleanup3);
	TEST_WRAPPER(test_condvar1);
	TEST_WRAPPER(test_condvar1_1);
	TEST_WRAPPER(test_condvar1_2);
	TEST_WRAPPER(test_condvar2);
	TEST_WRAPPER(test_condvar2_1);
	TEST_WRAPPER(test_condvar3);
	TEST_WRAPPER(test_condvar3_1);
	TEST_WRAPPER(test_condvar3_2);
	TEST_WRAPPER(test_condvar3_3);
	TEST_WRAPPER(test_condvar4);
	TEST_WRAPPER(test_condvar5);
	TEST_WRAPPER(test_condvar6);
//	TEST_WRAPPER(test_condvar7);
	TEST_WRAPPER(test_condvar8);
//	TEST_WRAPPER(test_condvar9);
	TEST_WRAPPER(test_context1);
	TEST_WRAPPER(test_context2);
	TEST_WRAPPER(test_count1);
	TEST_WRAPPER(test_create1);
	TEST_WRAPPER(test_create2);
	TEST_WRAPPER(test_create3);
	TEST_WRAPPER_W_ARGV(test_create3a);
	TEST_WRAPPER(test_delay1);
	TEST_WRAPPER(test_delay2);
	TEST_WRAPPER(test_detach1);
	TEST_WRAPPER(test_equal0);
	TEST_WRAPPER(test_equal1);
	TEST_WRAPPER(test_errno0);
	TEST_WRAPPER(test_errno1);
//	TEST_WRAPPER(test_exception1);
	TEST_WRAPPER_W_ARGV(test_exception2);
	TEST_WRAPPER(test_exception3);
	TEST_WRAPPER(test_exception3_0);
	//TEST_WRAPPER(test_exit1);
	TEST_WRAPPER(test_exit2);
	TEST_WRAPPER(test_exit3);
	TEST_WRAPPER(test_exit4);
	TEST_WRAPPER(test_exit5);
	TEST_WRAPPER(test_exit6);
	TEST_WRAPPER(test_eyal1);
	TEST_WRAPPER(test_inherit1);
	TEST_WRAPPER(test_join0);
	TEST_WRAPPER(test_join1);
	TEST_WRAPPER(test_join2);
	TEST_WRAPPER(test_join3);
	TEST_WRAPPER(test_join4);
	TEST_WRAPPER(test_kill1);
	TEST_WRAPPER(test_kill2);
	//	TEST_WRAPPER(test_loadfree);
	TEST_WRAPPER(test_reinit1);
	TEST_WRAPPER(test_namenp1);
	TEST_WRAPPER(test_namenp2);
	TEST_WRAPPER(test_mutex1);
	TEST_WRAPPER(test_mutex1e);
	TEST_WRAPPER(test_mutex1n);
	TEST_WRAPPER(test_mutex1r);
	TEST_WRAPPER(test_mutex2);
	TEST_WRAPPER(test_mutex2e);
	TEST_WRAPPER(test_mutex2r);
	TEST_WRAPPER(test_mutex3);
	TEST_WRAPPER(test_mutex3e);
	TEST_WRAPPER(test_mutex3r);
	TEST_WRAPPER(test_mutex4);
	TEST_WRAPPER(test_mutex5);
	TEST_WRAPPER(test_mutex6);
	TEST_WRAPPER(test_mutex6e);
	TEST_WRAPPER(test_mutex6es);
	TEST_WRAPPER(test_mutex6n);
	TEST_WRAPPER(test_mutex6r);
	TEST_WRAPPER(test_mutex6rs);
	TEST_WRAPPER(test_mutex6s);
	TEST_WRAPPER(test_mutex7);
	TEST_WRAPPER(test_mutex7e);
	TEST_WRAPPER(test_mutex7n);
	TEST_WRAPPER(test_mutex7r);
	TEST_WRAPPER(test_mutex8);
	TEST_WRAPPER(test_mutex8e);
	TEST_WRAPPER(test_mutex8n);
	TEST_WRAPPER(test_mutex8r);
	TEST_WRAPPER(test_once1);
	TEST_WRAPPER(test_once2);
	TEST_WRAPPER(test_once3);
	TEST_WRAPPER(test_once4);
	TEST_WRAPPER_W_ARGV(test_openmp1);
	TEST_WRAPPER(test_priority1);
	TEST_WRAPPER(test_priority2);
	/* TEST_WRAPPER(test_reuse1); -- fails when run here; does not fail when run at start of run :-S */
	TEST_WRAPPER(test_reuse2);
	TEST_WRAPPER(test_robust1);
	TEST_WRAPPER(test_robust2);
	TEST_WRAPPER(test_robust3);
	TEST_WRAPPER(test_robust4);
	TEST_WRAPPER(test_robust5);
	TEST_WRAPPER(test_rwlock1);
	TEST_WRAPPER(test_rwlock2);
	TEST_WRAPPER(test_rwlock2_t);
	TEST_WRAPPER(test_rwlock3);
	TEST_WRAPPER(test_rwlock3_t);
	TEST_WRAPPER(test_rwlock4);
	TEST_WRAPPER(test_rwlock4_t);
	TEST_WRAPPER(test_rwlock5);
	TEST_WRAPPER(test_rwlock5_t);
	TEST_WRAPPER(test_rwlock6);
	TEST_WRAPPER(test_rwlock6_t);
	TEST_WRAPPER(test_rwlock6_t2);
	TEST_WRAPPER(test_rwlock7);
	TEST_WRAPPER(test_rwlock8);
	TEST_WRAPPER(test_self1);
	TEST_WRAPPER(test_self2);
	TEST_WRAPPER(test_semaphore1);
	TEST_WRAPPER(test_semaphore2);
	TEST_WRAPPER(test_semaphore3);
	TEST_WRAPPER(test_semaphore4);
	TEST_WRAPPER(test_semaphore4t);
	TEST_WRAPPER(test_semaphore5);
//	TEST_WRAPPER(test_sequence1);
	TEST_WRAPPER(test_sizes);
	TEST_WRAPPER(test_spin1);
	TEST_WRAPPER(test_spin2);
	TEST_WRAPPER(test_spin3);
	TEST_WRAPPER(test_spin4);
	TEST_WRAPPER(test_stress1);
	TEST_WRAPPER_W_ARGV(test_threestage);
	TEST_WRAPPER(test_timeouts);
	TEST_WRAPPER(test_tryentercs);
	TEST_WRAPPER(test_tryentercs2);
	TEST_WRAPPER(test_tsd1);
	TEST_WRAPPER(test_tsd2);
	TEST_WRAPPER(test_tsd3);
	TEST_WRAPPER(test_valid1);
	TEST_WRAPPER(test_valid2);

	TEST_WRAPPER(test_benchtest1);
	TEST_WRAPPER(test_benchtest2);
	TEST_WRAPPER(test_benchtest3);
	TEST_WRAPPER(test_benchtest4);
	TEST_WRAPPER(test_benchtest5);

	/* test_exit1 should be the VERY LAST test of the bunch as it will exit the application before it returns! */
	TEST_WRAPPER(test_exit1);

	printf("Should never get here!\n");
	return EXIT_FAILURE;
}

#endif

