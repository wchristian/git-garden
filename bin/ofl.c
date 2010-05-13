/*
 *	Show processes using directories/files/mountpoints
 *
 *	(While it says mountpoint in the source, any directory is acceptable,
 *	as are files.)
 *
 *	written by Jan Engelhardt, 2008 - 2010
 *	Released in the Public Domain.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX.h>

/**
 * @sb:		just space
 * @pid:	pid for current process
 * @signal:	signal to send
 * @check:	check for symlink
 * @found:	found something (used for exit value)
 */
struct ofl_compound {
	struct stat sb;
	pid_t pid;
	unsigned char signal;
	bool check, found;
};

static bool pids_only;

static const char *ofl_comm(pid_t pid, char *buf, size_t size)
{
	char src[64], dst[512];
	const char *p;
	ssize_t ret;

	snprintf(src, sizeof(src), "/proc/%u/exe", (unsigned int)pid);
	ret = readlink(src, dst, sizeof(dst) - 1);
	if (ret < 0) {
		*buf = '\0';
		return buf;
	}
	dst[ret] = '\0';
	p = HX_basename(dst);
	strncpy(buf, p, size);
	return buf;
}

/**
 * ofl_file - check if file is within directory
 * @mnt:	mountpoint
 * @file:	file that is supposed to be within @mnt
 *
 * Returns true if that seems so.
 * We do not check for the existence of @file using lstat() or so - it is
 * assumed this exists if it is found through procfs. In fact,
 * /proc/<pid>/task/<tid>/fd/<n> might point to the ominous
 * "/foo/bar (deleted)" which almost never exists, but it shows us anyway that
 * the file is still in use.
 */
static bool ofl_file(const char *mnt, const char *file, const char *ll_entry,
    struct ofl_compound *data)
{
	ssize_t mnt_len;
	const char *p;

	/* Strip extra slashes at the end */
	mnt_len = strlen(mnt);
	for (p = mnt + mnt_len - 1; p >= mnt && *p == '/'; --p)
		--mnt_len;

	if (strncmp(file, mnt, mnt_len) != 0)
		return false;
	if (file[mnt_len] != '\0' && file[mnt_len] != '/')
		return false;

	data->found = true;
	if (pids_only) {
		printf("%u ", data->pid);
	} else if (data->signal == 0) {
		char buf[24];
		printf("%u(%s): %s -> %s\n", data->pid,
		       ofl_comm(data->pid, buf, sizeof(buf)), ll_entry, file);
		return false; /* so that more FDs will be inspected */
	}

	if (kill(data->pid, data->signal) < 0) {
		if (errno == ESRCH)
			return true;
		return false;
	}
	return true;
}

/**
 * ofl_pmap - read process mappings
 * @mnt:	mountpoint
 * @map_file:	/proc/<pid>/maps
 */
static bool ofl_pmap(const char *mnt, const char *map_file,
    struct ofl_compound *data)
{
	hxmc_t *line = NULL;
	bool ret = false;
	unsigned int i;
	const char *p;
	FILE *fp;

	if ((fp = fopen(map_file, "r")) == NULL)
		return false;

	while (HX_getl(&line, fp) != NULL) {
		HX_chomp(line);
		p = line;
		for (i = 0; i < 5; ++i) {
			while (!HX_isspace(*p) && *p != '\0')
				++p;
			while (HX_isspace(*p))
				++p;
		}
		if (*p == '\0')
			continue;
		ret = ofl_file(mnt, p, map_file, data);
		if (ret)
			break;
	}

	HXmc_free(line);
	fclose(fp);
	return ret;
}

/**
 * ofl_one - check a symlink
 * @mnt:	Mountpoint that is to be removed.
 * @entry:	Path to a symlink.
 *
 * Returns true if the process does not exist anymore or has been signalled.
 */
static bool ofl_one(const char *mnt, const char *entry,
    struct ofl_compound *data)
{
	ssize_t lnk_len;
	char tmp[512];

	if (data->check)
		if (lstat(entry, &data->sb) < 0 || !S_ISLNK(data->sb.st_mode))
			return false;

	lnk_len = readlink(entry, tmp, sizeof(tmp) - 1);
	if (lnk_len < 0)
		return false;
	tmp[lnk_len] = '\0';

	return ofl_file(mnt, tmp, entry, data);
}

/**
 * ofl_taskfd - iterate through /proc/<pid>/task/<tid>/fd/
 */
static bool ofl_taskfd(const char *mnt, const char *path,
    struct ofl_compound *data)
{
	const char *de;
	char tmp[256];
	void *dir;
	bool ret;

