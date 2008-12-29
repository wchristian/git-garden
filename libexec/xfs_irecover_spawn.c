/*
 *	Copyright © Jan Engelhardt, 2006 - 2008
 *
 *	This file is part of pam_mount; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public License
 *	as published by the Free Software Foundation; either version 2.1
 *	of the License, or (at your option) any later version.
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/defs.h>
#include <libHX/deque.h>
#include <libHX/misc.h>
#include <libHX/string.h>
#include "xfs_irecover_spawn.h"

/* Variables */
static struct sigaction saved_handler = {.sa_handler = SIG_DFL};

static inline int spawn_build_pipes(const int **fd_request, int (*p)[2])
{
	unsigned int x, y;
	for (x = 0; x < 3; ++x)
		for (y = 0; y < 2; ++y)
			p[x][y] = -1;

	if (fd_request[0] != NULL && pipe(p[0]) < 0)
		return -errno;
	if (fd_request[1] != NULL && pipe(p[1]) < 0)
		return -errno;
	if (fd_request[2] != NULL && pipe(p[2]) < 0)
		return -errno;
	return 1;
}

static void spawn_close_pipes(int (*p)[2])
{
	if (p[0][0] >= 0)
		close(p[0][0]);
	if (p[0][1] >= 0)
		close(p[0][1]);
	if (p[1][0] >= 0)
		close(p[1][0]);
	if (p[1][1] >= 0)
		close(p[1][1]);
	if (p[2][0] >= 0)
		close(p[2][0]);
	if (p[2][1] >= 0)
		close(p[2][1]);
}

static int spawn_set_sigchld(void)
{
	struct sigaction nh;

	if (saved_handler.sa_handler != SIG_DFL) {
		fprintf(stderr, "saved_handler already grabbed, not overwriting\n");
		return 0;
	}

	memset(&nh, 0, sizeof(nh));
	nh.sa_handler = SIG_DFL;
	sigemptyset(&nh.sa_mask);
	return sigaction(SIGCHLD, &nh, &saved_handler);
}

static bool __spawn_start(const char *const *argv, pid_t *pid, int *fd_stdin,
    int *fd_stdout, int *fd_stderr, void (*setup)(const char *),
    const char *user)
{
	const int *fd_rq[] = {fd_stdin, fd_stdout, fd_stderr};
	int pipes[3][2], ret, saved_errno;

	if ((ret = spawn_build_pipes(fd_rq, pipes)) < 0) {
		saved_errno = -ret;
		fprintf(stderr, "pipe(): %s\n", strerror(-ret));
		errno = ret;
		return false;
	}

	spawn_set_sigchld();
	if ((*pid = fork()) < 0) {
		saved_errno = errno;
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		spawn_close_pipes(pipes);
		errno = saved_errno;
		return false;
	} else if (*pid == 0) {
		if (setup != NULL)
			(*setup)(user);
		if (fd_stdin != NULL)
			dup2(pipes[0][0], STDIN_FILENO);
		if (fd_stdout != NULL)
			dup2(pipes[1][1], STDOUT_FILENO);
		if (fd_stderr != NULL)
			dup2(pipes[2][1], STDERR_FILENO);
		spawn_close_pipes(pipes);
		execvp(*argv, const_cast2(char * const *, argv));
		fprintf(stderr, "execvp: %s: %s\n", *argv, strerror(errno));
		_exit(-1);
	}
	
	if (fd_stdin != NULL) {
		*fd_stdin = pipes[0][1];
		close(pipes[0][0]);
	}
	if (fd_stdout != NULL) {
		*fd_stdout = pipes[1][0];
		close(pipes[1][1]);
	}
	if (fd_stderr != NULL) {
		*fd_stderr = pipes[2][0];
		close(pipes[2][1]);
	}

	return true;
}

bool spawn_startl(const char *const *argv, pid_t *pid, int *fd_stdin,
    int *fd_stdout)
{
	return __spawn_start(argv, pid, fd_stdin, fd_stdout, NULL, NULL, NULL);
}

