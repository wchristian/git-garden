/*
 *	Bulk XFS Undelete
 *	Copyright C Jan Engelhardt <jengelh [at] medozas de>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 3
 *	of the License, or (at your option) any later version.
 *	For details, see the file named "LICENSE.GPL3".
 *
 *	The program extracts all inodes (with some bounding possible by
 *	means of specifying command line options).
 */
#ifndef _LARGEFILE_SOURCE
#	define _LARGEFILE_SOURCE 1
#endif
#ifndef _LARGE_FILES
#	define _LARGE_FILES 1
#endif
#ifndef _FILE_OFFSET_BITS
#	define _FILE_OFFSET_BITS 64
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libHX/ctype_helper.h>
#include <libHX/defs.h>
#include <libHX/misc.h>
#include <libHX/option.h>
#include <libHX/string.h>
#include <arpa/inet.h>
#include "xfs_irecover_spawn.h"

typedef int8_t __s8;
typedef uint8_t __u8;
typedef uint16_t __be16;
typedef uint32_t __be32;
typedef uint64_t __be64;

struct work_info {
	char *device;
	char *output_dir;
	unsigned long long start_inode, max_inodes, truncate_threshold;
	size_t size_cutoff;
	bool dry_run;

	/* generated */
	unsigned long long stop_inode, icount;
	size_t blocksize;
	char *buffer;
	unsigned int buffer_size, inode_size;
	int xdb_read, xdb_write, blkfd;
	pid_t xdb_pid;
	time_t stat_timestamp;
	unsigned int stat_recovered;
	unsigned long long stat_lastnode;
};

struct extent_entry {
	unsigned long long start_offset, start_block, block_count;
};

struct extent_info {
	unsigned int e_num;
	struct extent_entry e_data[0];
};

struct xfs_timestamp {
	__be32		t_sec;		/* timestamp seconds */
	__be32		t_nsec;		/* timestamp nanoseconds */
};

struct xfs_dinode_core {
	__be16		di_magic;	/* inode magic # = XFS_DINODE_MAGIC */
	__be16		di_mode;	/* mode and type of file */
	__u8		di_version;	/* inode version */
	__u8		di_format;	/* format of di_c data */
	__be16		di_onlink;	/* old number of links to file */
	__be32		di_uid;		/* owner's user id */
	__be32		di_gid;		/* owner's group id */
	__be32		di_nlink;	/* number of links to file */
	__be16		di_projid;	/* owner's project id */
	__u8		di_pad[8];	/* unused, zeroed space */
	__be16		di_flushiter;	/* incremented on flush */
	struct xfs_timestamp	di_atime;	/* time last accessed */
	struct xfs_timestamp	di_mtime;	/* time last modified */
	struct xfs_timestamp	di_ctime;	/* time created/inode modified */
	__be64		di_size;	/* number of bytes in file */
	__be64		di_nblocks;	/* # of direct & btree blocks used */
	__be32		di_extsize;	/* basic/minimum extent size for file */
	__be32		di_nextents;	/* number of extents in data fork */
	__be16		di_anextents;	/* number of extents in attribute fork*/
	__u8		di_forkoff;	/* attr fork offs, <<3 for 64b align */
	__s8		di_aformat;	/* format of attr fork's data */
	__be32		di_dmevmask;	/* DMIG event mask */
	__be16		di_dmstate;	/* DMIG state info */
	__be16		di_flags;	/* random flags, XFS_DIFLAG_... */
	__be32		di_gen;		/* generation number */
};

static inline void ir_fdwait(const struct work_info *wi)
{
	struct pollfd poll_data = {
		.fd = wi->xdb_read,
		.events = POLLIN,
	};

	poll(&poll_data, 1, -1);
}

static char *ir_command(const struct work_info *wi, const char *cmd)
{
	ssize_t ret, have_read = 0;
	int amount;

	if (strchr(cmd, '\n') == NULL) {
		fprintf(stderr, "Command must contain a newline: \"%s\"\n", cmd);
		abort();
	}
	write(wi->xdb_write, cmd, strlen(cmd));

	*wi->buffer = '\0';
	do {
		ir_fdwait(wi);
		ioctl(wi->xdb_read, FIONREAD, &amount);
		ret = read(wi->xdb_read, wi->buffer + have_read, amount);
		if (ret < 0) {
			perror("read");
			abort();
		} else if (ret == 0) {
			continue;
		}
		have_read += ret;
		wi->buffer[have_read] = '\0';
		if (strstr(wi->buffer, "xfs_db> ") != NULL)
			break;
	} while (true);

	return wi->buffer;
}

