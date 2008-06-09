/*
 *	octl.c - Interface to OSS-API volume, recording and playback
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2005 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libHX.h>
#define VERBOSITY_MIN 0
#define VERBOSITY_MAX 4
#define VERBOSITY_DEFAULT 3
#define MAXFNLEN 256
#define DEVDSP "/dev/dsp"
#define DEVMIX "/dev/mixer"

/* Functions */
static void mixer(int, const char **);
static int mixer_inst_dev(char *, const char *, size_t);
static void mixer_proc(const char *, const char *);
static int mixer_read_recsrc(int, int);
static int mixer_write_recsrc(int, const char *, const char *);
static void mixer_proc_ctl(int, const char *, const char *);
static int mixer_display_all(int);
static void play(int, const char **);
static int playrec_getopt(int *, const char ***, struct HXdeque *);
static void getopt_op_K(const struct HXoptcb *);
static void getopt_op_kjump(const struct HXoptcb *);
static void playrec_setopt(int);
static void record(int, const char **);

/* Variables */
const static char *mixer_ctlname[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
static char cdev[MAXFNLEN], *cdevp = cdev;
static struct HXdeque *dv = NULL;

static struct {
	int blksize, channels, fragsize, smprate, smpsize, verbose, jwarn;
	long timelimit, bytelimit;
	long long seekto;
	char *buf;
} pri = {
	.blksize   = 8192,
	.channels  = 2,
	.fragsize  = 0,
	.seekto    = 0,
	.smprate   = 44100,
	.smpsize   = 16,
	.timelimit = 0,
	/* verbosity levels: 
	   0 nothing
	   1 filename
	   2 filename and dots
	   3 filename and times
	   4 filename and times and byte positions
	*/
	.verbose = VERBOSITY_DEFAULT,
};

//-----------------------------------------------------------------------------
int main(int argc, const char **argv)
{
	char *pp = HX_basename(*argv);

	if (strstr(pp, "mixer") != NULL) {
		mixer(argc, argv);
	} else if (strstr(pp, "play") != NULL) {
		play(argc, argv);
	} else if (strstr(pp, "rec") != NULL) {
		record(argc, argv);
	} else if (argc < 2) {
		fprintf(stderr, "Syntax: %s { mixer | play | rec } [...]\n",
		        *argv);
		return EXIT_FAILURE;
	} else if (strstr(argv[1], "mixer") != NULL) {
		mixer(--argc, ++argv);
	} else if (strstr(argv[1], "play") != NULL) {
		play(--argc, ++argv);
	} else if (strstr(argv[1], "rec") != NULL) {
		record(--argc, ++argv);
	} else {
		fprintf(stderr, "%s: No such module \"%s\"\n",
		        argv[0], argv[1]);
	}
	return EXIT_SUCCESS;
}

static void mixer(int argc, const char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Syntax: %s [device] [control [volume]]\n",
		        *argv);
		exit(EXIT_FAILURE);
	}

	if (argc == 1) {
		mixer_proc(NULL, NULL);
	} else if (argc == 2) {
		if (mixer_inst_dev(cdevp, argv[1], MAXFNLEN)) {
			/* Show all controls of given device */
			mixer_proc(NULL, NULL);
		} else {
			/*
			 * Show given control of /dev/mixer (usually ptr to
			 * /dev/mixer0)
			 */
			HX_strlcpy(cdevp, DEVMIX, MAXFNLEN);
			fprintf(stderr, "Device: %s; ", cdevp);
			mixer_proc(argv[1], NULL);
		}
	} else if (argc == 3) {
		if (mixer_inst_dev(cdevp, argv[1], MAXFNLEN)) {
			/* Show given control of given device */
			mixer_proc(argv[2], NULL);
		} else {
			/* Set given volume of given control (of /dev/mixer) */
			HX_strlcpy(cdevp, DEVMIX, MAXFNLEN);
			fprintf(stderr, "Device: %s; ", cdevp);
			mixer_proc(argv[1], argv[2]);
		}
	} else if (argc == 4) {
		/* omixer =/dev/mixer1 vol 80 */
		mixer_inst_dev(cdevp, argv[1], MAXFNLEN - 1);
		mixer_proc(argv[2], argv[3]);
	}
	return;
}

