/*
 *	bsvplay.c - BASICA binary music format interpreter
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/option.h>
#include "pcspkr.h"

struct bsv_insn {
	uint16_t divisor, duration, af_pause;
};

static struct pcspkr pcsp = {
	.format      = PCSPKR_16,
	.sample_rate = 48000,
	.prop_square = 1,
	.prop_sine   = 1,
};

static unsigned int filter_lo = 0, filter_hi = ~0U;

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
		bool silenced;

		silenced = frequency < filter_lo || frequency > filter_hi;

		fprintf(stderr, "(%5u) %5hu %5ld%c %5hu %5hu\n",
			++count, tone.divisor, frequency,
			silenced ? '*' : ' ', tone.duration,
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
		if (silenced)
			pcspkr_silence(&pcsp, (tone.duration + tone.af_pause) *
				pcsp.sample_rate / 1086);
		else
			pcspkr_output(&pcsp, frequency,
			              tone.duration * pcsp.sample_rate / 1086,
			              tone.af_pause * pcsp.sample_rate / 1086);
	}

	fprintf(stderr, "Total ticks: %u\n", ticks);
}

int main(int argc, const char **argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'H', .type = HXTYPE_UINT, .ptr = &filter_hi,
		 .help = "High frequency cutoff (low-pass filter)"},
		{.sh = 'L', .type = HXTYPE_UINT, .ptr = &filter_lo,
		 .help = "Low frequency cutoff (high-pass filter)"},
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
