/*
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2009
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libHX/misc.h>
#include <libHX.h>
#include <pwd.h>

enum pstat_fields {
	STAT_PID, STAT_COMM, STAT_STATUS, STAT_PPID, STAT_PGID, STAT_SID,
	STAT_TTY_NR, STAT_TTY_PGRP, STAT_TASK_FLAGS, STAT_MINOR_FAULTS,
	STAT_CMINOR_FAULTS, STAT_MAJOR_FAULTS, STAT_CMAJOR_FAULTS, STAT_UTIME,
	STAT_STIME, STAT_CUTIME, STAT_CSTIME, STAT_PRIORITY, STAT_NICE,
	STAT_NTHREADS, STAT_START_TIME, STAT_VSIZE, STAT_RSS, STAT_RSSLIM,
	STAT_START_CODE, STAT_END_CODE, STAT_START_STACK, STAT_ESP, STAT_EIP,
	STAT_SIGPENDING, STAT_SIGBLOCKED, STAT_SIGIGNORED, STAT_SIGCAUGHT,
	STAT_WCHAN, STAT__NO_FIELD1, STAT__NO_FIELD2, STAT_EXIT_SIGNAL,
	STAT_CPU, STAT_RT_PRIO, STAT_POLICY, STAT_BLKIO_TICKS, STAT_GTIME,
	STAT_CGTIME, __STAT_NFIELDS,
};

struct kps_proc_data {
	struct HXlist_head anchor;
	struct HXlist_head children;

	hxmc_t *cmdline;
	int cmdlen;
	unsigned int pid, ppid, pgid, tgid, uid, sid, tty_pgrp;
	int prio, nice;
	unsigned int nthreads, cpu;
	int rtprio;
	unsigned int policy;
	bool sublevel;
};

static void kps_read_cmdline(struct kps_proc_data *p, int fd)
{
	ssize_t ret;
	char buf[200];

	ret = read(fd, buf, sizeof(buf));
	if (ret > 0) {
		p->cmdline = HXmc_meminit(buf, ret);
		for (--ret; ret >= 0; --ret) {
			if (p->cmdline[ret] == '\0')
				p->cmdline[ret] = ' ';
		}
	}
	close(fd);
}

static void kps_read_status(struct kps_proc_data *task, FILE *fp)
{
	hxmc_t *ln = NULL;
	char *ptr;

	while (HX_getl(&ln, fp) != NULL) {
		HX_chomp(ln);
		ptr = ln;
		while (!HX_isspace(*ptr))
			++ptr;
		while (HX_isspace(*ptr))
			++ptr;
		if (strncmp(ln, "Name:", 5) == 0) {
			if (task->cmdline == NULL) {
				task->cmdline = HXmc_strinit(ptr);
				HXmc_strpcat(&task->cmdline, "[");
				HXmc_strcat(&task->cmdline, "]");
			}
			for (; *ptr != '\0'; ++ptr) {
				if (*ptr == '\\')
					continue;
				++task->cmdlen;
			}
		} else if (strncmp(ln, "Tgid:", 5) == 0) {
			task->tgid = strtoul(ptr, NULL, 0);
		} else if (strncmp(ln, "Uid:", 4) == 0) {
			task->uid = strtoul(ptr, NULL, 0);
		}
	}
	HXmc_free(ln);
	fclose(fp);
}

static void kps_read_stat(struct kps_proc_data *p, FILE *fp)
{
	hxmc_t *ln = NULL;
	char *ptr = ln, *field[__STAT_NFIELDS+1];

	if (HX_getl(&ln, fp) == NULL)
		goto out;
	if ((ptr = strchr(ln, '(')) == NULL)
		goto out;
	for (; p->cmdlen > 0; --p->cmdlen)
		*ptr = 'X';

	HX_split5(ln, " ", ARRAY_SIZE(field), field);
	p->pid      = strtoul(field[STAT_PID], NULL, 0);
	p->ppid     = strtoul(field[STAT_PPID], NULL, 0);
	p->pgid     = strtoul(field[STAT_PGID], NULL, 0);
	p->sid      = strtoul(field[STAT_SID], NULL, 0);
	p->tty_pgrp = strtoul(field[STAT_TTY_PGRP], NULL, 0);
	p->prio     = strtoul(field[STAT_PRIORITY], NULL, 0);
	p->nice     = strtoul(field[STAT_NICE], NULL, 0);
	p->nthreads = strtoul(field[STAT_NTHREADS], NULL, 0);
	p->cpu      = strtoul(field[STAT_CPU], NULL, 0);
	p->rtprio   = strtoul(field[STAT_RT_PRIO], NULL, 0);
	p->policy   = strtoul(field[STAT_POLICY], NULL, 0);
 out:
	HXmc_free(ln);
	fclose(fp);
}

static struct kps_proc_data *kps_proc_read_one(unsigned int pid)
{
	struct kps_proc_data *task;
	char buf[64];
	FILE *fp;
	int fd;

	if ((task = calloc(1, sizeof(*task))) == NULL)
		abort();
	HXlist_init(&task->anchor);
	HXlist_init(&task->children);
	task->cmdlen = 0;

	snprintf(buf, sizeof(buf), "/proc/%u/cmdline", pid);
	if ((fd = open(buf, O_RDONLY)) >= 0)
		kps_read_cmdline(task, fd);

	snprintf(buf, sizeof(buf), "/proc/%u/status", pid);
	if ((fp = fopen(buf, "r")) != NULL)
		kps_read_status(task, fp);

	snprintf(buf, sizeof(buf), "/proc/%u/stat", pid);
	if ((fp = fopen(buf, "r")) != NULL)
		kps_read_stat(task, fp);

	return task;
}

static void kps_proc_read(struct HXmap *tree)
{
	const char *dentry;
	char buf[64];
	void *dproc, *dthr;

	if ((dproc = HXdir_open("/proc")) == NULL) {
		fprintf(stderr, "Could not open /proc: %s\n", strerror(errno));
		return;
	}

	while ((dentry = HXdir_read(dproc)) != NULL) {
		unsigned int tgid;
		char *end;

		tgid = strtoul(dentry, &end, 0);
		if (end == dentry || *end != '\0')
			continue;

		snprintf(buf, sizeof(buf), "/proc/%u/task", tgid);
		if ((dthr = HXdir_open(buf)) == NULL) {
			fprintf(stderr, "Could not open %s: %s\n", buf, strerror(errno));
			continue;
		}

		while ((dentry = HXdir_read(dthr)) != NULL) {
			struct kps_proc_data *parent = NULL, *task;
			unsigned int tid;

			tid = strtoul(dentry, &end, 0);
			if (end == dentry || *end != '\0')
				continue;

			task = kps_proc_read_one(tid);
			if (task == NULL)
				abort();

			if (task->pid != task->tgid)
				parent = HXmap_get(tree, reinterpret_cast(void *,
				         static_cast(long, task->tgid)));
			else if (task->ppid > 1)
				parent = HXmap_get(tree, reinterpret_cast(void *,
				         static_cast(long, task->ppid)));

			if (parent != NULL) {
				HXlist_add_tail(&parent->children, &task->anchor);
				task->sublevel = true;
			}

			HXmap_add(tree, reinterpret_cast(void *,
				static_cast(long, tid)), task);
		}
		HXdir_close(dthr);
	}
}

static void kps_proc_free(void *entry)
{
	struct kps_proc_data *e = entry;

	HXmc_free(e->cmdline);
	free(e);
}

static struct HXmap *kps_proc_init(void)
{
	static const struct HXmap_ops ops = {.d_free = kps_proc_free};
	struct HXmap *tree;

	tree = HXmap_init5(HXMAPT_ORDERED, 0,
	       &ops, 0, sizeof(struct kps_proc_data));
	if (tree == NULL)
		abort();
	kps_proc_read(tree);

	return tree;
}

static inline void kps_color_vine(void)
{
	static int b;
	if (b == 0)
		b = (rand() % 3) + 1;
	if (isatty(STDOUT_FILENO)) {
		if (b == 1)
			printf("\e[32m");
		else if (b == 2)
			printf("\e[33m");
		else if (b == 3)
			printf("\e[31m");
	}
}

static inline void kps_color_restore(void)
{
	if (isatty(STDOUT_FILENO))
		printf("\e[0m");
}

static inline void kps_graph_vine(void)
{
	printf(" │   ");
}

static inline void kps_graph_nothing(void)
{
	printf("     ");
}

static inline void kps_graph_tleaf(void)
{
	kps_color_vine();
	printf(" └─ ");
	kps_color_restore();
}

static inline void kps_graph_leaf(void)
{
	kps_color_vine();
	printf(" ├─ ");
	kps_color_restore();
}

static inline void kps_common_fields(const struct kps_proc_data *task)
{
	const struct passwd *e;

	if ((e = getpwuid(task->uid)) != NULL)
		printf("%-8.8s ", e->pw_name);
	else
		printf("%8u ", task->uid);
	printf("%5u ", task->pid);
}

static void kps_task_show(const struct kps_proc_data *parent,
    unsigned int depth, hxmc_t **bar_array)
{
	const struct kps_proc_data *task;
	unsigned int i;

	HXlist_for_each_entry(task, &parent->children, anchor) {
		kps_common_fields(task);
		if (depth != 0)
			kps_color_vine();
		for (i = 0; i < depth; ++i)
			if (HXbitmap_test(*bar_array, i))
				kps_graph_vine();
			else
				kps_graph_nothing();
		if (depth != 0)
			kps_color_restore();
		HXmc_setlen(bar_array, HXbitmap_size(*bar_array, depth + 1));
		if (task->anchor.next == &parent->children) {
			HXbitmap_clear(*bar_array, depth);
			kps_graph_tleaf();
			printf("%s\n", task->cmdline);
		} else {
			HXbitmap_set(*bar_array, depth);
			kps_graph_leaf();
			printf("%s\n", task->cmdline);
		}
		kps_task_show(task, depth + 1, bar_array);
	}
}

static void kps_tree_show(struct HXmap *tree)
{
	const struct HXmap_node *node;
	struct HXmap_trav *trav;
	hxmc_t *bar_array = HXmc_meminit(NULL, 64);

	trav = HXmap_travinit(tree, 0);
	if (trav == NULL)
		abort();

	printf("USER.... ...PID COMMAND\n");
	while ((node = HXmap_traverse(trav)) != NULL) {
		const struct kps_proc_data *task = node->data;

		if (task->sublevel)
			continue;
		kps_common_fields(task);
		printf("%s\n", task->cmdline);
		kps_task_show(task, 0, &bar_array);
	}
	HXmap_travfree(trav);
}

int main(int argc, const char **argv)
{
	struct HXmap *tree;

	tree = kps_proc_init();
	kps_tree_show(tree);
	HXmap_free(tree);

	return EXIT_SUCCESS;
}
