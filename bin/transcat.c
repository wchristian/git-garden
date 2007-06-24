/*
 *	transcat.c - diskspace-conversing cat operation
 *	by Jan Engelhardt <jengelh [at] gmx de>, 2007
 *	http://jengelh.hopto.org/
 *	released in the Public Domain
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int transconcatenate(const char *file)
{
	struct stat source_sb;
	int source_fd = -1;

	if ((source_fd = open(file, O_RDONLY)) < 0) {
		perror("Could not open %s", argv[i]);
		goto out;
	}
	if (fstat(source_fd, &source_sb) < 0) {
		perror("Could not stat source_fd");
		goto out;
	}
	if (fstat(target_fd, &target_sb) < 0) {
		perror("Could not stat target_fd");
		goto out;
	}
	if (ftruncate(target_fd, target_sb.st_size + source_sb.st_size) < 0) {
		perror("Could not ftruncate target_fd");
		goto out;
	}
	close(source_fd);
	return 1;

 out:
	close(source_fd);
	fprintf(stderr, "Stopped transconcatenation at %s.\n", file);
	return 0;
}

int main(int argc, const char **argv)
{
	int i, source_fd, target_fd;
	struct stat source_sb, target_sb;

	if (argc < 3) {
		fprintf(stderr,
			"Usage: %s first_file parts...\n"
			"e.g.:  %s part1.iso part2.iso part3.iso\n"
			"will merge part2 and part3 _onto_ part1\n",
			*argv, *argv
		);
		return EXIT_FAILURE;
	}

	if ((target_fd = open(argv[1], O_RDWR)) < 0) {
		perror("Could not open %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	for (i = 2; i < argc; ++i)
		if (!transconcatenate(argv[i]))
			return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
