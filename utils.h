/* $Id: utils.h,v 1.2 2003/09/24 12:50:57 fabiob Exp $ */
#ifndef _UTILS_H
#define _UTILS_H

#define endian_host_to_big(x)		endian_big_to_host(x)
#define endian_host_to_little(x)	endian_little_to_host(x)
unsigned int endian_big_to_host(unsigned int);
unsigned int endian_little_to_host(unsigned int);

#endif /* !_UTILS_H */
