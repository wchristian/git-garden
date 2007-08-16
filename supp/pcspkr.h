#ifndef _PCSPKR_H
#define _PCSPKR_H 1

#ifndef __cplusplus
#	include <stdio.h>
#else
#	include <cstdio>
extern "C" {
#endif

enum pcspkr_format {
	PCSPKR_8  = 8,
	PCSPKR_16 = 16,
};

struct pcspkr {
	double prop_square, prop_sine;
	FILE *file_ptr;
	unsigned int sample_rate;
	enum pcspkr_format format;
};

extern void pcspkr_output(const struct pcspkr *, long, long, long);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PCSPKR_H */