	dir = HXdir_open(path);
	if (dir == NULL)
		return false;
	while ((de = HXdir_read(dir)) != NULL) {
		if (*de == '.')
			continue;
		snprintf(tmp, sizeof(tmp), "%s/%s", path, de);
		if (lstat(tmp, &data->sb) < 0 || !S_ISLNK(data->sb.st_mode))
			continue;
		ret = ofl_one(mnt, tmp, data);
		if (ret)
			break;
	}
	HXdir_close(dir);
	return ret;
}

/**
 * ofl_task - iterate through /proc/<pid>/task/
 */
static void ofl_task(const char *mnt, const char *path,
    struct ofl_compound *data)
{
	const char *de;
	char tmp[256];
	void *dir;

	dir = HXdir_open(path);
	if (dir == NULL)
		return;
	while ((de = HXdir_read(dir)) != NULL) {
		if (*de == '.')
			continue;
		snprintf(tmp, sizeof(tmp), "%s/%s/fd", path, de);
		if (lstat(tmp, &data->sb) < 0 || !S_ISDIR(data->sb.st_mode))
			continue;
		ofl_taskfd(mnt, tmp, data);
	}
	HXdir_close(dir);
}

/**
 * ofl - filesystem use checker
 * @mnt:	mountpoint to search for
 * @action:	action to take
 */
static bool ofl(const char *mnt, unsigned int signum)
{
	struct ofl_compound data = {.signal = signum};
	const char *de;
	char tmp[256];
	void *dir;

	dir = HXdir_open("/proc");
	if (dir == NULL)
		return false;
	while ((de = HXdir_read(dir)) != NULL) {
		if (*de == '.')
			continue;
		data.pid = strtoul(de, NULL, 0);
		if (data.pid == 0)
			continue;
		snprintf(tmp, sizeof(tmp), "/proc/%s", de);
		if (lstat(tmp, &data.sb) < 0 || !S_ISDIR(data.sb.st_mode))
			continue;

		/* Program map */
		snprintf(tmp, sizeof(tmp), "/proc/%s/maps", de);
		if (ofl_pmap(mnt, tmp, &data))
			continue;

		/* Basic links */
		data.check = true;
		snprintf(tmp, sizeof(tmp), "/proc/%s/root", de);
		if (ofl_one(mnt, tmp, &data))
			continue;
		snprintf(tmp, sizeof(tmp), "/proc/%s/cwd", de);
		if (ofl_one(mnt, tmp, &data))
			continue;
		snprintf(tmp, sizeof(tmp), "/proc/%s/exe", de);
		if (ofl_one(mnt, tmp, &data))
			continue;

		/* All file descriptors */
		data.check = false;
		snprintf(tmp, sizeof(tmp), "/proc/%s/task", de);
		ofl_task(mnt, tmp, &data);
	}

	return data.found;
}

static unsigned int parse_signal(const char *str)
{
	static const char *signames[] = {
		[SIGHUP]  = "HUP",	[SIGINT]  = "INT",
		[SIGQUIT] = "QUIT",	[SIGKILL] = "KILL",
		[SIGTERM] = "TERM",	[SIGALRM] = "ALRM",
		[SIGPIPE] = "PIPE",
	};
	unsigned int ret;
	char *end;

	if (HX_isdigit(*str)) {
		ret = strtoul(str, &end, 10);
		if (*end == '\0')
			return ret;
	}

	for (ret = 0; ret < ARRAY_SIZE(signames); ++ret)
		if (signames[ret] != NULL && strcmp(str, signames[ret]) == 0)
			return ret;
	return 0;
}

int main(int argc, const char **argv)
{
	unsigned int signum = 0;
	char *signum_str = NULL;
	struct HXoption options_table[] = {
		{.sh = 'P', .type = HXTYPE_NONE, .ptr = &pids_only,
		 .help = "Show only PIDs"},
		{.sh = 'k', .type = HXTYPE_STRING, .ptr = &signum_str,
		 .help = "Signal to send (if any)", .htyp = "NUM/NAME"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	int ret;

	if ((ret = HX_init()) <= 0) {
		fprintf(stderr, "HX_init: %s\n", strerror(-ret));
		abort();
	}
	if (HX_getopt(options_table, &argc, &argv, HXOPT_USAGEONERR) < 0)
		goto out;
	if (argc == 1) {
		fprintf(stderr, "You need to supply at least a path\n");
		goto out;
	}

	if (signum_str != NULL)
		signum = parse_signal(signum_str);
	ret = false;
	while (*++argv != NULL)
		ret |= ofl(*argv, signum);

	if (pids_only)
		printf("\n");

	HX_exit();
	return ret ? EXIT_SUCCESS : EXIT_FAILURE;
 out:
	HX_exit();
	return EXIT_FAILURE + 1;
}