static inline uint64_t ir_ntoh64(uint64_t x)
{
#ifdef WORDS_BIGENDIAN
	return x;
#else
	uint32_t lo = x, hi = x >> 32;
	return (static_cast(uint64_t, htonl(lo)) << 32) | htonl(hi);
#endif
}

static void ir_read_dinode(const struct work_info *wi, size_t pos,
    struct xfs_dinode_core *i)
{
	lseek(wi->blkfd, pos, SEEK_SET);
	read(wi->blkfd, i, sizeof(*i));
	i->di_magic = ntohs(i->di_magic);
	i->di_mode  = ntohs(i->di_mode);
	i->di_size  = ir_ntoh64(i->di_size);
}

static struct extent_info *ir_extent_list2(char **data, int num)
{
	struct extent_info *ext_list;
	struct extent_entry *ext;
	unsigned int ext_count = 0;
	char *w;
	int i;

	ext_list = malloc(sizeof(struct extent_info) +
	           sizeof(struct extent_entry) * num);
	if (ext_list == NULL) {
		perror("malloc");
		abort();
	}

	ext_list->e_num = num;
	for (i = 0; i < num; ++i) {
		/* 0:[0,54525968,2077652,0] */
		w = data[i];
		if (!HX_isdigit(*w))
			continue;
		while (HX_isdigit(*w))
			++w;
		if (*w++ != ':')
			continue;
		if (*w++ != '[') /* ] */
			continue;

		ext = &ext_list->e_data[ext_count];
		ext->start_offset = strtoll(w, &w, 0);
		if (*w++ != ',')
			continue;
		ext->start_block = strtoll(w, &w, 0);
		if (*w++ != ',')
			continue;
		ext->block_count = strtoll(w, &w, 0);
		if (*w != ',')
			continue;
		++ext_count;
	}

	memset(&ext_list[ext_count], 0, sizeof(*ext_list));
	HX_zvecfree(data);
	return ext_list;
}

static struct extent_info *ir_extent_list(struct work_info *wi,
    unsigned long long inum)
{
	char buf[64], *ret, *wp, **lines, **exts;
	int num = 0, i;

	snprintf(buf, sizeof(buf), "inode %llu\n", inum);
	ir_command(wi, buf);
	ir_command(wi, "type inode\n");
	ret = ir_command(wi, "print\n");
	lines = HX_split(ret, "\n", &num, 0);

	for (i = 0; i < num; ++i)
		if (strncmp(lines[i], "u.bmx[", strlen("u.bmx[")) == 0) /* ]] */
			break;

	if (i == num)
		/* no extents */
		return NULL;

	wp = lines[i];
	while (!HX_isspace(*wp))
		++wp;
	while (HX_isspace(*wp))
		++wp;
	if (*wp++ != '=')
		return NULL;
	while (HX_isspace(*wp))
		++wp;
	if (*wp++ != '[') /* ] */
		return NULL;
	while (*wp != ']')
		if (*wp++ == '\0')
			return NULL;
	++wp;
	while (HX_isspace(*wp))
		++wp;

	num  = 0;
	exts = HX_split(wp, " ", &num, 0);
	HX_zvecfree(lines);
	return ir_extent_list2(exts, num);
}

static void ir_xfer(int ofd, int ifd, size_t z, const struct work_info *wi)
{
	unsigned int segment;
	ssize_t ret;

	while (z > 0) {
		segment = (z < wi->buffer_size) ? z : wi->buffer_size;
		ret = read(ifd, wi->buffer, segment);
		if (ret < 0) {
			perror("read");
			break;
		} else if (ret == 0) {
			break;
		}
		write(ofd, wi->buffer, ret);
		z -= ret;
	}
}

