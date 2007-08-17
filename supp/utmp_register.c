/*
 *	utmp_register.c - (de)register in U/WTMP
 *	http://jengelh.hopto.org/
 *	Copyright © Jan Engelhardt <jengelh [at] gmx de>, 2004 - 2007
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2.1 of the License, or (at your option) any later version. 
 *	For details, see the file named "LICENSE.GPL2".
 */
#include <sys/time.h>
#include <paths.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>
#include <libHX.h>

/* Definitions */
#ifndef _PATH_UTMP
#	define _PATH_UTMP "/var/run/utmp"
#endif
#ifndef _PATH_WTMP
#	define _PATH_WTMP "/var/log/wtmp"
#endif

/* Variables */
static struct {
	char *host, *line, *user, *futmp, *fwtmp;
	long sess;
	int epid, pid, op_add, op_del, op_utmp, op_wtmp;
} Opt = {
	.futmp = _PATH_UTMP,
	.fwtmp = _PATH_WTMP,
};

static bool get_options(int *argc, const char ***argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'A', .type = HXTYPE_NONE, .ptr = &Opt.op_add,
		 .help = "Add an entry"},
		{.sh = 'D', .type = HXTYPE_NONE, .ptr = &Opt.op_del,
		 .help = "Delete an entry"},
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

int main(int argc, const char **argv)
{
	struct utmp entry = {};
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
			setutent();
			pututline(&entry);
		}
		if (Opt.op_wtmp)
			updwtmp(Opt.fwtmp, &entry);
	}

	entry.ut_type = DEAD_PROCESS;
	if (Opt.op_del) {
		if (Opt.op_utmp) {
			setutent();
			pututline(&entry);
		}
		if (Opt.op_wtmp)
			updwtmp(Opt.fwtmp, &entry);
	}

	endutent();
	return EXIT_SUCCESS;
}