static int mixer_inst_dev(char *ptr, const char *dev, size_t ln)
{
	memset(ptr, '\0', ln);
	if (*dev == ':') {
		HX_strlcpy(ptr, DEVMIX, ln);
		strncat(ptr, ++dev, ln - strlen(ptr));
		return 1;
	} else if (*dev == '=') {
		HX_strlcpy(ptr, ++dev, ln);
		return 1;
	}
	return 0;
}

static void mixer_proc(const char *ctl, const char *vol)
{
	int mixer_fd;
	if ((mixer_fd = open(cdevp, O_RDWR, 0600)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        cdevp, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (ctl != NULL && strcmp(ctl, "?r") == 0) {
		int recsrc;
		if (ioctl(mixer_fd, SOUND_MIXER_READ_RECSRC, &recsrc) < 0) {
			perror("SOUND_MIXER_READ_RECSRC");
			exit(EXIT_FAILURE);
		}
		mixer_read_recsrc(mixer_fd, recsrc);
	} else if (ctl != NULL && (strcmp(ctl, "+r") == 0 ||
	   strcmp(ctl, "-r") == 0)) {
		if (vol == NULL) {
			fprintf(stderr, "You forgot to specify a source\n");
			exit(EXIT_FAILURE);
		}
		mixer_read_recsrc(mixer_fd,
		                  mixer_write_recsrc(mixer_fd, ctl, vol));
	} else if (ctl != NULL) {
		mixer_proc_ctl(mixer_fd, ctl, vol);
	} else {
		mixer_display_all(mixer_fd);
	}

	close(mixer_fd);
	return;
}

static int mixer_read_recsrc(int mixer_fd, int recsrc)
{
	int i;
	printf("Recording source(s):");

	for (i = 0; i < SOUND_MIXER_NRDEVICES; ++i)
		if (recsrc & (1 << i))
			printf(" %s", mixer_ctlname[i]);

	printf("\n");
	return recsrc;
}

static int mixer_write_recsrc(int mixer_fd, const char *ctl, const char *vol)
{
	int i, recsrc;

	for (i = 0; i < SOUND_MIXER_NRDEVICES &&
	    strcmp(mixer_ctlname[i], vol) != 0; ++i)
		;

	if (i >= SOUND_MIXER_NRDEVICES) {
		fprintf(stderr, "Unknown recording source: %s\n", vol);
		exit(EXIT_FAILURE);
	}

	if (ioctl(mixer_fd, SOUND_MIXER_READ_RECSRC, &recsrc) < 0) {
		perror("SOUND_MIXER_READ_RECSRC");
		exit(EXIT_FAILURE);
	}

	if (*ctl == '-')
		recsrc &= ~(1 << i);
	else
		recsrc |= 1 << i;

	if (ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &recsrc) < 0) {
		perror("SOUND_MIXER_WRITE_RECSRC");
		exit(EXIT_FAILURE);
	}

	if (ioctl(mixer_fd, SOUND_MIXER_READ_RECSRC, &recsrc) < 0) {
		perror("SOUND_MIXER_READ_RECSRC");
		exit(EXIT_FAILURE);
	}

	return recsrc;
}

static void mixer_proc_ctl(int mixer_fd, const char *ctl, const char *vol)
{
	int arg, i, devmask;
	if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
		perror("SOUND_MIXER_READ_DEVMASK");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < SOUND_MIXER_NRDEVICES &&
	    strcmp(mixer_ctlname[i], ctl) != 0; ++i)
		;

	if (i >= SOUND_MIXER_NRDEVICES) {
		fprintf(stderr, "Invalid recording source: %s\n", ctl);
		exit(EXIT_FAILURE);
	}

	if (vol == NULL) {
		if (ioctl(mixer_fd, MIXER_READ(i), &arg) < 0) {
			if (errno == EINVAL)
				fprintf(stderr, "Invalid mixer control: %s\n",
				        mixer_ctlname[i]);
			else
				perror("MIXER_READ");
			exit(EXIT_FAILURE);
		}
		printf("%s: L=%d%% R=%d%%\n", mixer_ctlname[i],
		       arg & 0xFF, (arg >> 8) & 0xFF);
	} else if (*vol == '+' || *vol == '-') {
		int vol_l, vol_r;

		if (strchr(vol, ':') != NULL) {
			sscanf(vol, "%d:%d", &vol_l, &vol_r);
		} else {
			sscanf(vol, "%d", &vol_l);
			vol_r = vol_l;
		}

		if (ioctl(mixer_fd, MIXER_READ(i), &arg) < 0) {
			if (errno == EINVAL) {
				fprintf(stderr, "Invalid mixer control: %s\n",
				        mixer_ctlname[i]);
			} else {
				perror("MIXER_READ");
			}
			exit(EXIT_FAILURE);
		}

		if (strchr(vol, ':') != NULL) {
			sscanf(vol, "%d:%d", &vol_l, &vol_r);
		} else {
			sscanf(vol, "%d", &vol_l);
			vol_r = vol_l;
		}

		vol_l = (arg & 0xFF) + vol_l;
		vol_r = ((arg >> 8) & 0xFF) + vol_r;

		if (vol_l < 0)   vol_l = 0;
		if (vol_r < 0)   vol_r = 0;
		if (vol_l > 100) vol_l = 100;
		if (vol_r > 100) vol_r = 100;
		arg = vol_l | (vol_r << 8);

		if (ioctl(mixer_fd, MIXER_WRITE(i), &arg) < 0) {
			if (errno == EINVAL) {
				fprintf(stderr, "Invalid mixer control: %s\n",
				        mixer_ctlname[i]);
			} else {
				perror("MIXER_WRITE");
			}
			exit(EXIT_FAILURE);
		}
		printf("%s: L=%d%% R=%d%%\n", mixer_ctlname[i], vol_l, vol_r);
	} else {
		int vol_l, vol_r;

		if (strchr(vol, ':') != NULL) {
			sscanf(vol, "%d:%d", &vol_l, &vol_r);
		} else {
			sscanf(vol, "%d", &vol_l);
			vol_r = vol_l;
		}

		if (vol_l < 0)   vol_l = 0;
		if (vol_r < 0)   vol_r = 0;
		if (vol_l > 100) vol_l = 100;
		if (vol_r > 100) vol_r = 100;
		arg = vol_l | (vol_r << 8);

		if (ioctl(mixer_fd, MIXER_WRITE(i), &arg) < 0) {
			if (errno == EINVAL) {
				fprintf(stderr, "Invalid mixer control: %s\n",
				        mixer_ctlname[i]);
			} else {
				perror("MIXER_WRITE");
			}
			exit(EXIT_FAILURE);
		}
		printf("%s: L=%d%% R=%d%%\n", mixer_ctlname[i], vol_l, vol_r);
	}

	return;
}

