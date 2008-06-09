/*
 *	tailhex.c - tail follower with hexdump
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2005 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX.h>

/* Functions */
static int get_options(int *, const char ***);
static void getopt_op_e(const struct HXoptcb *);
static inline int printable(char);

/* Variables */
static struct {
	long long start;
	int approx, follow, bsize, quad;
} Opt = {
	.start  = 0,
	.approx = 0,
	.follow = 0,
	.bsize  = 16,
	.quad   = 0,
};

//-----------------------------------------------------------------------------
int main(int argc, const char **argv)
{
	unsigned int buf_offset = 0;
	unsigned char *buf;
	struct stat sb;
	long long rax;
	int ret, fd;

	if (get_options(&argc, &argv) <= 0)
		return EXIT_FAILURE;

	if ((buf = malloc(Opt.bsize)) == NULL) {
		fprintf(stderr, "Could not allocate buffer of size %d\n", Opt.bsize);
		return EXIT_FAILURE;
	}

	if ((fd = open(*++argv, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n", *argv, strerror(errno));
		return EXIT_FAILURE;
	}

	fstat(fd, &sb);
	if (!Opt.start && Opt.approx)
		Opt.start = (sb.st_size >> 8) << 8;
	if (Opt.start < 0)
		rax = lseek(fd, Opt.start, SEEK_END);
	else
		rax = lseek(fd, Opt.start, SEEK_SET);
	if (rax < 0)
		perror("seek to %lld failed");

	while (1) {
		long long pos;
		int i;

		pos = lseek(fd, 0, SEEK_CUR);
		ret = read(fd, buf + buf_offset, Opt.bsize - buf_offset);
		if (ret == 0) /* eof */
			break;
		if (ret < 0) {
			perror("read()");
			exit(EXIT_FAILURE);
		}
		if (Opt.follow && ret < Opt.bsize) {
			buf_offset = ret;
			sched_yield();
			continue;
		}

		buf_offset = 0;
		if (Opt.quad || pos > (long long)0xFFFFFFFF)
			printf("0x%016llx |", pos);
		else
			printf("0x%08llx |", pos);

		for (i = 0; i < Opt.bsize; ++i) {
			printf(" ");
			if (i < ret)   printf("%02x", (int)buf[i]);
			else           printf("  ");
			if (++i < ret) printf("%02x", (int)buf[i]);
			else           printf("  ");
		}
		printf(" | ");

		for (i = 0; i < Opt.bsize; ++i) {
			if (i < ret)   printf("%c", printable(buf[i]));
			else           printf(" ");
			if (++i < ret) printf("%c", printable(buf[i]));
			else           printf(" ");
		}
		printf("\n");
	}

	return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
static int get_options(int *argc, const char ***argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'B', .type = HXTYPE_INT, .ptr = &Opt.bsize,
		 .help = "Buffer and window width"},
		{.sh = 'Q', .type = HXTYPE_NONE, .ptr = &Opt.quad,
		 .help = "Use 64-bit position numbers beginning from 0"},
		{.sh = 'a', .type = HXTYPE_NONE, .ptr = &Opt.approx,
		 .help = "Approximate position start"},
		{.sh = 'e', .type = HXTYPE_STRING, .cb = getopt_op_e,
		 .help = "Exact position start"},
		{.sh = 'f', .type = HXTYPE_NONE, .ptr = &Opt.follow,
		 .help = "Output appended data as file grows", NULL},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) <= 0)
		return 0;

	if (*argc == 1) {
		fprintf(stderr, "Error: You need to provide a filename\n");
		return 0;
	}

	return 1;
}

static void getopt_op_e(const struct HXoptcb *cbi)
{
	Opt.start = strtoll(cbi->data, NULL, 0);
	return;
}

static inline int printable(char x)
{
	return isprint(x) ? x : '.';
}
