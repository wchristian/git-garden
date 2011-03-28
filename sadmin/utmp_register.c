/*
 *	utmp_register.c - (de)register in U/WTMP
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2004 - 2010
 *	http://jengelh.medozas.de/
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2.1 of the License, or (at your option) any later version. 
 *	For details, see the file named "LICENSE.GPL2".
 */
#define _GNU_SOURCE 1
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>
#include <libHX/init.h>
#include <libHX/option.h>
#include <libHX/string.h>
#include "config.h"
#ifdef HAVE_LASTLOG_H
#	include <lastlog.h>
#endif
#ifdef HAVE_PATHS_H
#	include <paths.h>
#endif

/* Definitions */
#ifndef _PATH_UTMP
#	define _PATH_UTMP "/var/run/utmp"
#endif
#ifndef _PATH_WTMP
#	define _PATH_WTMP "/var/log/wtmp"
#endif
#ifndef _PATH_LASTLOG
#	define _PATH_LASTLOG "/var/log/lastlog"
#endif

/* Variables */
static struct {
	char *host, *line, *user, *futmp, *fwtmp, *flastlog;
	long sess;
	unsigned int epid, pid, op_add, op_del, op_utmp, op_wtmp, op_lastlog;
} Opt = {
	.futmp    = _PATH_UTMP,
	.fwtmp    = _PATH_WTMP,
	.flastlog = _PATH_LASTLOG,
};

static inline unsigned int imax(unsigned int a, unsigned int b)
{
	return (a > b) ? a : b;
}

static bool get_options(int *argc, const char ***argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'A', .type = HXTYPE_NONE, .ptr = &Opt.op_add,
		 .help = "Add an entry"},
		{.sh = 'D', .type = HXTYPE_NONE, .ptr = &Opt.op_del,
		 .help = "Delete an entry"},
		{.sh = 'L', .type = HXTYPE_NONE, .ptr = &Opt.op_lastlog,
		 .help = "Perform operation on lastlog"},
		{.sh = 'U', .type = HXTYPE_NONE, .ptr = &Opt.op_utmp,
		 .help = "Perform operation on UTMP"},
		{.sh = 'W', .type = HXTYPE_NONE, .ptr = &Opt.op_wtmp,
		 .help = "Perform operation on WTMP"},
		{.sh = 'e', .type = HXTYPE_NONE, .ptr = &Opt.epid,
		 .help = "Encode PID into ut_id"},
		{.sh = 'h', .type = HXTYPE_STRING, .ptr = &Opt.host,
		 .help = "Host field", .htyp = "host"},
		{.sh = 'l', .type = HXTYPE_STRING, .ptr = &Opt.user,
		 .help = "User field", .htyp = "user"},
		{.sh = 't', .type = HXTYPE_STRING, .ptr = &Opt.line,
		 .help = "TTY field", .htyp = "tty"},
		{.sh = 'p', .type = HXTYPE_INT, .ptr = &Opt.pid,
		 .help = "Unique ID 1", .htyp = "pid"},
		{.sh = 's', .type = HXTYPE_LONG, .ptr = &Opt.sess,
		 .help = "Unique ID 2", .htyp = "sess"},
		{.sh = 'u', .type = HXTYPE_STRING, .ptr = &Opt.futmp,
		 .help = "Path to utmp"},
		{.sh = 'w', .type = HXTYPE_STRING, .ptr = &Opt.fwtmp,
		 .help = "Path to wtmp"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) <= 0)
		return false;

	if (Opt.user == NULL) {
		/* Want a login name */
		fprintf(stderr, "Error: You need to specify at least a user name\n");
		return false;
	}

	return true;
}

static void update_lastlog(const char *file, const struct utmpx *utmp)
{
#ifdef HAVE_LASTLOG_H
	struct lastlog entry = {};
	struct passwd *pwd;
	int fd;

	if ((pwd = getpwnam(utmp->ut_user)) == NULL)
		return;

	if ((fd = open(_PATH_LASTLOG, O_WRONLY)) < 0)
		return;
	if (lseek(fd, pwd->pw_uid * sizeof(struct lastlog), SEEK_SET) !=
	    pwd->pw_uid * sizeof(struct lastlog)) {
		close(fd);
		return;
	}

	entry.ll_time = utmp->ut_tv.tv_sec;
	strncpy(entry.ll_line, utmp->ut_line,
	        imax(sizeof(entry.ll_line), sizeof(utmp->ut_line)));
	strncpy(entry.ll_host, utmp->ut_host,
	        imax(sizeof(entry.ll_host), sizeof(utmp->ut_host)));
	write(fd, &entry, sizeof(entry));
	close(fd);
#endif
}

static int main2(int argc, const char **argv)
{
	struct utmpx entry = {0};
	struct timeval tv;

	if (!get_options(&argc, &argv))
		return EXIT_FAILURE;

	entry.ut_type = USER_PROCESS;
	entry.ut_pid  = (Opt.pid == 0) ? getppid() : Opt.pid;
	if (Opt.line != NULL)
		strncpy(entry.ut_line, Opt.line, sizeof(entry.ut_line));

	if (Opt.epid) {
		uint16_t p = entry.ut_pid;
		entry.ut_id[0] = 'P';
		entry.ut_id[1] = 'X';
		memcpy(&entry.ut_id[2], &p, 2);
	} else {
		size_t sz = strlen(Opt.line);
		char *str = Opt.line;
		if (sz > sizeof(entry.ut_id))
			str += sz - sizeof(entry.ut_id);
		strncpy(entry.ut_id, str, sizeof(entry.ut_id));
	}

	if (Opt.user != NULL)
		HX_strlcpy(entry.ut_user, Opt.user, sizeof(entry.ut_user));
	else
		HX_strlcpy(entry.ut_user, getpwuid(getuid())->pw_name,
		           sizeof(entry.ut_user));

	if (Opt.host != NULL)
		HX_strlcpy(entry.ut_host, Opt.host, sizeof(entry.ut_host));

	entry.ut_session = Opt.sess;
	gettimeofday(&tv, NULL);
	entry.ut_tv.tv_sec  = tv.tv_sec;
	entry.ut_tv.tv_usec = tv.tv_usec;

	if (Opt.op_add) {
		if (Opt.op_utmp) {
			setutxent();
			pututxline(&entry);
		}
		if (Opt.op_wtmp)
			updwtmpx(Opt.fwtmp, &entry);
		if (Opt.op_lastlog)
			update_lastlog(Opt.flastlog, &entry);
	}

	entry.ut_type = DEAD_PROCESS;
	if (Opt.op_del) {
		if (Opt.op_utmp) {
			setutxent();
			pututxline(&entry);
		}
		if (Opt.op_wtmp)
			updwtmpx(Opt.fwtmp, &entry);
	}

	endutent();
	return EXIT_SUCCESS;
}

int main(int argc, const char **argv)
{
	int ret;

	if ((ret = HX_init()) <= 0) {
		fprintf(stderr, "HX_init: %s\n", strerror(-ret));
		abort();
	}
	ret = main2(argc, argv);
	HX_exit();
	return ret;
}
