/*
 *	testdl - test symbol resolving
 *	by Jan Engelhardt <jengelh [at] gmx de>, 2004 - 2007
 *	http://jengelh.hopto.org/
 *	released in the Public Domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX.h>

int main(int argc, char **argv)
{
	void *handle;
	setvbuf(stdout, NULL, _IONBF, 0);

	while (*++argv != NULL) {
		fprintf(stderr, "Loading %s:", *argv);
		if ((handle = HX_dlopen(*argv)) == NULL) {
			fprintf(stderr, "  %s\n", HX_dlerror());
		} else {
			fprintf(stderr, " ok\n");
			HX_dlclose(handle);
		}
	}

	return EXIT_SUCCESS;
}
