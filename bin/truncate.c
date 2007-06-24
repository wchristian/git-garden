/*
 *	truncate.c - truncate or expand a file
 *	by Jan Engelhardt <jengelh [at] gmx de>, 2004 - 2007
 *	http://jengelh.hopto.org/
 *	released in the Public Domain
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	unsigned long long sz;
	char *err;
	int fd;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s SIZE FILE[...]\n", *argv);
		return EXIT_FAILURE;
	}

	sz = strtoull(*++argv, &err, 0);
	if (sz != (off_t)sz) {
		fprintf(stderr, "Error: No LARGEFILE support!\n");
		return EXIT_FAILURE;
	}
	if (err == *argv) {
		fprintf(stderr, "Error: Invalid size given!\n");
		return EXIT_FAILURE;
	}

	while (*++argv != NULL) {
		if ((fd = open(*argv, O_RDWR)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        *argv, strerror(errno));
			continue;
		}
		if (ftruncate(fd, sz) != 0)
			fprintf(stderr, "%s: truncate(): %s\n",
			        *argv, strerror(errno));
		close(fd);
	}

	return EXIT_SUCCESS;
}
