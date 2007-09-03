#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define STACK_SIZE (1024 * 1024)

static int mexec(void *arg)
{
	printf("Starting %s\n", *(char **)arg);
	execvp(*(char **)arg, arg);
	_exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	void *stack;
	int ret;

	if ((stack = malloc(STACK_SIZE)) == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	if (argc == 1)
		argv = (char *[]){"-", "/bin/bash", NULL};

	ret = clone(mexec, stack + STACK_SIZE / 2,
	      CLONE_NEWNS | CLONE_THREAD | CLONE_SIGHAND | CLONE_VM, &argv[1]);
	if (ret < 0)
		perror("clone");
	sleep(6000);
	return !!ret;
}
