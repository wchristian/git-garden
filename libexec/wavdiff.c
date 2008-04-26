#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/option.h>

struct fdstream {
	int fd;
	struct stat sb;
	void *area;
	unsigned long offset;
	union {
		const void *vptr;
		const int16_t *ptr;
	};
};

static struct fdstream gfile[3];
static unsigned int invert;
static unsigned long max = 0;
static unsigned int mono_mix;
static double volume = 1.0;

static bool wavdiff_get_options(int *argc, const char ***argv)
{
	struct HXoption options_table[] = {
		{.sh = 'a', .type = HXTYPE_ULONG, .ptr = &gfile[0].offset,
		 .help = "Offset in first file", .htyp = "BYTES"},
		{.sh = 'b', .type = HXTYPE_ULONG, .ptr = &gfile[1].offset,
		 .help = "Offset in second file", .htyp = "BYTES"},
		{.sh = 'i', .type = HXTYPE_NONE, .ptr = &invert,
		 .help = "Invert"},
		{.sh = 'm', .type = HXTYPE_ULONG, .ptr = &max,
		 .help = "maximum seconds"},
		{.sh = 'M', .type = HXTYPE_NONE, .ptr = &mono_mix,
		 .help = "mono mixer"},
		{.sh = 'q', .type = HXTYPE_DOUBLE, .ptr = &volume,
		 .help = "Volume multiplier"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) < 0)
		return false;
	max *= 44100*4;
	if (max == 0)
		max = -1;
	return true;
}

static bool wavdiff_open_streams(int argc, const char **argv,
    struct fdstream *file)
{
	if ((file[0].fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open %s failed: %s\n", argv[1], strerror(errno));
		return false;
	}
	if ((file[1].fd = open(argv[2], O_RDONLY)) < 0) {
		fprintf(stderr, "open %s failed: %s\n", argv[2], strerror(errno));
		return false;
	}
	if (argc == 3) {
		file[2].fd = STDOUT_FILENO;
	} else if ((file[2].fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "open %s failed: %s\n", argv[3], strerror(errno));
		return false;
	}

	if (fstat(file[0].fd, &file[0].sb) < 0 ||
	    fstat(file[1].fd, &file[1].sb) < 0) {
		perror("fstat");
		abort();
	}

	file[0].area = mmap(NULL, file[0].sb.st_size, PROT_READ, MAP_SHARED, file[0].fd, 0);
	file[1].area = mmap(NULL, file[1].sb.st_size, PROT_READ, MAP_SHARED, file[1].fd, 0);
	if (file[0].area == (void *)-1 || file[1].area == (void *)-1) {
		perror("mmap");
		abort();
	}

	file[0].ptr = file[0].area + file[0].offset;
	file[1].ptr = file[1].area + file[1].offset;
	return true;
}

static inline int16_t clamp16(int32_t x)
{
	if (x < -32768)
		return -32768;
	else if (x > 32767)
		return 32767;
	return x;
}

static void wavdiff_matrix_process(struct fdstream *file)
{
	unsigned long stream_max = file[0].sb.st_size - file[0].offset;
	unsigned long stream_count;
	int16_t *stream_buf;

	if (file[1].sb.st_size - file[1].offset < stream_max)
		stream_max = file[1].sb.st_size - file[1].offset;
	if (max < stream_max)
		stream_max = max;

	stream_max &= ~3;
	stream_buf = malloc(stream_max);
	if (stream_buf == NULL) {
		perror("malloc");
		abort();
	}

	stream_max /= sizeof(int16_t);
	if (mono_mix) {
		printf("Mono\n");
		while (stream_count < stream_max) {
			stream_buf[stream_count++] = *file[0].ptr;
			stream_buf[stream_count++] = *file[1].ptr;
			file[0].ptr += 2;
			file[1].ptr += 2;
			if (stream_count % 88200 == 0)
				fprintf(stderr, "\r\e[2K""%lu seconds", stream_count / 88200);
		}
	} else {
		while (stream_count < stream_max) {
			int16_t x = clamp16(*file[0].ptr - *file[1].ptr);
			x = (double)x * volume;
			stream_buf[stream_count++] = x;
			++file[0].ptr;
			++file[1].ptr;
			if (stream_count % 88200 == 0)
				fprintf(stderr, "\r\e[2K""%lu seconds", stream_count / 88200);
		}
	}
	stream_max *= sizeof(int16_t);

	write(file[2].fd, stream_buf, stream_max);
	free(stream_buf);
}

static void wavdiff_normal_process(struct fdstream *file)
{
	int16_t a_sample, b_sample;
	bool a_stop = false, b_stop = false;

	do {
		if (file[0].vptr < file[0].area + file[0].sb.st_size) {
			a_sample = *file[0].ptr++;
		} else {
			a_sample = 0;
			a_stop = true;
		}

		if (file[1].vptr < file[1].area + file[1].sb.st_size) {
			b_sample = *file[1].ptr++;
		} else {
			b_sample = 0;
			b_stop = true;
		}

		b_sample -= a_sample;
		write(file[2].fd, &b_sample, sizeof(b_sample));
	} while (!a_stop && !b_stop);
}

int main(int argc, const char **argv)
{
	if (!wavdiff_get_options(&argc, &argv))
		return EXIT_FAILURE;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: %s FILE1 FILE2 >FILE3\n", *argv);
		return EXIT_FAILURE;
	}

	if (!wavdiff_open_streams(argc, argv, gfile))
		return EXIT_FAILURE;
	wavdiff_matrix_process(gfile);
//	wavdiff_normal_process(gfile);
	munmap(gfile[0].area, gfile[0].sb.st_size);
	munmap(gfile[1].area, gfile[1].sb.st_size);
	close(gfile[0].fd);
	close(gfile[1].fd);
	close(gfile[2].fd);
	return EXIT_SUCCESS;
}
