/*  upackddir - extracts files from PackdDir archives
 *  Copyright (C) 2003-2004  Fabio Bonelli <fabiobonelli@libero.it>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/*  $Id: log.h,v 1.4 2004/01/13 16:50:23 fabiob Exp $ */

#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>

#define LOG_DEBUG	0
#define LOG_VERBOSE	1
#define LOG_NORMAL	2
#define LOG_QUIET	3

#define DEFAULT_LOGLEVEL LOG_NORMAL

#define LOG(x, ...) \
	do { /* if ((l) >= DEFAULT_LOGLEVEL) */ \
		fprintf(stderr, "upackddir: " x); \
	} \
	while (0)

#define LOGF(x, ...) \
	do { /* if ((l) >= DEFAULT_LOGLEVEL) */ \
		fprintf(stderr, "upackddir: " x , __VA_ARGS__); \
	} \
	while (0)

#endif /* !_LOG_H */
