#define __USE_XOPEN
#include <unistd.h>
#include "utils.h"
/* $Id: utils.c,v 1.1 2003/07/25 16:46:16 fabiob Exp $ */

/* Endianess
 * TODO: use the autotools */

static int is_little()
{
	int little = 1;

	return (unsigned char) little;
}

unsigned int endian_big_to_host(unsigned int n)
{
	return is_little() ? n<<24|(n&0xff00)<<8|n>>24|(n&0xff0000)>>8 : n;
}

unsigned int endian_little_to_host(unsigned int n)
{
	return is_little() ? n : n<<16|n>>16;
}

unsigned int endian_host_to_little(unsigned int n)
{
	return endian_little_to_host(n);
}
