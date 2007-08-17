/*
 *	pipe2pt - wrap pipe into a pty
 *	by Jan Engelhardt <jengelh [at] gmx de>, 2004 - 2007
 *	http://jengelh.hopto.org/
 *	released in the Public Domain
 */
#define _XOPEN_SOURCE 1
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char **argv)
{
	int master;
	pid_t pid;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s PROGRAM [ARGS...]\n", *argv);
		return EXIT_FAILURE;
	}

	if ((master = open("/dev/ptmx", O_RDWR | O_NOCTTY, 0600)) <= 0) {
		fprintf(stderr, "open(\"/dev/ptmx\"): %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	grantpt(master);
	unlockpt(master);

	if ((pid = fork()) < 0) {
		perror("fork()");
	} else if (pid == 0) {
		const char *sl_name = ptsname(master);
		int slave           = open(sl_name, O_RDONLY);

		dup2(slave, STDIN_FILENO);
		close(master);
		close(slave);
		++argv;
		return execvp(*argv, argv);
	} else {
		char buf[PIPE_BUF];
		ssize_t rd;

		while ((rd = read(STDIN_FILENO, buf, PIPE_BUF)) > 0)
			write(master, buf, rd);
		close(master);
		waitpid(pid, &status, 0);
	}

	return status;
}
