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
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

int main(int argc, const char **argv)
{
	double factor = 1.0 / 1024;
	unsigned int count = 0;
	struct bsv_insn tone;
	int ifd;

	if (argc < 2)
		return EXIT_FAILURE;

	// sd FILE SQUARE_PROPORTION SINE_PROPRTION FDIV
	ifd = open(argv[1], O_RDONLY);
	if (argc >= 3)
		pcsp.prop_square = strtod(argv[2], NULL);
	if (argc >= 4)
		pcsp.prop_sine = strtod(argv[3], NULL);
	if (argc >= 5)
		factor = 1.0 / strtod(argv[4], NULL);

	pcsp.file_ptr    = fopen("/dev/stdout", "wb");

	while (read(ifd, &tone, sizeof(struct bsv_insn)) ==
	    sizeof(struct bsv_insn))
	{
		long frequency = 0x1234DD / tone.divisor;

		pcspkr_output(&pcsp, frequency,
		              tone.duration * SAMPLE_RATE * factor,
		              tone.af_pause * SAMPLE_RATE * factor);
		fprintf(stderr, "(%5u) %5hu %5ld %5hu %5hu\n",
			++count, tone.divisor, frequency, tone.duration,
		        tone.af_pause);
		fflush(stderr);
	}

	fprintf(stderr, "\n");
	return EXIT_SUCCESS;
}
