/*
 *	fd0ssh -
 *	hand stdin (fd 0) passwords to ssh via ssh-askpass mechanism
 *
 *	Copyright © CC Computer Consultants GmbH, 2008
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 or 3 of the License.
 */
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef __sun__
#	include <sys/termios.h>
#endif
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char zerossh_exchange_fd[] = "7";

static void zerossh_detach_tty(void)
{
	int fd;

	fd = open("/dev/tty", O_RDWR);
	if (fd < 0 && errno != ENXIO) {
		perror("open /dev/tty");
		abort();
	}
	ioctl(fd, TIOCNOTTY);
	close(fd);
}

static int zerossh_pipe_writer(const int *pipe_fd, const char *password)
{
	unsigned int pw_len = strlen(password);

	close(pipe_fd[0]);
	while (write(pipe_fd[1], password, pw_len) == pw_len)
		;

	return EXIT_SUCCESS;
}

static int zerossh_exec(const int *pipe_fd, const char **argv)
{
	if (dup2(pipe_fd[0], strtol(zerossh_exchange_fd, NULL, 0)) < 0) {
		perror("dup2");
		abort();
	}
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	zerossh_detach_tty();

	if (isatty(4)) {
		dup2(4, STDIN_FILENO);
		close(4);
	}

	return execvp(*argv, (char *const *)argv);
}

static int zerossh_setup(int argc, const char **argv)
{
	char password[256], *p;
	int pipe_fd[2], fd;
	pid_t pid;

	setenv("DISPLAY", "-:0", false);
	setenv("SSH_ASKPASS", *argv, true);
	setenv("SSH_ASKPASS_FD", zerossh_exchange_fd, true);

	if (fgets(password, sizeof(password)-1, stdin) == NULL)
		*password = '\0';
	p = password + strlen(password);
	*p++ = '\n';
	*p++ = '\0';
	fclose(stdin);

	/*
	 * STDIN_FILENO and STDERR_FILENO must be open, otherwise fuse/ssh
	 * and -- for some reason, the pipe writer -- feels very upset.
	 */
	fd = open("/dev/null", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/null");
		abort();
	}
	if (fd != STDIN_FILENO) {
		if (dup2(fd, STDIN_FILENO) < 0) {
			perror("dup");
			abort();
		}
		close(fd);
	}
	if (dup2(fd, STDERR_FILENO) < 0) {
		perror("dup");
		abort();
	}

	if (pipe(pipe_fd) < 0) {
		perror("pipe");
		abort();
	}

	/*
	 * Making the writer a subprocess makes for a very compact memory
	 * usage, allows to use no special signal setup, and even both
	 * interactive and non-interactive work as expected, that is, if
	 * mount.fuse detaches, so does the pipe writer with it.
	 */
	if ((pid = fork()) < 0) {
		perror("fork");
		abort();
	} else if (pid == 0) {
		return zerossh_pipe_writer(pipe_fd, password);
	}

	return zerossh_exec(pipe_fd, &argv[1]);
}

/*
 * zerossh_askpass - askpass part of the program
 * @in_fd:	inherited pipe (from zerossh_exec) to read password from
 * @out_fd:	pipe to the ssh parent process wanting our password
 */
static int zerossh_askpass(int in_fd, int out_fd)
{
	ssize_t ret;
	char *buf, *p;

	buf = malloc(4096);
	if (buf == NULL) {
		perror("malloc");
		abort();
	}

	ret = read(in_fd, buf, 4096);
	if (ret < 0) {
		perror("read");
		abort();
	}

	close(in_fd);
	p = memchr(buf, '\n', ret);
	/* ignore return values of write() */
	if (p == NULL)
		ret = write(out_fd, buf, ret);
	else
		ret = write(out_fd, buf, p - buf + 1);

	close(out_fd);
	return EXIT_SUCCESS;
}

int main(int argc, const char **argv)
{
	const char *s;

	if (**argv != '/' && strchr(argv[0], '/') != NULL)
		/*
		 * We either need an absolute path or something that is
		 * reachable through $PATH -- warn on everything else.
		 */
		fprintf(stderr, "You used a relative path -- ssh might not "
		        "locate the fd0ssh binary.\n");

	s = getenv("SSH_ASKPASS_FD");
	if (s != NULL)
		return zerossh_askpass(strtoul(s, NULL, 0), STDOUT_FILENO);

	if (argc == 1) {
		fprintf(stderr,
			"This program is not run from an interactive prompt, "
			"but rather from a script which utilizes it.\n"
			"Semantic call syntax:\n"
			"\t""echo $password | %s <program> [options...]\n",
		        *argv);
		return EXIT_FAILURE;
	}

	close(STDERR_FILENO);
	return zerossh_setup(argc, argv);
}