static int mixer_display_all(int mixer_fd)
{
	int arg, i, devmask;
	if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
		perror("SOUND_MIXER_READ_DEVMASK");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < SOUND_MIXER_NRDEVICES; ++i) {
		if (devmask & (1 << i)) {
			if (ioctl(mixer_fd, MIXER_READ(i), &arg) < 0) {
				if (errno == EINVAL) {
					fprintf(stderr, "Invalid mixer control: %s\n",
					        mixer_ctlname[i]);
				} else {
					perror("MIXER_READ");
				}
				exit(EXIT_FAILURE);
			}

			printf("%-8s   L=%3d%%   R=%3d%%\n", mixer_ctlname[i],
			       arg & 0xFF, (arg >> 8) & 0xFF);
		}
	}
	return devmask;
}

static void sighandler(int s)
{
	struct HXdeque_node *travp = dv->first;

	if (s == SIGUSR1) /* lower speed */
		pri.smprate -= 1000;
	else if (s == SIGUSR2) /* raise speed */
		pri.smprate += 1000;

	while (travp != NULL) {
		playrec_setopt((long)travp->ptr);
		travp = travp->next;
	}
	return;
}

static void play(int argc, const char **argv)
{
	struct HXdeque_node *travp;

	dv = HXdeque_init();
	if (!playrec_getopt(&argc, &argv, dv))
		return;
	if (dv->items == 0)
		HXdeque_push(dv, DEVDSP);

	travp = dv->first;
	while (travp != NULL) {
		int dsp_fd;
		if ((dsp_fd = open(travp->ptr, O_WRONLY)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        cdevp, strerror(errno));
			exit(EXIT_FAILURE);
		}
		travp->ptr = (void *)(long)dsp_fd;
		playrec_setopt(dsp_fd);
		travp = travp->next;
	}

	{ /* Signal handlers for speed control */
		struct sigaction sa = {};
		sa.sa_handler = sighandler;
		sa.sa_flags   = SA_RESTART;
		sigaction(SIGUSR1, &sa, NULL);
		sigaction(SIGUSR2, &sa, NULL);
	}

	++argv;
	while (--argc > 0 && *argv != NULL) {
		unsigned long bytes_per_sec = pri.smprate * pri.channels * pri.smpsize / 8;
		unsigned long long rel_bytes = 0, rel_pos = 0;
		int run = 1, ffd;

		if (strcmp(*argv, "-") == 0) {
			ffd = STDIN_FILENO;
		} else if ((ffd = open(*argv, O_RDONLY)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        *argv, strerror(errno));
			++argv;
			continue;
		}

		lseek(ffd, pri.seekto, SEEK_SET);
		if (pri.verbose > 0)
			fprintf(stderr, "%s", *argv);

		while (run) {
			int have_read;
			if ((pri.timelimit != 0 &&
			    (signed long)rel_pos >= pri.timelimit) ||
			    (pri.bytelimit != 0 &&
			    (signed long)rel_bytes >= pri.bytelimit))
				break;

			have_read = read(ffd, pri.buf, pri.blksize);

			if (have_read == 0) {
				run = 0;
			} else if (have_read == -1) {
				fprintf(stderr, "\r%s: Error while reading "
				        "from file: %s\e[K\n",
				        *argv, strerror(errno));
				run = 0;
			}

			travp = dv->first;
			while (travp != NULL) {
				if (write((long)travp->ptr, pri.buf, have_read) < 0) {
					fprintf(stderr, "\r%s: Error while "
					        "writing to DSP: %s\e[K\n",
					        *argv, strerror(errno));
					run = 0;
				}
				travp = travp->next;
			}

			rel_bytes += have_read;
			rel_pos = rel_bytes / bytes_per_sec;
			switch (pri.verbose) {
				case 2:
					fprintf(stderr, ".");
					break;
				case 3: {
					unsigned long long abs_pos = (pri.seekto + rel_bytes) / bytes_per_sec;
					fprintf(stderr, "\r%s [%3lld:%02lld * %3lld:%02lld]",
					        *argv, abs_pos / 60, abs_pos % 60, rel_pos / 60,
					        rel_pos % 60);
					break;
				}
				case 4: {
					unsigned long long abs_bytes = pri.seekto + rel_bytes,
						abs_pos = abs_bytes / bytes_per_sec;
					fprintf(stderr, "\r%s [%3lld:%02lld * %3lld:%02lld; BP: %lldB * %lldB]",
					        *argv, abs_pos / 60, abs_pos % 60, rel_pos / 60, rel_pos % 60,
					        abs_bytes, rel_bytes);
					break;
				}
			}
		}
		if (pri.verbose > 0)
			fprintf(stderr, "\n");
		++argv;
	}

	travp = dv->first;
	while (travp != NULL) {
		close((long)travp->ptr);
		travp = travp->next;
	}
	HXdeque_free(dv);
	return;
}

