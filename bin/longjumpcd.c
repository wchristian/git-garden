/*
 *	longjumpcd.c - cd to a nearby path
 *	Copyright © Jan Engelhardt <jengelh [at] gmx de>, 2005 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#define _GNU_SOURCE 1
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX.h>
#define PREFIX "longjumpcd: "
#define MAX_DOWN_DEPTH 5
#define MAXFNLEN 256

/* Functions */
static ino_t get_inode(const char *);
static int try_path(const char *, const char *, ino_t);

/* Variables */
static ino_t Base_inode, Root_inode;

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	struct HXdeque *dq;
	char *upath;
	int hit_root = 0, depth = 0;

	++argv;
	if (argc < 2 || (**argv == '-' && argc < 3)) {
		fprintf(stderr, PREFIX "wrong number of arguments\n");
		return EXIT_FAILURE;
	}

	Base_inode = get_inode(".");
	Root_inode = get_inode("/");
	dq         = HXdeque_init();
	upath      = malloc(MAXFNLEN);

	HXdeque_push(dq, HX_strdup("."));
	HXdeque_push(dq, NULL);
	strcpy(upath, "..");

	/*
	 * In the beginning, @dq only contains ".", which means to search in
	 * ".".  Notice the NULL element, which is added to distinguish
	 * elements-to-be- processed and new-elements-added-while-processing.
	 */
	while (1) {
		/* Search all directories in DQ for the wanted one (@*argv) */
		void *ptr;
		while ((ptr = HXdeque_shift(dq)) != NULL) {
			if (try_path(ptr, *argv, 0))
				exit(EXIT_SUCCESS);

			/*
			 * If there was no directory @ptr + *@argv, gather all
                         * child directories of @ptr and put them at the end of
                         * @dq.
                         */
			void *wd = HXdir_open(ptr);
			char fullname[MAXFNLEN];
			const char *this;
			struct stat sb;

			while ((this = HXdir_read(wd)) != NULL) {
				if (strcmp(this, ".") == 0 ||
				    strcmp(this, "..") == 0)
					continue;
				snprintf(fullname, MAXFNLEN, "%s/%s",
				         (const char *)ptr, this);
				if (stat(fullname, &sb) == 0 &&
				    S_ISDIR(sb.st_mode))
					HXdeque_push(dq, HX_strdup(fullname));
			}

			HXdir_close(wd);
			free(ptr);
		}

		if (dq->last != NULL && dq->last->ptr != NULL)
			HXdeque_push(dq, NULL);

		/*
		 * Stepview (using src/linux as example):
		 * In the beginning:
		 *     DQ = {".", NULL};
		 * In the while loop:
		 *     DQ = {NULL, "arch", "crypto", "drivers", ...};
		 * After the loop:
		 *     DQ = {"arch", "crypto", "drivers", ..., NULL};
		 */

		/*
		 * Stop if we walked too far (only count for DOWN!).  Setting
		 * MAX_DOWN_DEPTH to 1 will render this program useless.
		 */
		if(++depth >= MAX_DOWN_DEPTH)
			break;

		/*
		 * If the root directory was already seen, do not try any
		 * parent directories. root->parent == root.
		 */
		if(hit_root)
			continue;
		if(try_path(upath, *argv, Base_inode))
			break;
		if(get_inode(upath) == Root_inode)
			++hit_root;
		HX_strlcat(upath, "/..", MAXFNLEN);
	}

	free(upath);
	return EXIT_SUCCESS;
}

static ino_t get_inode(const char *path)
{
	struct stat sb;
	int fd;

	if ((fd = open(path, O_RDONLY | O_DIRECTORY)) < 0) {
		fprintf(stderr, PREFIX "Could not open \"%s\": %s\n",
		        path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	fstat(fd, &sb);
	return sb.st_ino;
}

static int try_path(const char *base, const char *path, ino_t ino)
{
	char fullname[MAXFNLEN];
	struct stat sb;

	snprintf(fullname, MAXFNLEN, "%s/%s", base, path);

	if (stat(fullname, &sb) == 0 && sb.st_ino != ino &&
	    S_ISDIR(sb.st_mode) && sb.st_ino != Root_inode) {
		printf("%s\n", fullname);
		return 1;
	}
	return 0;
}
