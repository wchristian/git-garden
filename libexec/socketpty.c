/*
 *	socketpty - connect socket to pty master
 *	Jan Engelhardt <jengelh [at] medozas de>, 2008
 *
 *	(for making a VMware serial socket available as a pty)
 *	See http://jengelh.medozas.de/2008/0425-vmware-serial.php
 */
#define _XOPEN_SOURCE 1
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

int main(int argc, const char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s path-to-socket\n", *argv);
		return EXIT_FAILURE;
	}

	int sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		abort();
	}

	struct sockaddr_un sa = {};
	sa.sun_family = PF_UNIX;
	strncpy(sa.sun_path, argv[1], sizeof(sa.sun_path));

	if (connect(sockfd, (const void *)&sa, sizeof(sa)) < 0) {
		perror("connect");
		abort();
	}

	int ptmxfd = open("/dev/ptmx", O_RDWR);
	if (ptmxfd < 0) {
		perror("open /dev/ptmx");
		abort();
	}

	grantpt(ptmxfd);
	unlockpt(ptmxfd);
	printf("Slave is %s\n", ptsname(ptmxfd));

	struct pollfd pollreq[2] = {
		{.fd = ptmxfd, .events = POLLIN},
		{.fd = sockfd, .events = POLLIN},
	};

	char buf[4096];

	while (true) {
		if (poll(pollreq, ARRAY_SIZE(pollreq), 0) < 0) {
			if (errno == EINTR)
				break;
			continue;
		}
		if (pollreq[0].revents & POLLIN) {
			ssize_t rd = read(ptmxfd, buf, sizeof(buf));
			if (rd < 0 || write(sockfd, buf, rd) < 0)
				continue;
		}
		if (pollreq[1].revents & POLLIN) {
			ssize_t rd = read(sockfd, buf, sizeof(buf));
			if (rd < 0 || write(ptmxfd, buf, rd) < 0)
				continue;
		}
	}

	close(ptmxfd);
	close(sockfd);
	return EXIT_SUCCESS;
}