static void ir_copy(const struct work_info *wi, unsigned long long inum,
    const struct xfs_dinode_core *inode, const struct extent_info *ext_info)
{
	const struct extent_entry *ext;
	struct stat sb;
	unsigned int i;
	char buf[256];
	int fd;

	snprintf(buf, sizeof(buf), "%s/%llu", wi->output_dir, inum);
	if ((fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC,
	    inode->di_mode)) < 0) {
		fprintf(stderr, "open: %s: %s\n", buf, strerror(errno));
		return;
	}

	for (i = 0; i < ext_info->e_num; ++i) {
		ext = &ext_info->e_data[i];
		lseek(wi->blkfd, wi->blocksize * ext->start_block, SEEK_SET);
		lseek(fd, wi->blocksize * ext->start_offset, SEEK_SET);
		ir_xfer(fd, wi->blkfd, wi->blocksize * ext->block_count, wi);
	}

	if (fstat(fd, &sb) == 0 && (sb.st_size > wi->truncate_threshold ||
	    inode->di_size > wi->truncate_threshold) &&
	    inode->di_size < sb.st_size)
		ftruncate(fd, inode->di_size);
	close(fd);
}

static inline void ir_stats(unsigned long long inum, struct work_info *wi)
{
	time_t now = time(NULL);
	unsigned long long delta_ino, rem_ino, rem_part;
	unsigned int rem_sec, rem_min, rem_hour;

	if (now <= wi->stat_timestamp + 3)
		return;

	delta_ino = inum - wi->stat_lastnode;
	if (delta_ino < 256)
		return;
	rem_ino   = wi->stop_inode - inum;
	rem_part  = rem_ino / delta_ino;
	rem_sec   = rem_part % 60;
	rem_min   = rem_part / 60 % 60;
	rem_hour  = rem_part / 3600;

	printf("\rino %llu/%llu (%.2f%%) %llu/s ETA %uh:%02um:%02us recov %u\e[K", /* ] */
	       inum, wi->stop_inode,
	       static_cast(double, inum * 100) / wi->stop_inode,
	       delta_ino, rem_hour, rem_min, rem_sec, wi->stat_recovered);
	wi->stat_lastnode = inum;
	wi->stat_timestamp = now;
}

static void ir_extract(struct work_info *wi)
{
	unsigned long long inum;
	struct xfs_dinode_core inode;
	struct extent_info *ext_list;
	size_t ino_pos;

	for (inum = wi->start_inode, ino_pos = inum * wi->inode_size;
	     inum < wi->stop_inode; ++inum, ino_pos += wi->inode_size)
	{
		ir_stats(inum, wi);
		ir_read_dinode(wi, ino_pos, &inode);
		if (inode.di_magic != 0x494E || inode.di_size == 0 ||
		    !S_ISREG(inode.di_mode)) {
			fflush(stdout);
			continue;
		}

		/* Skip extentless files before even trying to ask. */
		ext_list = ir_extent_list(wi, inum);
		if (ext_list == NULL) {
			fflush(stdout);
			continue;
		}

		if (inode.di_size >= wi->size_cutoff) {
			printf("\n" "ino %llu is pretty large (size %Zu MB), "
			       "skipping.\n", inum,
			       static_cast(size_t, inode.di_size) >> 20);
		} else if (!wi->dry_run) {
			ir_copy(wi, inum, &inode, ext_list);
			++wi->stat_recovered;
		}

		free(ext_list);
	}
}

static bool ir_get_devinfo(struct work_info *work_info)
{
	const char *const args[] = {"xfs_db", work_info->device, NULL};
	unsigned long long di_dblocks = 0, di_blocksize = 0, di_inodesize = 0;
	int num_lines = 0, i;
	char *ret, **lines;

	if (!spawn_startl(args, &work_info->xdb_pid, &work_info->xdb_write,
	    &work_info->xdb_read)) {
		fprintf(stderr, "Error starting xfs_db: %s\n",
		        strerror(errno));
		return false;
	}

	/* munge prompt */
	ir_fdwait(work_info);
	i = read(work_info->xdb_read, work_info->buffer,
	    strlen("xfs_db> "));
	if (i != strlen("xfs_db> ")) {
		fprintf(stderr, "Crap: %s\n", strerror(errno));
		return false;
	}

	ir_command(work_info, "inode 0\n");
	ir_command(work_info, "type sb\n");
	ret = ir_command(work_info, "print\n");
	lines = HX_split(ret, "\n", &num_lines, 0);
	if (lines == NULL) {
		fprintf(stderr, "HX_split: %s\n", strerror(errno));
		return false;
	}
	for (i = 0; i < num_lines; ++i) {
		const char *key = lines[i];
		char *value = lines[i];

		while (!HX_isspace(*value))
			++value;
		*value++ = '\0';
		while (HX_isspace(*value))
			++value;
		if (*value++ != '=')
			continue;
		while (HX_isspace(*value))
			++value;

		if (strcmp(key, "dblocks") == 0)
			di_dblocks = strtoull(value, NULL, 0);
		else if (strcmp(key, "blocksize") == 0)
			di_blocksize = strtoull(value, NULL, 0);
		else if (strcmp(key, "inodesize") == 0)
			di_inodesize = strtoull(value, NULL, 0);
	}
	HX_zvecfree(lines);

	if (di_dblocks == 0 || di_blocksize == 0 || di_inodesize == 0) {
		fprintf(stderr, "Insufficient SB data\n");
		return false;
	}

	work_info->blocksize  = di_blocksize;
	work_info->inode_size = di_inodesize;
	work_info->icount     = di_dblocks * di_blocksize / di_inodesize;
	return true;
}

