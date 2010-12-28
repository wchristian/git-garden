/*
 *	Analyze /proc/iomem
 *	Author: Jan Engelhardt, 2008-2010
 *	released into the Public Domain
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libHX/ctype_helper.h>
#include <libHX/defs.h>
#include <libHX/init.h>
#include <libHX/string.h>

static int main2(int argc, const char **argv)
{
	const char *file = "/proc/iomem";
	uint64_t start, end;
	hxmc_t *ln = NULL;
	char *e;
	FILE *fp;

	if (argc >= 2)
		file = argv[1];

	fp = fopen(file, "r");
	if (fp == NULL) {
		fprintf(stderr, "fopen: /proc/iomem: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	while (HX_getl(&ln, fp) != NULL) {
		HX_chomp(ln);
		start = strtoull(ln, &e, 16);
		if (*e != '-')
			continue;
		++e;
		end = strtoull(e, &e, 16);
		while (HX_isspace(*e))
			++e;
		if (*e == ':')
			++e;
		while (HX_isspace(*e))
			++e;
		printf("%5llu MB\t%s\n",
		       static_cast(unsigned long long, (end - start) >> 20),
		       ln);
	}

	HXmc_free(ln);
	fclose(fp);
	return EXIT_SUCCESS;
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
