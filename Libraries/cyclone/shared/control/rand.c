/* Copyright (c) 1997-2004 Miller Puckette, krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <time.h>
#include "m_pd.h"
EXTERN double sys_getrealtime(void);  /* used to be in m_imp.h */
#include "rand.h"

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
