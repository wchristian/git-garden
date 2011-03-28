#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <libHX/defs.h>
#undef _POSIX_SOURCE
#include <sys/capability.h>

static const char *const cap_names[] = {
#define E(x) [CAP_##x] = #x
	E(CHOWN),
	E(DAC_OVERRIDE),
	E(DAC_READ_SEARCH),
	E(FOWNER),
	E(FSETID),
	E(KILL),
	E(SETGID),
	E(SETUID),
	E(SETPCAP),
	E(LINUX_IMMUTABLE),
	E(NET_BIND_SERVICE),
	E(NET_BROADCAST),
	E(NET_ADMIN),
	E(NET_RAW),
	E(IPC_LOCK),
	E(IPC_OWNER),
	E(SYS_MODULE),
	E(SYS_RAWIO),
	E(SYS_CHROOT),
	E(SYS_PTRACE),
	E(SYS_PACCT),
	E(SYS_ADMIN),
	E(SYS_BOOT),
	E(SYS_NICE),
	E(SYS_RESOURCE),
	E(SYS_TIME),
	E(SYS_TTY_CONFIG),
	E(MKNOD),
	E(LEASE),
	E(AUDIT_WRITE),
	E(AUDIT_CONTROL),
	E(SETFCAP),
	E(MAC_OVERRIDE),
	E(MAC_ADMIN),
#undef E
};

int main(void)
{
	cap_flag_value_t value;
	unsigned int i, j;
	cap_t data;

	data = cap_get_proc();
	printf("%-20s %s %s %s %-20s %s %s %s\n", "", "EFF", "PRM", "INH",
	                                          "", "EFF", "PRM", "INH");
	for (i = 0; i < (ARRAY_SIZE(cap_names) + 1) / 2; ++i) {
		j = i + ARRAY_SIZE(cap_names) / 2;

		printf("%-20s ", cap_names[i]);
		cap_get_flag(data, i, CAP_EFFECTIVE, &value);
		printf(" %s  ", (value == CAP_SET) ? "X" : ".");
		cap_get_flag(data, i, CAP_PERMITTED, &value);
		printf(" %s  ", (value == CAP_SET) ? "X" : ".");
		cap_get_flag(data, i, CAP_INHERITABLE, &value);
		printf(" %s  ", (value == CAP_SET) ? "X" : ".");

		if (j < ARRAY_SIZE(cap_names)) {
			printf("%-20s ", cap_names[j]);
			cap_get_flag(data, j, CAP_EFFECTIVE, &value);
			printf(" %s  ", (value == CAP_SET) ? "X" : ".");
			cap_get_flag(data, j, CAP_PERMITTED, &value);
			printf(" %s  ", (value == CAP_SET) ? "X" : ".");
			cap_get_flag(data, j, CAP_INHERITABLE, &value);
			printf(" %s", (value == CAP_SET) ? "X" : ".");
		}

		printf("\n");
	}

	cap_free(data);
	return EXIT_SUCCESS;
}
