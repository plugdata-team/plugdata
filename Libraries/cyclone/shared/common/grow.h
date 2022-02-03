/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __GROW_H__
#define __GROW_H__

void *grow_nodata(int *nrequested, int *sizep, void *bufp,
		  int inisize, void *bufini, size_t typesize);
void *grow_withdata(int *nrequested, int *nexisting,
		    int *sizep, void *bufp,
		    int inisize, void *bufini, size_t typesize);
void *grow_withtail(int *nrequested, int *nexisting, char **startp,
		    int *sizep, void *bufp,
		    int inisize, void *bufini, size_t typesize);

#endif
