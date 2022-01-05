// random number generator from supercollider
// coded by matt barber

#include <m_pd.h>
#include <stdint.h>

#include <time.h>
#include "m_pd.h"
EXTERN double sys_getrealtime(void);  /* used to be in m_imp.h */
#include "random.h"

#define PINK_ARRAY_SIZE 32768

/* borrowed from x_misc.c, LATER rethink */
void rand_seed(unsigned int *statep, unsigned int seed)
{
    if (seed) *statep = (seed & 0x7fffffff);
    else
    {
        /* LATER consider using time elapsed from system startup,
         (or login -- in linux we might call getutent) */
        static unsigned int failsafe = 1489853723;
        static int shift = 0;
        static unsigned int lastticks = 0;
        /* LATER rethink -- it might fail on faster machine than mine
         (but does it matter?) */
        unsigned int newticks = (unsigned int)(sys_getrealtime() * 1000000.);
        if (newticks == lastticks)
        {
            failsafe = failsafe * 435898247 + 938284287;
            *statep = (failsafe & 0x7fffffff);
#ifdef RAND_DEBUG
            post("rand_seed failed (newticks %d)", newticks);
#endif
        }
        else
        {
            if (!shift)
                shift = (int)time(0);  /* LATER deal with error return (-1) */
            *statep = ((newticks + shift) & 0x7fffffff);
#if 0
            post("rand_seed: newticks %d, shift %d", newticks, shift);
#endif
        }
        lastticks = newticks;
    }
}

/* borrowed from x_misc.c, LATER rethink */
int rand_int(unsigned int *statep, int range)
{
    int result;
    *statep = *statep * 472940017 + 832416023;
    result = ((double)range) * ((double)*statep) * (1./4294967296.);
    return (result < range ? result : range - 1);
}

int32_t random_hash(int32_t inKey)
{
    // Thomas Wang's integer hash.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    // a faster hash for integers. also very good.
    uint32_t hash = (uint32_t)inKey;
    hash += ~(hash << 15);
    hash ^=   hash >> 10;
    hash +=   hash << 3;
    hash ^=   hash >> 6;
    hash += ~(hash << 11);
    hash ^=   hash >> 16;
    return (int32_t)hash;
}


void random_init(t_random_state* rstate, float f)
{
	// humans tend to use small seeds - mess up the bits
	uint32_t seed = (uint32_t)random_hash((int)f);
	uint32_t *s1 = &rstate->s1;
	uint32_t *s2 = &rstate->s2;
	uint32_t *s3 = &rstate->s3;
	// initialize seeds using the given seed value taking care of
	// the requirements. The constants below are arbitrary otherwise
	*s1 = 1243598713U ^ seed; if (*s1 <  2) *s1 = 1243598713U;
	*s2 = 3093459404U ^ seed; if (*s2 <  8) *s2 = 3093459404U;
	*s3 = 1821928721U ^ seed; if (*s3 < 16) *s3 = 1821928721U;
	
}




