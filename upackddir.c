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

/*  $Id: upackddir.c,v 1.33 2003/12/02 21:52:17 fabiob Exp $ */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

/* Needed for fileno() */
#define __USE_POSIX
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
#include "log.h"
#include "utils.h"

#define PACKDDIR_VERSION "0.0.5"

#define MAX_OSPATH 128
#define MAX_FILENAMELEN 128
#define IDPAKHEADER (('P'<<24) + ('A'<<16) + ('C'<<8) + 'K')

#define DEBUG

/* PackdDir file header */
typedef struct {
	int ident;	/* IDPAKHEADER */
	int dirofs;	/* Directory offset */
	int dirlen;	/* Directory length */
} packheader_t;

/* Packed file */
typedef struct {
	char name[56];
	int filepos, filelen;
} packedfile_t;

/* PackdDir file */
typedef struct pack_s {
	char filename[MAX_OSPATH];
	packheader_t header;
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
		LOGF("Directory name '%s` too long\n", path);
		return 0;
	}

	strncpy(dir, path, len);
	dir[len] = '\0';

	if (mkdir(dir, 0777) == -1) {
		if (errno == EEXIST) goto cont;

		LOGF("%s: ", dir);
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

pack_t *packfile_open(char *name)
{
	pack_t *pack;

	pack = malloc(sizeof(pack_t));
	if (!pack) return NULL;

	if (strcmp(name, "-") == 0)
		pack->handle = stdin;
	else pack->handle = fopen(name, "rb");

	if (!pack->handle) {
		LOGF("Error extracting `%s': %s\n",
		     name, strerror(errno));

		return NULL;
	}

	fread(&pack->header, 1, sizeof(pack->header), pack->handle);

	if (endian_big_to_host(pack->header.ident) != IDPAKHEADER) {
		LOGF("%s is not a packfile.\n", name);
		return NULL;
	}

	pack->numfiles = endian_little_to_host(pack->header.dirlen) /
			 sizeof(packedfile_t);

	return pack;
}

/* Extracts the file described by the packedfile_t entry
 * from the given packfile.
 * @pack: PackdDir file, returned from packfile_open()
 * @info: file entry
 *
 * returns: 1 on success
 *          0 on failure */
int extract_file(pack_t *pack, packedfile_t *info)
{
	FILE *out;
	char buf[BUFSIZ * 2];
	int rest = 0, count = BUFSIZ * 2;

	if (!pack) return 0;

	if (!makepath(info->name)) return 0;

	out = fopen(info->name, "w");
	if (!out) {
		LOGF("Can't open file `%s' for write: %s.\n",
		     info->name, strerror(errno));
		return 0;
	}

	fseek(pack->handle, endian_little_to_host(info->filepos), SEEK_SET);

	rest = endian_little_to_host(info->filelen);
	if (rest < BUFSIZ * 2) count = rest;

	while (fread(buf, count, 1, pack->handle)) {
		if (!fwrite(buf, count, 1, out))
			break;

		rest -= count;
		if (rest < BUFSIZ * 2) count = rest;
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
int packfile_extract(char *packfile, int mode)
{
	int i, n;
	packedfile_t *mapped;
	pack_t *pack;
	long ps;

	pack = packfile_open(packfile);
	if (!pack) return 0;

	/* With mmap() we MUST use a multiple of the page size as offset */
	ps = sysconf(_SC_PAGESIZE);
	n = endian_little_to_host(pack->header.dirofs) % ps;
	mapped = mmap(0, endian_little_to_host(pack->header.dirlen) + n,
		      PROT_READ, MAP_PRIVATE, fileno(pack->handle),
		      endian_little_to_host(pack->header.dirofs) - n);

	if (mapped == MAP_FAILED) {
		perror("mmap");
		return 0;
	}

	/* Let's jump to the beginning of our sweet data */
	(char *) mapped += n;

	for (i = 0; i < pack->numfiles; i++) {
		fprintf(stderr, "%s\n", mapped->name);
		if (mode) extract_file(pack, mapped);
		++mapped;
	}

	printf("Packfile %s (%i files)\n", packfile, pack->numfiles);

	return 1;
}

ino_t finode; /* Inode of the output file */
dev_t fdev;   /* Device of the output file */

/* Appends the file @name to the previosly opened packddir file @f
 * @name: name of the file to be added, absolute or relative
 * @f: the packddir file
 *
 * returns: 0 on failure,
 *	    1 on success */
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

/* Adds recursively a directory to a packddir file
 * @f: the packddir file
 * @name: the directory name
 *
 * returns: 0 on failure,
 *	    1 on success. */
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
		LOGF("E: Trying to opendir() `%s': %s.\n",
		     name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		if ((len + strlen(d->d_name)) >= 56) {
			LOGF("File name too long: `%s/%s'\n",
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

		if ((buf.st_dev == fdev) && (buf.st_ino == finode)) {
			LOGF("Skipping the output archive `%s'.\n", file);
			continue;
		}


		if (S_ISDIR(buf.st_mode))
			traverse_dir(f, file);
		else {
			pf = malloc(sizeof(packedfile_t));
			strcpy(pf->name, file);
			pf->filepos = endian_host_to_little(ftell(f));
			pf->filelen = endian_host_to_little(buf.st_size);
			list_append(packedfiles, pf);

			append_file(f, file);
		}
	}
	return 1;
}

/* Creates a PackdDir file with given files in it
 * @name: PackdDir file name
 * @files: name of files and/or directories to add
 *
 * returns: 1 on success
 *          0 on failure */
static int packfile_create(char *name, char *files[])
{
	int i = 0;
	int dirlen;
	FILE *out;
	struct stat buf;
	int position;
	list_node_t node;
	packedfile_t *f;

	if (!name) out = stdout;
	else {
		int r;

		if (access(name, R_OK) == 0) {
			int c;

			LOGF("`%s' exists. Overwrite (N/y)? ", name);
			c = getchar();
			if (c != 'y') {
				LOG("OK, exiting.\n");
				exit(EXIT_FAILURE);

			} else LOGF("Overwriting `%s' as requested.\n", name);
		}

		out = fopen(name, "w");
		if (!out) {
			LOGF("E: Trying to fopen() `%s': %s.\n",
			     name, strerror(errno));
			return 0;
		}

		r = stat(name, &buf);
		if (r == -1) {
			perror("stat() on output file");
			return 0;
		} else {
			fdev = buf.st_dev;
			finode = buf.st_ino;
		}

	}

	packedfiles = list_new();
	fseek(out, 12, SEEK_SET);

	while (files[i] != NULL) {
		int r;

		r = stat(files[i], &buf);
		if (r == -1) {
			LOGF("Failed to stat() `%s': %s.\n",
			     files[i], strerror(errno));
			return 0;
		}

		if ((buf.st_dev == fdev) && (buf.st_ino == finode)) {
			LOGF("Skipping the output archive `%s'.\n", files[i]);
			continue;
		}

		if (S_ISDIR(buf.st_mode))
			traverse_dir(out, files[i]);
		else {
			f = malloc(sizeof(packedfile_t));
			strcpy(f->name, files[i]);
			f->filepos = endian_host_to_little(ftell(out));
			f->filelen = endian_host_to_little(buf.st_size);
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

	dirlen = endian_host_to_little(ftell(out) - position);
	position = endian_host_to_little(position);

	fseek(out, 4, SEEK_SET);
	fwrite(&position, 4, 1, out);
	fwrite(&dirlen, 4, 1, out);

	fseek(out, 0, SEEK_SET);
	i = endian_host_to_big(IDPAKHEADER);
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
	fprintf(stderr, "Usage: upackddir [OPTIONS] FILENAME...\n"
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
	int c;
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
				LOG("W: getopt_long() returned an "
				    "impossible value.\n");
				return EXIT_FAILURE;
		}
	}
	if (!create && !list)
		extract = 1;

	if (file && extract) {
		LOG("W: Ignoring -f. Use it only when creating "
		    "a new archive.\n");
	}

	if (create && (list || extract))
		error_options_conflict();

	if (argv[optind] != NULL) {
		if (create) {
			ret = packfile_create(file, argv + optind);
			if (!ret) LOG("Can't create pack file.\n");
		}

		if (list || extract)
			ret = packfile_extract(argv[optind], extract);
	} else {
		LOG("Argument expected.\n");
		usage_display();
	}

	return (ret) ? EXIT_SUCCESS : EXIT_FAILURE;
}
/* vim set ai ts=8 */
