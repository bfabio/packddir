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

/*  $Id: upackddir.c,v 1.11 2003/07/24 23:15:41 fabiob Exp $ */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

/* Needed for getopt_long() */
#define __GNU_SOURCE
#include <getopt.h>

#include "lists.h"

#define PACKDDIR_VERSION "0.0.3"

#define MAX_QPATH 64
#define MAX_OSPATH 128
#define MAX_FILENAMELEN 128
#define MAX_FILES_IN_PACK 8192
#define IDPAKHEADER (('K'<<24) + ('C'<<16) + ('A'<<8) + 'P')

#define DEBUG

/* PackdDir file header */
typedef struct {
	int ident;	/* IDPAKHEADER */
	int dirofs;
	int dirlen;
} packheader_t;

/* Packed file */
typedef struct {
	char name[56];
	int filepos, filelen;
} packedfile_t;

/* PackdDir file */
typedef struct pack_s {
	char filename[MAX_OSPATH];
	FILE *handle;
	int numfiles;
	packedfile_t *files;
} pack_t;

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

/* Extracts the file described by the packedfile_t entry
 * from the given stream.
 * @f:    file stream associated with the PackdDir file
 * @info: file entry
 *
 * returns: 1 on success
 *          0 on failure */
int extract_file(FILE *f, packedfile_t *info)
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
 * returns: 1 on success,
 *	    0 on failure */
int extract_pack(char *packfile, int mode)
{
	packheader_t header;
	int i, numpackfiles;
	FILE *packhandle, *ph;
	packedfile_t files[MAX_FILES_IN_PACK];

	packhandle = fopen(packfile, "rb");
	if (!packhandle) {
		fprintf(stderr, "Error extracting `%s': %s\n",
				 packfile, strerror(errno));

		return 0;
	}

	fread(&header, 1, sizeof (header), packhandle);

	if (header.ident != IDPAKHEADER) {
		fprintf(stderr, "%s is not a packfile.\n", packfile);
		return 0;
	}

	numpackfiles = header.dirlen / sizeof (packedfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK) {
		fprintf(stderr, "%s has %i files, max is %i\n",
				packfile, numpackfiles, MAX_FILES_IN_PACK);
		exit(EXIT_FAILURE);
	}

	fseek(packhandle, header.dirofs, SEEK_SET);
	fread(files, 1, header.dirlen, packhandle);

	ph = fopen (packfile, "rb");

	/* Parse the directory */
	for (i = 0; i < numpackfiles; i++) {

		fprintf(stderr, "%s\n", files[i].name);
		if (mode) extract_file(ph, &files[i]);
	}

	printf ("Packfile %s (%i files)\n", packfile, numpackfiles);

	return 1;
}

static int append_file(FILE *f, char *name)
{
	FILE *src;
	int r;
	char buf[BUFSIZ];
	long toread, count = BUFSIZ;
	struct stat sbuf;

	fprintf(stderr, "appending %s\n", name);
	fflush(stderr);

	if (stat(name, &sbuf) == -1)
		return 0;
	toread = sbuf.st_size;

	src = fopen(name, "r");
	if (!src) {
		perror("open()");
		return 0;
	}

	if (toread < BUFSIZ) count = toread;

	while ((r = fread(buf, count, 1, src))) {
		fwrite(buf, count, r, f);
		toread -= count;

		if (toread < BUFSIZ) count = toread;
	}

	fclose(src);
	return 1;
}

list_t packedfiles;

static int traverse_dir(FILE *f, char *name)
{
	DIR *dir;
	int r;
	struct dirent *d;
	struct stat buf;
	char file[56];
	int len;
	packedfile_t *pf;

	len = strlen(name) + 1;
	if (len >= 56) return 0;

	dir = opendir(name);
	if (!dir) {
		perror("opendir()");
		exit(EXIT_FAILURE);
	}

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")
		    || !strcmp(d->d_name, "file.pak"))
			continue;

		if ((len + strlen(d->d_name)) >= 56) {
			fprintf(stderr, "File name too long: `%s/%s'",
					name, d->d_name);
			exit(EXIT_FAILURE);
		}

		file[0] = '\0';
		if (strcmp(name, ".") != 0) {
			strcpy(file, name);
			file[len - 1] = '/';
			file[len] = 0;
		}

		strcat(file, d->d_name);
		r = stat(file, &buf);
		if (r == -1) return 0;

		if (S_ISDIR(buf.st_mode))
			traverse_dir(f, file);
		else {
			pf = malloc(sizeof(packedfile_t));
			strcpy(pf->name, file);
			pf->filepos = ftell(f);
			pf->filelen = buf.st_size;
			list_append(packedfiles, pf);

			append_file(f, file);
		}
	}
	return 1;
}

