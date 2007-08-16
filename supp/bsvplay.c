/*
 *	bsvplay.c - BASICA binary music format interpreter
 *	Copyright © Jan Engelhardt <jengelh [at] gmx de>, 2002 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libHX.h>
#include "pcspkr.h"

#define SAMPLE_RATE 48000

struct bsv_insn {
	uint16_t divisor, duration, af_pause;
};

static struct pcspkr pcsp = {
	.format      = PCSPKR_16,
	.sample_rate = 48000,
	.prop_square = 1,
	.prop_sine   = 1,
};

static void parse_file(const char *file)
{
	unsigned int count = 0, ticks = 0;
	struct bsv_insn tone;
	int fd;

	if (strcmp(file, "-") == 0)
		fd = STDIN_FILENO;
	else
		fd = open(file, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n", file, strerror(errno));
		return;
	}

	while (read(fd, &tone, sizeof(struct bsv_insn)) ==
	    sizeof(struct bsv_insn))
	{
		long frequency = 0x1234DD / tone.divisor;

		fprintf(stderr, "(%5u) %5hu %5ld %5hu %5hu\n",
			++count, tone.divisor, frequency, tone.duration,
		        tone.af_pause);
		/*
		 * It seems that in the sample BSV executables from 1989
		 * calculate the cpu speed and then do around 1086 ticks/sec.
		 * entertan.exe: 199335 / 183 = 1089
		 * ihold.exe:     73248 /  68 = 1077
		 * maplleaf.exe: 170568 / 157 = 1086
		 * mnty.exe:     119680 / 110 = 1088
		 * willtell.exe: 225350 / 206 = 1093
		 */
		ticks += tone.duration + tone.af_pause;
		pcspkr_output(&pcsp, frequency,
		              tone.duration * SAMPLE_RATE / 1086,
		              tone.af_pause * SAMPLE_RATE / 1086);
	}

	fprintf(stderr, "Total ticks: %u\n", ticks);
	return;
}

int main(int argc, const char **argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'i', .type = HXTYPE_DOUBLE, .ptr = &pcsp.prop_sine,
		 .help = "Proportion of sine-wave calculation mixed in"},
		{.sh = 'q', .type = HXTYPE_DOUBLE, .ptr = &pcsp.prop_square,
		 .help = "Proportion of square-wave calculation mixed in"},
		{.sh = 'r', .type = HXTYPE_UINT, .ptr = &pcsp.sample_rate,
		 .help = "Sample rate (default: 48000)"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, &argc, &argv, HXOPT_USAGEONERR) <= 0)
		return EXIT_FAILURE;

	pcsp.file_ptr = fdopen(STDOUT_FILENO, "wb");
	if (argc == 1)
		parse_file("-");
	else
		while (--argc > 0)
			parse_file(*++argv);

	return EXIT_SUCCESS;
}