static int playrec_getopt(int *argc, const char ***argv,
    struct HXdeque *devlist)
{
	unsigned long t_fragsize = 0;
	struct HXoption options_table[] = {
		{.sh = 'B', .type = HXTYPE_INT, .ptr = &pri.blksize,
		 .help = "Sets the block size (bytes to r/w at ocne)",
		 .htyp = "bytes"},
		{.sh = 'K', .type = HXTYPE_STRING, .cb = getopt_op_K,
		 .help = "Jump to position (bytes)", .htyp = "pos"},
		{.sh = 'M', .type = HXTYPE_VAL, .ptr = &pri.channels,
		 .val = 1, .help = "Set single-channel input/output"},
		{.sh = 'Q', .type = HXTYPE_VAL, .ptr = &pri.channels,
		 .val = 4, .help = "Sets 4 channel input/output"},
		{.sh = 'T', .type = HXTYPE_LONG, .ptr = &pri.bytelimit,
		 .help = "Maximum amount of bytes to record/play",
		 .htyp = "bytes"},
		{.sh = 'V', .type = HXTYPE_INT, .ptr = &pri.verbose,
		 .help = "Set verbosity level"},
		{.sh = 'b', .type = HXTYPE_INT, .ptr = &pri.smpsize,
		 .help = "Sets the sample size (8/16 bit)", .htyp = "bits"},
		{.sh = 'c', .type = HXTYPE_INT, .ptr = &pri.channels,
		 .help = "Sets the number of channels"},
		{.sh = 'd', .type = HXTYPE_STRDQ, .ptr = devlist,
		 .help = "Uses this DSP device (default: /dev/dsp)",
		 .htyp = "file"},
		{.sh = 'f', .type = HXTYPE_INT, .ptr = &t_fragsize,
		 .help = "Sets the fragment size", .htyp = "bytes"},
		{.sh = 'k', .type = HXTYPE_STRING, .cb = getopt_op_kjump,
		 .help = "Jump to position (minutes:seconds)", .htyp = "pos"},
		{.sh = 'q', .type = HXTYPE_NONE | HXOPT_DEC,
		 .ptr = &pri.verbose, .help = "Lower verbosity level"},
		{.sh = 's', .type = HXTYPE_INT, .ptr = &pri.smprate,
		 .help = "Set the samplerate (11025, 22050, 44100 Hz, etc.)",
		 .htyp = "bytes"},
		{.sh = 't', .type = HXTYPE_LONG, .ptr = &pri.timelimit,
		 .help = "Maximum amount of time to record/play",
		 .htyp = "seconds"},
		{.sh = 'v', .type = HXTYPE_NONE | HXOPT_INC,
		 .ptr = &pri.verbose, .help = "Raise verbosity level"},
		{.sh = 'w', .type = HXTYPE_NONE, .ptr = &pri.jwarn,
		 .help = "Do not exit upon ioctl error"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};

