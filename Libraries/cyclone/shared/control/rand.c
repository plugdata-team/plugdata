/* Copyright (c) 1997-2004 Miller Puckette, krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <time.h>
#include "m_pd.h"
EXTERN double sys_getrealtime(void);  /* used to be in m_imp.h */
#include "rand.h"

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

float rand_unipolar(unsigned int *statep)
{
    float result;
    *statep = *statep * 472940017 + 832416023;
    result = (float)((double)*statep * (1./4294967296.));
    return (result);
}

/* borrowed from d_osc.c, LATER rethink */
float rand_bipolar(unsigned int *statep)
{
    float result = ((float)(((int)*statep & 0x7fffffff) - 0x40000000))
	* (float)(1.0 / 0x40000000);
    *statep = (unsigned)((int)*statep * 435898247 + 382842987);
    return (result);
}
