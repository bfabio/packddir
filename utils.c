/* $Id: utils.c,v 1.3 2003/12/02 20:57:23 fabiob Exp $ */
#include "utils.h"

/* Endianess
 * TODO: use the autotools */

#define SWAP(x) ((x)<<24|((x)&0xff00)<<8|(x)>>24|((x)&0xff0000)>>8)

static int is_little()
{
	int little = 1;

	return (*(unsigned char *) &little);
}

unsigned inline endian_big_to_host(unsigned n)
{
	return is_little() ? SWAP(n) : n;
}

unsigned inline endian_little_to_host(unsigned n)
{
	return is_little() ? n : SWAP(n);
}
