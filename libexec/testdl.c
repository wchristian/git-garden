/*
 *	testdl - test symbol resolving
 *	written by Jan Engelhardt <jengelh [at] medozas de>, 2004 - 2007
 *	http://jengelh.medozas.de/
 *	released in the Public Domain
 */
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/defs.h>
#include <libHX/deque.h>
#include <libHX/misc.h>
#include <libHX/option.h>
#include <libHX/string.h>
#define MAXFNLEN 256

static struct HXdeque *tdl_search_dirs = NULL;
static struct HXdeque *tdl_load_libs   = NULL;

static bool tdl_open_direct(const char *lib)
{
	printf("Preloading %s", lib);
	if (dlopen(lib, RTLD_GLOBAL | RTLD_NOW) == NULL) {
		printf(": %s\n", dlerror());
		return false;
	} else {
		printf(": ok\n");
		return true;
	}
}

/**
 * tdl_lname - load a library by "-lfoo"
 */
static bool tdl_open_by_lname(const char *lib)
{
	const struct HXdeque_node *node;
	hxmc_t *file = HXmc_meminit(NULL, 0);
	bool ret = false;
	struct stat sb;

	for (node = tdl_search_dirs->first; node != NULL; node = node->next) {
		HXmc_trunc(&file, 0);
		HXmc_strcat(&file, node->ptr);
		HXmc_strcat(&file, "/lib");
		HXmc_strcat(&file, lib);
		if (strstr(lib, ".so.") == NULL)
			HXmc_strcat(&file, ".so");
		if (stat(file, &sb) == 0)
			ret = tdl_open_direct(file);
	}
	HXmc_free(file);
	return ret;
}

/**
 * tdl_read_ldso_conf1 -
 * @file:	file to analyze
 *
 * Reads @file and adds the search paths listed therein.
 */
static void tdl_read_ldso_conf1(const char *file)
{
	hxmc_t *ln = NULL;
	FILE *fp;

	if ((fp = fopen(file, "r")) == NULL)
		return;

	while (HX_getl(&ln, fp) != NULL) {
		char *q;
		if (*ln != '/')
			continue;
		HX_chomp(ln);
		if ((q = strchr(ln, '=')) != NULL)
			*q = '\0';
		HXdeque_push(tdl_search_dirs, HX_strdup(ln));
	}

	fclose(fp);
	HXmc_free(ln);
}

/**
 * tdl_read_ldso_conf -
 *
 * Looks into /etc/ld.so.conf and /etc/ld.so.conf.d.
 */
static void tdl_read_ldso_conf(void)
{
	const char *dentry;
	char buf[MAXFNLEN];
	void *dirp;

	tdl_read_ldso_conf1("/etc/ld.so.conf");
	if ((dirp = HXdir_open("/etc/ld.so.conf.d")) == NULL)
		return;
	while ((dentry = HXdir_read(dirp)) != NULL) {
		snprintf(buf, sizeof(buf), "/etc/ld.so.conf.d/%s", dentry);
		tdl_read_ldso_conf1(buf);
	}

	HXdir_close(dirp);
}

static bool tdl_get_options(int *argc, const char ***argv)
{
	/* must not be static const */
	struct HXoption options_table[] = {
		{.sh = 'L', .type = HXTYPE_STRDQ, .ptr = tdl_search_dirs,
		 .help = "Add directory to search path", .htyp = "dir"},
		{.sh = 'l', .type = HXTYPE_STRDQ, .ptr = tdl_load_libs,
		 .help = "Load extra libraries"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	return HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) > 0;
}

int main(int argc, const char **argv)
{
	const struct HXdeque_node *node;
	void *handle;

	if ((tdl_search_dirs = HXdeque_init()) == NULL ||
	    (tdl_load_libs = HXdeque_init()) == NULL) {
		perror("HXdeque_init");
		abort();
	}

	if (!tdl_get_options(&argc, &argv))
		return EXIT_FAILURE;

	if (argc == 1) {
		fprintf(stderr, "%s: missing operand.\n", HX_basename(*argv));
		return EXIT_FAILURE;
	}

	tdl_read_ldso_conf();
	setvbuf(stdout, NULL, _IONBF, 0);

	for (node = tdl_load_libs->first; node != NULL; node = node->next) {
		const char *lib = node->ptr;

		if (strchr(lib, '/') != NULL)
			tdl_open_direct(lib);
		else
			tdl_open_by_lname(lib);
	}

	while (*++argv != NULL) {
		fprintf(stderr, "Loading %s", *argv);
		if ((handle = dlopen(*argv, RTLD_NOW)) == NULL) {
			fprintf(stderr, ": %s\n", dlerror());
		} else {
			fprintf(stderr, ": ok\n");
			dlclose(handle);
		}
	}

	return EXIT_SUCCESS;
}
