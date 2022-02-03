/* Copyright (c) 2002-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __RAND_H__
#define __RAND_H__

#define RAND_DEBUG

void rand_seed(unsigned int *statep, unsigned int seed);
int rand_int(unsigned int *statep, int range);
float rand_unipolar(unsigned int *statep);
float rand_bipolar(unsigned int *statep);

#endif