static bool ir_get_options(int *argc, const char ***argv,
    struct work_info *work_info)
{
	struct stat sb;
	struct HXoption options_table[] = {
		{.sh = 'D', .type = HXTYPE_STRING, .ptr = &work_info->device,
		 .help = "Device name", .htyp = "dev"},
		{.sh = 'N', .type = HXTYPE_ULLONG,
		 .ptr = &work_info->max_inodes,
		 .help = "Maximum number of inodes to scan"},
		{.sh = 'n', .type = HXTYPE_NONE, .ptr = &work_info->dry_run,
		 .help = "Dry run"},
		{.sh = 'o', .type = HXTYPE_STRING,
		 .ptr = &work_info->output_dir, .htyp = "dir",
		 .help = "Output directory"},
		{.sh = 'r', .type = HXTYPE_ULLONG,
		 .ptr = &work_info->start_inode,
		 .help = "Start at the specified inode"},
		{.sh = 's', .type = HXTYPE_ULLONG,
		 .ptr = &work_info->size_cutoff, .htyp = "size",
		 .help = "Do not extract files larger than size without asking"},
		{.sh = 't', .type = HXTYPE_ULLONG,
		 .ptr = &work_info->truncate_threshold, .htyp = "size",
		 .help = "Files less than the size are not subject to truncation"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) <= 0)
		return false;

	if (work_info->device == NULL) {
		fprintf(stderr, "%s: -D option is required\n",
		        HX_basename(**argv));
		return false;
	}
	if (work_info->output_dir == NULL) {
		fprintf(stderr, "%s: -o option is required\n",
		        HX_basename(**argv));
		return false;
	}

	if (stat(work_info->output_dir, &sb) < 0 ||
	    !S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "Output dir non-existant or something wrong.\n");
		return false;
	}

	work_info->buffer_size = 1024 << 10;
	if ((work_info->buffer = malloc(work_info->buffer_size)) == NULL) {
		fprintf(stderr, "malloc: %s\n", strerror(errno));
		return false;
	}

	work_info->blkfd = open(work_info->device, O_RDONLY);
	if (work_info->blkfd < 0) {
		fprintf(stderr, "open: %s: %s\n",
		        work_info->device, strerror(errno));
		return false;
	}

	if (!ir_get_devinfo(work_info))
		return false;

	if (work_info->max_inodes == 0) {
		fprintf(stderr, "Approximately %llu inodes\n",
		        work_info->icount);
		work_info->max_inodes = work_info->icount;
	}

	work_info->stop_inode = work_info->start_inode + work_info->max_inodes;
	return true;
}

int main(int argc, const char **argv)
{
	struct work_info work_info;

	memset(&work_info, 0, sizeof(work_info));
	work_info.size_cutoff = 1024 << 20; /* 1 GB */

	if (!ir_get_options(&argc, &argv, &work_info))
		return EXIT_FAILURE;

	ir_extract(&work_info);

	close(work_info.blkfd);
	close(work_info.xdb_write);
	close(work_info.xdb_read);
	free(work_info.buffer);
	free(work_info.device);
	free(work_info.output_dir);
	if (waitpid(work_info.xdb_pid, NULL, WNOHANG) < 0 &&
	    errno == EAGAIN)
		kill(work_info.xdb_pid, SIGTERM);
	waitpid(work_info.xdb_pid, NULL, 0);
	return EXIT_SUCCESS;
}
