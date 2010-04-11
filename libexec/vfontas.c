/*
 *	vfontas.c - VGA font textfile assembler
 *	FNT and CPI font extraction/converter program.
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2005 - 2010
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2.1 of the License, or (at your option) any later version.
 *	For details, see the file named "LICENSE.GPL2".
 *
 */
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/defs.h>
#include <libHX/init.h>
#include <libHX/misc.h>
#include <libHX/option.h>
#include <libHX/string.h>
#define MMAP_NONE ((void *)-1)
#ifndef S_IRUGO
#	define S_IRUGO (S_IRUSR | S_IRGRP | S_IROTH)
#endif
#ifndef S_IWUGO
#	define S_IWUGO (S_IWUSR | S_IWGRP | S_IWOTH)
#endif

enum {
	CMD_NONE = 0,
	CMD_EMPTY,
	CMD_CREATE,
	CMD_EXTRACT,
	CMD_CONVERTCPI,
};

struct vg_font {
	unsigned int width, height, wh, whz;
	char glyph[0];
};

/* CPI: see http://www.seasip.info/DOS/CPI/cpi.html */
struct cpi_fontfile_header {
	uint8_t id0;
	char id[7], reserved[8];
	uint16_t pnum;
	uint8_t ptyp;
	uint32_t fih_offset;
} __attribute__((packed));

struct cpi_fontinfo_header {
	uint16_t num_codepages;
} __attribute__((packed));

struct cpi_cpentry_header {
	uint16_t cpeh_size;
	uint32_t next_cpeh_offset;
	uint16_t device_type;
	char device_name[8];
	uint16_t codepage;
	char reserved[6];
	uint32_t cpih_offset;
} __attribute__((packed));

struct cpi_cpinfo_header {
	uint16_t version;
	uint16_t num_fonts;
	uint16_t size;
} __attribute__((packed));

struct cpi_screenfont_header {
	uint8_t height, width, yaspect, xaspect;
	uint16_t num_chars;
} __attribute__((packed));

/* Variables */
static struct {
	char *file, *directory;
	int action;
} Opt;

static struct vg_font *vf_font_alloc(unsigned int w, unsigned int h)
{
	struct vg_font *font;

	if ((font = malloc(sizeof(*font) + w * h * 256)) == NULL)
		return NULL;

	font->width  = w;
	font->height = h;
	font->wh     = w * h / 8;
	font->whz    = font->wh * 256;
	return font;
}

static int vf_empty(const char *filename)
{
	char field[4096] = {};
	int fd;

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUGO | S_IWUGO);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for writing: %s\n",
		        filename, strerror(errno));
		return 0;
	}

	write(fd, field, 4096);
	close(fd);
	return 1;
}

static void vf_mem_to_text(const char *area, FILE *fp)
{
	unsigned int y;
	char c;
	int x;

	for (y = 0; y < 16; ++y) {
		for (x = 7; x >= 0; --x) {
			c = (area[y] & (1 << x)) ? '#' : '.';
			fprintf(fp, "%c%c", c, c);
		}
		fprintf(fp, "\n");
	}
	return;
}

static int vf_extract(const char *filename, const char *directory)
{
	unsigned int count;
	int in_fd, saved_errno, ret = 0;
	struct stat sb;
	char *mapping;
	FILE *out_fp;

	if (stat(directory, &sb) < 0)
		HX_mkdir(directory);
	if ((in_fd = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        filename, strerror(errno));
		return -errno;
	} else if (fstat(in_fd, &sb) < 0) {
		saved_errno = errno;
		fprintf(stderr, "Could not fstat(): %s\n", strerror(errno));
		close(in_fd);
		return -saved_errno;
	}

	mapping = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, in_fd, 0);
	if (mapping == MMAP_NONE) {
		saved_errno = errno;
		fprintf(stderr, "Could not mmap(): %s\n", strerror(errno));
		close(in_fd);
		return -saved_errno;
	}

	for (count = 0; count < 256; ++count) {
		char buf[512];

		snprintf(buf, sizeof(buf), "%s/%03d", directory, count);
		if ((out_fp = fopen(buf, "wb")) == NULL) {
			fprintf(stderr, "Could not open %s: %s\n",
			        buf, strerror(errno));
			continue;
		}

		vf_mem_to_text(&mapping[16*count], out_fp);
		fclose(out_fp);
		++ret;
	}

	munmap(mapping, sb.st_size);
	close(in_fd);
	return ret == 256;
}

