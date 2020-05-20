/*  ----------------------------------------------------------------------

    Copyright (C) 2001  Juha Yrjölä  (jyrjola@cc.hut.fi)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FRAMES 4

void usage()
{
	fprintf(stderr, "Usage: play-raw [-s] <filename | ->\n");
	fprintf(stderr, "\tUse -s to change endianness of input data\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int	fd, in, tmp, do_swab = 0, c = 1;
	unsigned char buffer[4096];
	
	if (argc < 2)
		usage();
	if (argc == 3) {
		if (strcmp(argv[1], "-s") == 0)
			do_swab = 1;
		else
			usage();
		c++;
	}
	if (strcmp(argv[c], "-") == 0) {
		fprintf(stderr, "Reading from stdin.\n");
		in = 0;
	} else {
		in = open(argv[c], O_RDONLY);
		if (in < 0) {
			fprintf(stderr, "Error opening file \"%s\".\n", argv[c]);
			exit(1);
		}
	}
	fd = open("/dev/dsp", O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening DSP.\n");
		exit(1);
	}
	tmp = AFMT_S16_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &tmp) < 0 || tmp != AFMT_S16_LE) {
		fprintf(stderr, "SNDCTL_DSP_SETFMT failed.\n");
		exit(1);
	}
	tmp = 48000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &tmp) < 0 || tmp != 48000) {
		fprintf(stderr, "SNDCTL_DSP_SPEED failed.\n");
		exit(1);
	}
	tmp = 1;
	if (ioctl(fd, SNDCTL_DSP_STEREO, &tmp) < 0 || tmp != 1) {
		fprintf(stderr, "SNDCTL_DSP_STEREO failed.\n");
		exit(1);
	}
	c = 0;
	while (read(in, buffer, sizeof(buffer)) == sizeof(buffer)) {
		if (do_swab)
			swab(buffer, buffer, sizeof(buffer));
		write(fd, buffer, sizeof(buffer));
		c++;
		// Simulate buffer underruns
#if 0
		if ((c % 20) == 0 && c)
			usleep(500*1000);
#endif
	}
	return 0;
}
