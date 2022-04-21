/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER generic handling of reentrant output request and self-invoked set */

#include <string.h>
#include "m_pd.h"
#include "grow.h"

/* Prior to this call a caller is supposed to check for *nrequested > *sizep.
   Returns a reallocated buffer's pointer (success) or a given 'bufini'
   default value (failure).
   Upon return *nrequested contains the actual number of elements:
   requested (success) or a given default value of 'inisize' (failure). */
void *grow_nodata(int *nrequested, int *sizep, void *bufp,
		  int inisize, void *bufini, size_t typesize)
{
    int newsize = *sizep * 2;
    while (newsize < *nrequested) newsize *= 2;
    if (bufp == bufini)
	bufp = getbytes(newsize * typesize);
    else
	bufp = resizebytes(bufp, *sizep * typesize, newsize * typesize);
    if (bufp)
    {
	*sizep = newsize;
	return (bufp);
    }
    else
    {
	*nrequested = *sizep = inisize;
	return (bufini);
    }
}

/* Like grow_nodata(), but preserving first *nexisting elements. */
void *grow_withdata(int *nrequested, int *nexisting,
		    int *sizep, void *bufp,
		    int inisize, void *bufini, size_t typesize)
{
    int newsize = *sizep * 2;
    while (newsize < *nrequested) newsize *= 2;
    if (bufp == bufini)
    {
	if (!(bufp = getbytes(newsize * typesize)))
	{
	    *nrequested = *sizep = inisize;
	    return (bufini);
	}
	*sizep = newsize;
	memcpy(bufp, bufini, *nexisting * typesize);
    }
    else
    {
//	int oldsize = *sizep;
	if (!(bufp = resizebytes(bufp, *sizep * typesize, newsize * typesize)))
	{
	    *nrequested = *sizep = inisize;
	    *nexisting = 0;
	    return (bufini);
	}
	*sizep = newsize;
    }
    return (bufp);
}

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