static void vf_extract_cpi3(const void *_data,
    const struct cpi_cpinfo_header *cpih, const char *directory)
{
	hxmc_t *out_file;
	char buf[sizeof("-4294967296")];
	const struct cpi_screenfont_header *sfh =
		static_cast(const void *, cpih + 1);
	unsigned int i, length;
	int out_fd;

	for (i = 0; i < cpih->num_fonts; ++i) {
		HX_mkdir(directory);
		out_file = HXmc_strinit(directory);
		HXmc_strcat(&out_file, "/");
		snprintf(buf, sizeof(buf), "%ux%u.fnt",
			sfh->width, sfh->height);
		HXmc_strcat(&out_file, buf);
		printf("Writing to %s\n", out_file);

		if ((out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC,
		    S_IRUGO | S_IWUGO)) < 0) {
			fprintf(stderr, "%s: %s\n", out_file, strerror(errno));
			HXmc_free(out_file);
			continue;
		}

		length = sfh->width * sfh->height / 8 * sfh->num_chars;
		write(out_fd, static_cast(const void *, sfh + 1), length);
		close(out_fd);
		sfh = static_cast(const void *, sfh + 1) + length;
	}
}

static void vf_extract_cpi2(const void *_data, const char *directory)
{
	const struct cpi_fontfile_header *ffh = _data;
	const struct cpi_fontinfo_header *fih;
	const struct cpi_cpentry_header *cpeh;
	const struct cpi_cpinfo_header *cpih;
	unsigned int i;
	hxmc_t *out_dir;
	char buf[sizeof("-4294967296")];

	if (ffh->id0 != 0xFF || strncmp(ffh->id, "FONT    ", sizeof(ffh->id)) != 0)
		return;
	if (ffh->pnum != 1 || ffh->ptyp != 1)
		return;

	fih  = _data + ffh->fih_offset;
	cpeh = static_cast(const void *, fih + 1);

	for (i = 0; i < fih->num_codepages; ++i,
	     cpeh = _data + cpeh->next_cpeh_offset)
	{
		printf("CPEH #%u: Name: %.*s, Codepage: %u\n",
		       i, sizeof(cpeh->device_name), cpeh->device_name,
		       cpeh->codepage);

		if (cpeh->device_type != 1)
			/* non-screen */
			continue;

		cpih = _data + cpeh->cpih_offset;
		if (cpih->version != 1)
			continue;

		out_dir = HXmc_strinit(directory);
		HXmc_strcat(&out_dir, "/");
		*buf = '\0';
		HX_strlncat(buf, cpeh->device_name, sizeof(buf), sizeof(cpeh->device_name));
		HX_strrtrim(buf);
		HXmc_strcat(&out_dir, buf);
		HXmc_strcat(&out_dir, "/");
		snprintf(buf, sizeof(buf), "%u", cpeh->codepage);
		HXmc_strcat(&out_dir, buf);
		vf_extract_cpi3(_data, cpih, out_dir);
		HXmc_free(out_dir);
	}
}

static int vf_extract_cpi(const char *filename, const char *directory)
{
	struct stat sb;
	void *mapping;
	int in_fd, saved_errno;

	if ((in_fd = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        filename, strerror(errno));
		return -errno;
	} else if (fstat(in_fd, &sb) < 0) {
		saved_errno = errno;
		fprintf(stderr, "Could not fstat(): %s\n", strerror(errno));
		close(in_fd);
		return -saved_errno;
	}

	mapping = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, in_fd, 0);
	if (mapping == MMAP_NONE) {
		saved_errno = errno;
		fprintf(stderr, "Could not mmap(): %s\n", strerror(errno));
		close(in_fd);
		return -saved_errno;
	}

	vf_extract_cpi2(mapping, directory);
	munmap(mapping, sb.st_size);
	close(in_fd);
	return 1;
}