	if (HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) <= 0)
		return 0;

	t_fragsize &= ~0x7FFF0000;
	t_fragsize |= 0x7FFF0000;
	return 1;
}

static void getopt_op_K(const struct HXoptcb *cbi)
{
	pri.seekto = strtoll(cbi->data, NULL, 0);
	return;
}

static void getopt_op_kjump(const struct HXoptcb *cbi)
{
	char *p, *timespec;
	unsigned long s;

	if (cbi->data == NULL || strlen(cbi->data) == 0)
		return;

	timespec = HX_strdup(cbi->data);
	s = 0;

	if ((p = strchr(cbi->data, ':')) == NULL) {
		s = strtoul(cbi->data, NULL, 0);
	} else {
		*p++ = '\0';
		s = 60 * strtoul(cbi->data, NULL, 0) + strtoul(p, NULL, 0);
	}

	pri.seekto = s * pri.smpsize * pri.channels / 8 * pri.smprate;
	free(timespec);
	return;
}

static void playrec_setopt(int fd)
{
	int tmp = pri.fragsize;
	if ((tmp & 0xFFFF) != 0 && ioctl(fd, SNDCTL_DSP_SETFRAGMENT,
	    &pri.fragsize) < 0) {
		perror("ioctl(): Could not set fragment size");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}
	if (tmp != pri.fragsize)
		fprintf(stderr, "driver: Could not set fragment size %d, "
		        "falling back to %d\n", tmp, pri.fragsize);

	tmp = pri.smpsize;
	if (ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &pri.smpsize) < 0) {
		perror("ioctl(): Could not set sample size");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}
	if (tmp != pri.smpsize)
		fprintf(stderr, "driver: Could not set sample size %d, "
		        "falling back to %d\n", pri.smpsize, tmp);

	tmp = pri.channels;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &pri.channels) < 0) {
		perror("ioctl(): Could not set channel count");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}
	if (tmp != pri.channels)
		fprintf(stderr, "driver: Could not set %d channels, falling "
		        "back to %d\n", tmp, pri.channels);

	tmp = pri.smprate;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &pri.smprate) < 0) {
		perror("ioctl(): Could not set sample rate");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}
	if (tmp != pri.smprate)
		fprintf(stderr, "driver: Could not set sample rate %d, "
		        "faling back to %d\n", tmp, pri.smprate);

	if (pri.blksize == 0 && (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &pri.blksize) < 0 ||
	    pri.blksize < 0)) {
		perror("SNDCTL_DSP_GETBLKSIZE");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}

	if (pri.verbose > 0)
		fprintf(stderr, "Device parameters set: %d bit %d channels %d "
		        "Hz (Block size %d)\n",  pri.smpsize, pri.channels,
		        pri.smprate, pri.blksize);

	if ((pri.buf = malloc(pri.blksize)) == NULL) {
		perror("Could not allocate buffer");
		if (!pri.jwarn)
			exit(EXIT_FAILURE);
	}

	return;
}

