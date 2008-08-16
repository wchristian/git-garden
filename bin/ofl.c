/*
 *	Show processes using directories/files/mountpoints
 *	written by Jan Engelhardt, 2008
 *	Released in the Public Domain.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libHX.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

struct ofl_compound {
	struct stat sb;
	pid_t pid;
	unsigned char signal;
	bool check;
};

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
	ssize_t mnt_len, lnk_len;
	const char *p;
	char tmp[512];

	if (data->check)
		if (lstat(entry, &data->sb) < 0 || !S_ISLNK(data->sb.st_mode))
			return false;

	lnk_len = readlink(entry, tmp, sizeof(tmp) - 1);
	if (lnk_len < 0)
		return false;
	tmp[lnk_len] = '\0';

	/* Strip extra slashes at the end */
	mnt_len = strlen(mnt);
	for (p = mnt + mnt_len - 1; p >= mnt && *p == '/'; --p)
		--mnt_len;

	if (strncmp(tmp, mnt, mnt_len) != 0)
		return false;
	if (tmp[mnt_len] != '\0' && tmp[mnt_len] != '/')
		return false;

	if (data->signal == 0) {
		printf("%u: %s -> %s\n", data->pid, entry, tmp);
		return false; /* so that more FDs will be inspected */
	} else {
		if (kill(data->pid, data->signal) < 0) {
			if (errno == ESRCH)
				return true;
			return false;
		}
		return true;
	}
}

/**
 * ofl_fd - iterate through /proc/<process>/task/<task>/fd/
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
 * ofl_task - iterate through /proc/<process>/task/
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
		snprintf(tmp, sizeof(tmp), "%s/%s", path, de);
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
static void ofl(const char *mnt, unsigned int signum)
{
	struct ofl_compound data = {.signal = signum};
	const char *de;
	char tmp[256];
	void *dir;

	dir = HXdir_open("/proc");
	if (dir == NULL)
		return;
	while ((de = HXdir_read(dir)) != NULL) {
		if (*de == '.')
			continue;
		data.pid = strtoul(de, NULL, 0);
		if (data.pid == 0)
			continue;
		snprintf(tmp, sizeof(tmp), "/proc/%s", de);
		if (lstat(tmp, &data.sb) < 0 || !S_ISDIR(data.sb.st_mode))
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
}

static unsigned int parse_signal(const char *str)
{
	static const char *signames[] = {
		[SIGHUP] = "HUP",	[SIGINT] = "INT",
		[SIGQUIT] = "QUIT",	[SIGKILL] = "KILL",
		[SIGTERM] = "TERM",	[SIGALRM] = "ALRM",
		[SIGPIPE] = "PIPE",
	};
	unsigned int ret;
	char *end;

	if (isdigit(*str)) {
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
		{.sh = 'k', .type = HXTYPE_STRING, .ptr = &signum_str,
		 .help = "Signal to send (if any)", .htyp = "NUM/NAME"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, &argc, &argv, HXOPT_USAGEONERR) < 0)
		return EXIT_FAILURE;
	if (argc == 1) {
		fprintf(stderr, "You need to supply at least a path\n");
		return EXIT_FAILURE;
	}

	if (signum_str != NULL)
		signum = parse_signal(signum_str);
	while (*++argv != NULL)
		ofl(*argv, signum);
	return EXIT_SUCCESS;
}
