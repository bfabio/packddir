/* $Id: utils.c,v 1.2 2003/09/24 12:50:56 fabiob Exp $ */
#include "utils.h"

/* Endianess
 * TODO: use the autotools */

#define SWAP(x) ((x)<<24|((x)&0xff00)<<8|(x)>>24|((x)&0xff0000)>>8)

static int is_little()
{
	int little = 1;

	return (*(unsigned char *) &little);
}

unsigned endian_big_to_host(unsigned n)
{
	return is_little() ? SWAP(n) : n;
}

unsigned endian_little_to_host(unsigned n)
{
	return is_little() ? n : SWAP(n);
}