static void record(int argc, const char **argv)
{
	int dsp_fd, ffd;

	dv = HXdeque_init();
	if (!playrec_getopt(&argc, &argv, dv))
		return;
	if (dv->items == 0)
		HXdeque_push(dv, DEVDSP);

	if ((dsp_fd = open(dv->first->ptr, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        (const char *)dv->first->ptr, strerror(errno));
		exit(EXIT_FAILURE);
	}

	++argv;
	playrec_setopt(dsp_fd);
	if (strcmp(*argv, "-") == 0) {
		ffd = STDOUT_FILENO;
	} else if (pri.seekto != 0) {
		if ((ffd = open(*argv, O_RDWR | O_APPEND | O_CREAT,
		    S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) < 0) {
			fprintf(stderr, "Could not open %s: %s\n",
			        *argv, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (pri.seekto > 0)
			lseek(ffd, pri.seekto, SEEK_SET);
		else
			lseek(ffd, pri.seekto, SEEK_END);
	} else if ((ffd = open(*argv, O_WRONLY | O_CREAT | O_TRUNC,
	    S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        *argv, strerror(errno));
		exit(EXIT_FAILURE);
	}

	{
	unsigned long long blk_read = 0;
	unsigned long bytes_per_sec = pri.smprate * pri.channels * pri.smpsize / 8;
	int run = 1;

	if (pri.verbose > 0)
		fprintf(stderr, "Recording to %s\n", *argv);

	while (run) {
		int have_read = read(dsp_fd, pri.buf, pri.blksize);

		if (have_read == 0) {
			run = 0;
		} else if (have_read < 0) {
			perror("Error while reading from device");
			run = 0;
		}

		if (write(ffd, pri.buf, have_read) < 0) {
			perror("Error while writing to file");
			run = 0;
		}

		++blk_read;
		switch (pri.verbose) {
			case 2:
				fprintf(stderr, ".");
				break;
			case 3: {
				unsigned long long rel_bytes = blk_read * pri.blksize,
					rel_pos = rel_bytes / bytes_per_sec,
					abs_pos = (pri.seekto + rel_bytes) / bytes_per_sec;
				fprintf(stderr, "\r[%3lld:%02lld * %3lld:%02lld]",
				        abs_pos / 60, abs_pos % 60, rel_pos / 60,
				        rel_pos % 60);
				break;
			}
			case 4: {
				unsigned long long rel_bytes = blk_read * pri.blksize,
					rel_pos = rel_bytes / bytes_per_sec,
					abs_bytes = pri.seekto + rel_bytes,
					abs_pos = abs_bytes / bytes_per_sec;
				fprintf(stderr, "\r[%3lld:%02lld * %3lld:%02lld; BP: %lldB * %lldB]", abs_pos / 60, abs_pos % 60,
				        rel_pos / 60, rel_pos % 60, abs_bytes, rel_bytes);
				break;
			}
		}
	}
	if (pri.verbose > 0)
		fprintf(stderr, "\n");
	/* block */
	}

	fprintf(stderr, "EOF on %s\n", (const char *)dv->first->ptr);
	close(dsp_fd);
	HXdeque_free(dv);
	return;
}
