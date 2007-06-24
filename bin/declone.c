/*
 *	declone.c - unlink but preserve a file
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
#define BLOCKSIZE 4096

int main(int argc, const char **argv)
{
	++argv;
	for (; --argc > 0 && *argv != NULL; ++argv) {
		struct stat sb;
		ssize_t ret;
		char buffer[BLOCKSIZE];
		int in, out;

		if ((in = open(*argv, O_RDONLY)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        *argv, strerror(errno));
			continue;
		}
		if (fstat(in, &sb) < 0) {
			fprintf(stderr, "Could not stat %s: %s\n",
			        *argv, strerror(errno));
			continue;
		}
		if (unlink(*argv) < 0) {
			fprintf(stderr, "Could not unlink %s: %s\n",
			        *argv, strerror(errno));
			continue;
		}
		if ((out = open(*argv, O_WRONLY | O_TRUNC | O_CREAT,
		    sb.st_mode)) < 0) {
			fprintf(stderr, "Could not recreate/open file %s: %s\n",
			        *argv, strerror(errno));
			fgets(buffer, 4, stdin);
			continue;
		}

		printf("* %s\n", *argv);
		fchown(out, sb.st_uid, sb.st_gid);
		fchmod(out, sb.st_mode);
		while ((ret = read(in, buffer, BLOCKSIZE)) > 0)
			if (write(out, buffer, ret) < 0) {
				fprintf(stderr, "Error during write() to %s: ",.
				        "%s\n", *argv, strerror(errno));
				break;
			}

		if (ret < 0)
			fprintf(stderr, "Error during read() on %s: %s\n",
			        *argv, strerror(errno));
		close(in);
		close(out);
	}
	return EXIT_SUCCESS;
}
