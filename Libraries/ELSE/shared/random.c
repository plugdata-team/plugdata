// random number generator from supercollider
// coded by matt barber

#include <m_pd.h>
#include "random.h"


static int instance_number = 0;

int random_get_id(void){
    return(++instance_number);
}

int rand_int(unsigned int *statep, int range)
{
    int result;
    *statep = *statep * 472940017 + 832416023;
    result = ((double)range) * ((double)*statep) * (1./4294967296.);
    return (result < range ? result : range - 1);
}

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



    
uint32_t random_trand(uint32_t *s1, uint32_t *s2, uint32_t *s3){
// Provided for speed in inner loops where the state variables are loaded into registers.
// Thus updating the instance variables can be postponed until the end of the loop.
    *s1 = ((*s1 & (uint32_t)- 2) << 12) ^ (((*s1 << 13) ^ *s1) >> 19);
    *s2 = ((*s2 & (uint32_t)- 8) <<  4) ^ (((*s2 <<  2) ^ *s2) >> 25);
    *s3 = ((*s3 & (uint32_t)-16) << 17) ^ (((*s3 <<  3) ^ *s3) >> 11);
    return(*s1 ^ *s2 ^ *s3);
}

float random_frand(uint32_t *s1, uint32_t *s2, uint32_t *s3){
    // return a float from -1.0 to +0.999...
    union { uint32_t i; float f; } u; // union for floating point conversion of result
    u.i = 0x40000000 | (random_trand(s1, s2, s3) >> 9);
    return(u.f - 3.f);
}

int32_t random_hash(int32_t inKey){
    // Thomas Wang's integer hash (a faster hash for integers, also very good).
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    uint32_t hash = (uint32_t)inKey;
    hash += ~(hash << 15);
    hash ^=   hash >> 10;
    hash +=   hash << 3;
    hash ^=   hash >> 6;
    hash += ~(hash << 11);
    hash ^=   hash >> 16;
    return((int32_t)hash);
}

void random_init(t_random_state* rstate, int seed){
	// humans tend to use small seeds - mess up the bits
	uint32_t seedval = (uint32_t)random_hash(seed);
	uint32_t *s1 = &rstate->s1;
	uint32_t *s2 = &rstate->s2;
	uint32_t *s3 = &rstate->s3;
	// initialize seeds using the given seed value taking care of
	// the requirements. The constants below are arbitrary otherwise
	*s1 = 1243598713U ^ seedval;
    if(*s1 <  2)
        *s1 = 1243598713U;
	*s2 = 3093459404U ^ seedval;
    if(*s2 <  8)
        *s2 = 3093459404U;
	*s3 = 1821928721U ^ seedval;
    if(*s3 < 16)
        *s3 = 1821928721U;
}

int get_seed(t_symbol *s, int ac, t_atom *av, int n){
    s = NULL;
    return(ac ? atom_getint(av) : (int)(time(NULL)*n));
}
