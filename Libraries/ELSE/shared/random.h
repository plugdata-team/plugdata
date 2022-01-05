// random number generator from supercollider
// coded by matt barber

#include <stdint.h>

int rand_int(unsigned int *statep, int range);
void rand_seed(unsigned int *statep, unsigned int seed);

// these are for pinknoise

#if defined(__GNUC__)

/* use gcc's builtins */
static __inline__ int32_t CLZ(int32_t arg)
{
    if (arg)
        return __builtin_clz(arg);
    else
        return 32;
}

#elif defined(_MSC_VER)

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)

__forceinline static int32_t CLZ( int32_t arg )
{
	unsigned long idx;
	if (_BitScanReverse(&idx, (unsigned long)arg))
	{
		return (int32_t)(31-idx);
	}
	return 32;
}

#elif defined(__ppc__) || defined(__powerpc__) || defined(__PPC__)

static __inline__ int32_t CLZ(int32_t arg) {
	__asm__ volatile("cntlzw %0, %1" : "=r" (arg) : "r" (arg));
	return arg;
}

#elif defined(__i386__) || defined(__x86_64__)
static __inline__ int32_t CLZ(int32_t arg) {
	if (arg) {
		__asm__ volatile("bsrl %0, %0\nxorl $31, %0\n"
						 : "=r" (arg) : "0" (arg));
	} else {
		arg = 32;
	}
	return arg;
}

#else

static __inline__ int32_t CLZ(int32_t arg) {
	
	if !(arg) 
		return 32;
	int32_t n = 0;
	if !(arg & 0xFFFF0000) {
		n += 16;
		arg <<= 16;
	}
	if !(arg & 0xFF000000) {
		n += 8;
		arg <<= 8;
	}
	if !(arg & 0xF0000000) {
		n += 4;
		arg <<= 4;
	}
	if !(arg & 0xC0000000) {
		n += 2;
		arg <<= 2;
	}
	if !(arg & 0x80000000)
		n++;
	return n;

}

#endif

typedef struct _random_state
{
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
} t_random_state;


int32_t random_hash(int32_t inKey);

void random_init(t_random_state* rstate, float f);
