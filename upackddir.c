/*  upackddir - extracts files from PackdDir archives

 *  Copyright (C) 1997-2001 Id Software, Inc.
 *  Copyright (C) 2002 The Quakeforge Project.
 *  Copyright (C) 2003 Fabio Bonelli <fabiobonelli@libero.it>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  See the GNU General Public License for more details
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*  $Id: upackddir.c,v 1.4 2003/06/18 22:38:25 fabiob Exp $ */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Needed for getopt_long() */
#define __GNU_SOURCE
#include <getopt.h>

#define PACKDDIR_VERSION "0.0.1"

#define MAX_QPATH 64
#define MAX_OSPATH 128
#define MAX_FILENAMELEN 128
#define MAX_FILES_IN_PACK 4096
#define IDPAKHEADER (('K'<<24) + ('C'<<16) + ('A'<<8) + 'P')

typedef struct {
	int ident;	/* IDPAKHEADER */
	int dirofs;
	int dirlen;
} dpackheader_t;

typedef struct {
	char name[MAX_QPATH];
	int filepos, filelen;
} packfile_t;

typedef struct pack_s {
	char filename[MAX_OSPATH];
	FILE *handle;
	int numfiles;
	packfile_t *files;
} pack_t;

typedef struct {
	char name[56];
	int filepos, filelen;
} dpackfile_t;

int makepath(const char *path);

int _makepath(const char *path)
{
	int len;
	char *p;
	char dir[MAX_FILENAMELEN];

	/* Skip initial slashes */
	while (*path == '/')
		path++;

	p = strchr(path, '/');

	/* Done, path is now a filename */
	if (!p) return 1;

	len = p - path;
	if (len >= MAX_FILENAMELEN) {
		fprintf(stderr, "Directory name too long\n");
		return 0;
	}

	strncpy(dir, path, len);
	dir[len] = '\0';

	if (mkdir(dir, 0777) == -1) {
		if (errno == EEXIST) goto cont;

		fprintf(stderr, "%s: ", dir);
		perror("mkdir");
		return 0;
	}
cont:
	chdir(dir);
	return makepath(p + 1);
}

int makepath(const char *path)
{
	char savedcwd[MAX_FILENAMELEN];
	int ret;

	getcwd(savedcwd, MAX_FILENAMELEN);
	ret = _makepath(path);
	chdir(savedcwd);

	return ret;
}

int extract_file(FILE *f, dpackfile_t *info)
{
	FILE *out;
	char buf[BUFSIZ];
	int rest = 0, count = BUFSIZ;

	if (!makepath(info->name))
		return 0;

	out = fopen(info->name, "w");
	if (!out) {
		perror("Can't open file for write");
		return 0;
	}

	fseek(f, info->filepos, SEEK_SET);

	rest = info->filelen;
	if (rest < BUFSIZ) count = rest;

	while (fread(buf, count, 1, f)) {
		if (!fwrite(buf, count, 1, out))
			break;

		rest -= count;
		if (rest < BUFSIZ) count = rest;
	}

	if (rest) {
		perror("Error");
		return 0;
	}

	fclose(out);

	return 1;
}

/* Extracts a PackdDir file to current directory.
 * @packfile: the filename
 * @mode: extraction mode,
 *	  0 just print filenames
 *	  1 actually extract files
 *
 * returns: pack_t on success,
 *	    NULL on failure */
pack_t *extract_pack(char *packfile, int mode)
{
	dpackheader_t header;
	int i;
	packfile_t *newfiles;
	int numpackfiles;
	pack_t *pack;
	FILE *packhandle, *ph;
	dpackfile_t info[MAX_FILES_IN_PACK];

	packhandle = fopen(packfile, "rb");
	if (!packhandle)
		return NULL;

	fread(&header, 1, sizeof (header), packhandle);

	if (header.ident != IDPAKHEADER)
		fprintf(stderr, "%s is not a packfile", packfile);

	header.dirofs = header.dirofs;
	header.dirlen = header.dirlen;

	numpackfiles = header.dirlen / sizeof (dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		fprintf(stderr, "%s has %i files, max is %i\n",
				packfile, numpackfiles, MAX_FILES_IN_PACK);

	newfiles = malloc(numpackfiles * sizeof (packfile_t));

	fseek(packhandle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, packhandle);

	ph = fopen (packfile, "rb");

	/* Parse the directory */
	for (i = 0; i < numpackfiles; i++) {

		fprintf(stderr, "%s\n", info[i].name);
		if (mode) extract_file(ph, &info[i]);

		strcpy(newfiles[i].name, info[i].name);
		newfiles[i].filepos = info[i].filepos;
		newfiles[i].filelen = info[i].filelen;

	}

	pack = malloc(sizeof (pack_t));
	strcpy(pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	printf ("Packfile %s (%i files)\n", packfile, numpackfiles);

	return pack;
}

static void help_display()
{
	printf("Usage: upackddir FILENAME\n"
	       "Extracts PackdDir archives.\n\n"
	       "Options:\n"
	       "  --help\t\tThis help\n"
	       "  --version\t\tDisplay version information");
}

static void version_display()
{
	printf("upackddir v." PACKDDIR_VERSION "\n"
	       "Copyright (C) 1997-2001 Id Software, Inc.\n"
               "Copyright (C) 2002 The Quakeforge Project.\n"
               "Copyright (C) 2003 Fabio Bonelli <fabiobonelli@libero.it>\n");
}

int
main (int argc, char *argv[])
{
	char c;
	int index;
	struct option long_opts[] = {
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 0 },
		{ NULL, 0, 0, 0}
	};

	if (argc < 2) {
		fprintf (stderr, "Usage: upackddir FILENAME\n"
				 "Try `upackddir --help' for "
				 "more informations.");
		return EXIT_FAILURE;
	}

	c = getopt_long(argc, argv, "h", long_opts, &index);
	switch (c) {
		case 'h':
			help_display();
			break;
		case 0:
			version_display();
			break;
		default:
			fprintf(stderr, "W: getopt_long returned an impossible"
					"value.\n");
	}

	extract_pack(argv[1], 1);

	return EXIT_SUCCESS;
}
/* vim set ai ts=8 */
