// random number generator from supercollider
// coded by matt barber

#include <m_pd.h>
#include "random.h"

static int instance_number = 0;


static int makeseed(void){
    static PERTHREAD unsigned int seed = 0;
    if(!seed)
        seed = time(NULL);
    seed = seed * 435898247 + 938284287;
    return(seed & 0x7fffffff);
}


static int32_t random_hash(int32_t inKey){
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


unsigned int cyclone_random_get_seed(t_symbol *s, int ac, t_atom *av, int n){
    s = NULL;
    unsigned int timeval;
    if(ac && av->a_type == A_FLOAT)
        timeval = (unsigned int)(atom_getfloat(av));
    else
        timeval = (unsigned int)(time(NULL)*151*n);
    return(timeval);
}

int cyclone_random_get_id(void){
    return(++instance_number);
}

uint32_t cyclone_random_trand(uint32_t *s1, uint32_t *s2, uint32_t *s3){
// Provided for speed in inner loops where the state variables are loaded into registers.
// Thus updating the instance variables can be postponed until the end of the loop.
    *s1 = ((*s1 & (uint32_t)- 2) << 12) ^ (((*s1 << 13) ^ *s1) >> 19);
    *s2 = ((*s2 & (uint32_t)- 8) <<  4) ^ (((*s2 <<  2) ^ *s2) >> 25);
    *s3 = ((*s3 & (uint32_t)-16) << 17) ^ (((*s3 <<  3) ^ *s3) >> 11);
    return(*s1 ^ *s2 ^ *s3);
}

float cyclone_random_frand(uint32_t *s1, uint32_t *s2, uint32_t *s3){
    // return a float from -1.0 to +0.999...
    union { uint32_t i; float f; } u; // union for floating point conversion of result
    u.i = 0x40000000 | (cyclone_random_trand(s1, s2, s3) >> 9);
    return(u.f - 3.f);
}

int cyclone_rand_int(unsigned int *statep, int range){ // from [random]
    *statep = *statep * 472940017 + 832416023;
    int result = ((double)range) * ((double)*statep) * (1./4294967296.);
    return(result < range ? result : range - 1);
}

void cyclone_random_init(t_cyclone_random_state* rstate, float f){
	// humans tend to use small seeds - mess up the bits
	uint32_t seedval = (uint32_t)random_hash((int)f);
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
