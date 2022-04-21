/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER generic handling of reentrant output request and self-invoked set */

#include <string.h>
#include "m_pd.h"
#include "grow.h"



/* Like grow_nodata(), but preserving a 'tail' of *nexisting elements,
   starting from *startp. */
/* LATER rethink handling of a start pointer (clumsy now) */
void *grow_withtail(int *nrequested, int *nexisting, char **startp,
		    int *sizep, void *bufp,
		    int inisize, void *bufini, size_t typesize)
{
    int newsize = *sizep * 2;
    while (newsize < *nrequested) newsize *= 2;
    if (bufp == bufini)
    {
	char *oldstart = *startp;
	if (!(bufp = getbytes(newsize * typesize)))
	{
	    *nrequested = *sizep = inisize;
	    return (bufini);
	}
	*startp = (char *)bufp + (newsize - *nexisting) * typesize;
	*sizep = newsize;
	memcpy(*startp, oldstart, *nexisting * typesize);
    }
    else
    {
	int oldsize = *sizep;
	if (!(bufp = resizebytes(bufp, *sizep * typesize, newsize * typesize)))
	{
	    *startp = (char *)bufini + inisize * typesize;
	    *nrequested = *sizep = inisize;
	    *nexisting = 0;
	    return (bufini);
	}
	*startp = (char *)bufp + (newsize - *nexisting) * typesize;
	*sizep = newsize;
	memmove(*startp, (char *)bufp + (oldsize - *nexisting) * typesize,
		*nexisting * typesize);
    }
    return (bufp);
}
