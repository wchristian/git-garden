/*
 *	extended copy
 *	written by Jan Engelhardt <jengelh [at] medozas de>, 2008
 *	http://jengelh.medozas.de/
 *	released in the Public Domain
 */
#define _GNU_SOURCE 1
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/init.h>
#include <libHX/option.h>

enum {
	XCP_MMAP,
	XCP_SPLICE,
};

static unsigned int xcp_mode = XCP_MMAP;

static bool xcp_get_options(int *argc, const char ***argv)
{
	static struct HXoption options_table[] = {
		{.sh = 'm', .ln = "mmap", .ptr = &xcp_mode,
		 .type = HXTYPE_VAL, .val = XCP_MMAP,
		 .help = "Use mmap(2) for copying"},
		{.sh = 's', .ln = "splice", .ptr = &xcp_mode,
		 .type = HXTYPE_VAL, .val = XCP_SPLICE,
		 .help = "Use splice(2) for copying"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	return HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) > 0;
}

static int xcp_splice(const char *input, const char *output)
{
	off_t ioff = 0, ooff = 0;
	int ifd, pfd[2], ofd;
	struct stat isb;
	long ret;

	ifd = open(input, O_RDONLY);
	if (ifd < 0) {
		fprintf(stderr, "open(\"%s\"): %s\n", input, strerror(errno));
		return EXIT_FAILURE;
	}

	ofd = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (ofd < 0) {
		fprintf(stderr, "open(\"%s\"): %s\n", output, strerror(errno));
		return EXIT_FAILURE;
	}

	if (pipe(pfd) < 0) {
		perror("pfd");
		return EXIT_FAILURE;
	}

	if (fstat(ifd, &isb) < 0) {
		perror("fstat");
		return EXIT_FAILURE;
	}

	while (ioff != isb.st_size) {
		ret = splice(ifd, &ioff, pfd[1], NULL, isb.st_size, 0);
		if (ret < 0) {
			perror("splice-in");
			return EXIT_FAILURE;
		}
		ret = splice(pfd[0], NULL, ofd, &ooff, ret, 0);
		if (ret < 0) {
			perror("splice-out");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

static int xcp_mmap(const char *input, const char *output)
{
	struct stat isb;
	int ifd, ofd;
	void *area;

	ifd = open(input, O_RDONLY);
	if (ifd < 0) {
		fprintf(stderr, "open(\"%s\"): %s\n", input, strerror(errno));
		return EXIT_FAILURE;
	}

	ofd = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (ofd < 0) {
		fprintf(stderr, "open(\"%s\"): %s\n", output, strerror(errno));
		return EXIT_FAILURE;
	}

	if (fstat(ifd, &isb) < 0) {
		perror("fstat");
		return EXIT_FAILURE;
	}

	area = mmap(NULL, isb.st_size, PROT_READ, MAP_SHARED, ifd, 0);
	if (area == (void *)-1) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	if (write(ofd, area, isb.st_size) != isb.st_size) {
		perror("write");
		return EXIT_FAILURE;
	}

	munmap(area, isb.st_size);
	close(ifd);
	close(ofd);
	return EXIT_SUCCESS;
}

static int main2(int argc, const char **argv)
{
	if (!xcp_get_options(&argc, &argv))
		return EXIT_FAILURE;
	if (argc != 3) {
		fprintf(stderr, "%s: Source and destination file required\n",
		        *argv);
		return EXIT_FAILURE;
	}

	if (xcp_mode == XCP_SPLICE)
		return xcp_splice(argv[1], argv[2]);
	else if (xcp_mode == XCP_MMAP)
		return xcp_mmap(argv[1], argv[2]);
	return EXIT_FAILURE;
}

int main(int argc, const char **argv)
{
	int ret;

	if ((ret = HX_init()) < 0) {
		fprintf(stderr, "HX_init: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	ret = main2(argc, argv);
	HX_exit();
	return ret;
}