static int create_pack(char *name, char *files[])
{
	int i = 0;
	int dirlen;
	FILE *out;
	struct stat buf;
	int position;
	list_node_t node;
	packedfile_t *f;

	if (!name) out = stdout;
	else out = fopen(name, "w");

	if (!out) {
		perror("fopen()");
		return 0;
	} else if (out != stdout) {
		stat(name, &buf);
	}

	packedfiles = list_new();
	fseek(out, 12, SEEK_SET);

	while (files[i] != NULL) {
		int r;

		r = stat(files[i], &buf);
		if (r == -1) return 0;

		if (S_ISDIR(buf.st_mode))
			traverse_dir(out, files[i]);
		else {
			f = malloc(sizeof(packedfile_t));
			strcpy(f->name, files[i]);
			f->filepos = ftell(out);
			f->filelen = buf.st_size;
			list_append(packedfiles, f);

			append_file(out, files[i]);
		}
		++i;
	}

	position = ftell(out);
	foreach(node, packedfiles) {
		fwrite(((packedfile_t *) node->element)->name, 56, 1, out);
		fwrite(&((packedfile_t *) node->element)->filepos, 4, 1, out);
		fwrite(&((packedfile_t *) node->element)->filelen, 4, 1, out);
	}

	dirlen = ftell(out) - position;

	fseek(out, 4, SEEK_SET);
	fwrite(&position, 4, 1, out);
	fwrite(&dirlen, 4, 1, out);

	fseek(out, 0, SEEK_SET);
	i = IDPAKHEADER;
	fwrite(&i, 4, 1, out);

	fclose(out);

	return 1;
}

static void help_display()
{
	printf("Usage: upackddir [OPTIONS] FILENAME...\n"
	       "Extracts and creates PackdDir archives.\n\n"
	       "Options:\n"
	       "  -h, --help\t\tThis help.\n"
	       "  -c, --create\t\tCreate new archive.\n"
	       "  -f, --file=ARG\tWrite new archive to ARG."
	                        " Default is stdout.\n"
	       "  -t, --list\t\tList contents.\n"
	       "  --version\t\tDisplay version information.\n"
	       "Report bugs to <fabiobonelli@libero.it>\n");
	exit(EXIT_SUCCESS);
}

static void version_display()
{
	printf("upackddir v." PACKDDIR_VERSION "\n"
	       "Copyright (C) 1997-2001 Id Software, Inc.\n"
               "Copyright (C) 2002 The Quakeforge Project.\n"
               "Copyright (C) 2003 Fabio Bonelli <fabiobonelli@libero.it>\n");
	exit(EXIT_SUCCESS);
}

static void usage_display()
{
	fprintf (stderr, "Usage: upackddir [OPTIONS] FILENAME...\n"
			 "Try `upackddir --help' for "
			 "more informations.\n");
	exit(EXIT_FAILURE);
}

static void error_options_conflict()
{
	fprintf(stderr, "Can't use -ct at the same time.\n");
	usage_display();
}

int
main (int argc, char *argv[])
{
	int ret = 0, extract = 0, create = 0, list = 0;
	char *file = NULL;
	char c;
	int index;

	struct option long_opts[] = {
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 0 },
		{ "file", 1, 0, 'f' },
		{ "create", 0, 0, 'c'},
		{ "list", 0, 0, 't' },
		{ NULL, 0, 0, 0}
	};

	if (argc < 2)
		usage_display();

	while ((c = getopt_long(argc, argv, "htcf:", long_opts, &index)) != -1) {
		switch (c) {
			case 'h':
				help_display();
				break;
			case 'c':
				create = 1;
				break;
			case 0:
				version_display();
				break;
			case 't':
				list = 1;
				break;
			case 'f':
				file = optarg;
				break;
			case '?':
				return EXIT_FAILURE;
			default:
				fprintf(stderr, "W: getopt_long() returned an "
						"impossible value.\n");
				return EXIT_FAILURE;
		}
	}
	if (!create && !list)
		extract = 1;

	if (file && extract) {
		fprintf(stderr, "W: Ignoring -f. Use it only when creating "
				"a new archive.\n");
	}

	if (create && (list || extract))
		error_options_conflict();

	if (argv[optind] != NULL) {
		if (create) {
			ret = create_pack(file, argv + optind);
			if (!ret) perror("create");
		}

		if (list || extract)
			ret = extract_pack(argv[optind], extract);
	} else {
		fprintf(stderr, "Argument expected.\n");
		usage_display();
	}

	return (ret) ? EXIT_SUCCESS : EXIT_FAILURE;
}
/* vim set ai ts=8 */