static void vf_text_to_mem(int fd, char *data)
{
	unsigned int y;
	char b;
	int x;

	for (y = 0; y < 16; ++y) {
		for (x = 7; x >= 0; --x) {
			read(fd, &b, 1);
			if (b == '#')
				data[y] |= 1 << x;
			read(fd, &b, 1);
		}
		read(fd, &b, 1);
	}

	close(fd);
	return;
}

static int vf_create(const char *filename, const char *directory)
{
	struct vg_font *font;
	unsigned int i = 0;
	int fd;

	font = vf_font_alloc(8, 16);
	if (font == NULL)
		return EXIT_FAILURE;

	for (i = 0; i < 256; ++i) {
		char buf[256];

		snprintf(buf, sizeof(buf), "%s/%03u", directory, i);
		if ((fd = open(buf, O_RDONLY)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        buf, strerror(errno));
			continue;
		}
		vf_text_to_mem(fd, &font->glyph[font->wh * i]);
		close(fd);
	}

	if ((fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT,
	    S_IRUGO | S_IWUGO)) < 0) {
		fprintf(stderr, "Could not write to %s: %s\n",
		        filename, strerror(errno));
		return EXIT_FAILURE;
	}

	write(fd, font->glyph, font->whz);
	close(fd);
	return EXIT_SUCCESS;
}

static bool vf_get_options(int *argc, const char ***argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'D', .type = HXTYPE_STRING, .ptr = &Opt.directory,
		 .help = "Directory to operate on", .htyp = "DIR"},
		{.sh = 'E', .type = HXTYPE_VAL, .val = CMD_EMPTY,
		 .ptr = &Opt.action, .help = "Create empty file"},
		{.sh = 'c', .ln = "create", .type = HXTYPE_VAL, .ptr = &Opt.action,
		 .val = CMD_CREATE, .help = "Create font file from directory"},
		{.sh = 'f', .type = HXTYPE_STRING, .ptr = &Opt.file,
		 .help = "File to operate on", .htyp = "FILE"},
		{.sh = 'x', .ln = "extract", .type = HXTYPE_VAL, .ptr = &Opt.action,
		 .val = CMD_EXTRACT, .help = "Extract font file to directory"},
		{.ln = "cpi", .type = HXTYPE_VAL, .ptr = &Opt.action,
		 .val = CMD_CONVERTCPI, .help = "Convert a CPI to FNT"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) <= 0)
		return false;
	if (Opt.action == CMD_NONE) {
		fprintf(stderr, "Missing command. Use `vfontas -?` for help.\n");
		return false;
	}
	if (Opt.action == CMD_EMPTY && Opt.file == NULL) {
		fprintf(stderr, "-E option requires -f option too\n");
		return false;
	}
	if ((Opt.action == CMD_CREATE || Opt.action == CMD_EXTRACT ||
	    Opt.action == CMD_CONVERTCPI) &&
	    (Opt.file == NULL || Opt.directory == NULL)) {
		fprintf(stderr, "-c and -x option require both "
		        "-D and -f options\n");
		return false;
	}
	return true;
}

static int main2(int argc, const char **argv)
{
	int ret = 0;

	if (!vf_get_options(&argc, &argv))
		return EXIT_FAILURE;

	switch (Opt.action) {
		case CMD_EMPTY:
			ret = vf_empty(Opt.file);
			break;
		case CMD_EXTRACT:
			ret = vf_extract(Opt.file, Opt.directory);
			break;
		case CMD_CONVERTCPI:
			ret = vf_extract_cpi(Opt.file, Opt.directory);
			break;
		case CMD_CREATE:
			ret = vf_create(Opt.file, Opt.directory);
			break;
	}

	return (ret > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
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
