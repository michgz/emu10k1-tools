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
#include "ac3-iec958.h"

#ifndef AFMT_AC3
#define AFMT_AC3      0x00000400      /* Dolby Digital AC3 */
#endif

#define FRAMES 4

void usage()
{ 
	fprintf(stderr, "Usage: play-ac3 [-t <data type>] <filename | ->\n"); 
	fprintf(stderr, "\tUse -t to change the data type field in output data\n");
	exit(1);
}

int check_ac3_stream(int in, int *frame_size)
{
	int i, skipped;
	unsigned char buffer[4096];
	struct ac3info ai;

	i = read(in, buffer, sizeof(buffer));
	if (i != sizeof(buffer)) {
		fprintf(stderr, "Error reading AC3 stream.\n");
		return -1;
	}
	i = ac3_iec958_parse_syncinfo(buffer, sizeof(buffer), &ai, &skipped);
	if (i < 0) {
		fprintf(stderr, "AC3 stream not valid.\n");
		return -1;
	}
	if (ai.samplerate != 48000) {
		fprintf(stderr, "Only 48000 Hz streams supported.\n");
		return -1;
	}
	if (in != 0)
		i = lseek(in, skipped, SEEK_SET);
	else {
		i = read(in, buffer, ai.framesize-(sizeof(buffer)%ai.framesize)+skipped);
	}
	if (i < 0) {
		fprintf(stderr, "Error seeking in AC3 stream.\n");
		return -1;
	}
	*frame_size = ai.framesize;
	
	return 0;
}

int main(int argc, char *argv[])
{
	int	fd, in;
	int	tmp, i, c = 1, bytes_read, data_type = 1, frame_size;
	unsigned char *ac3buf, *iecbuf;
	
	if (argc < 2)
		usage();
	if (argc == 4) {
		if (strcmp(argv[c++], "-t") == 0) {
			data_type = atoi(argv[c++]);
			printf("Using data type %d.\n", data_type);
		} else
			usage();
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
	switch (data_type & 0x1F) {
	case IEC61937_DATA_TYPE_AC3:
	default:
		if (check_ac3_stream(in, &frame_size))
			exit(1);
		break;
	}
	fd = open("/dev/dsp", O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening DSP.\n");
		exit(1);
	}
	tmp = AFMT_AC3;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &tmp) < 0 || tmp != AFMT_AC3) {
		fprintf(stderr, "SNDCTL_DSP_SETFMT failed.\n");
		exit(1);
	}
	ac3buf = malloc(FRAMES * frame_size);
	iecbuf = malloc(FRAMES * 6144);
	bytes_read = read(in, ac3buf, FRAMES * frame_size);
	while (bytes_read == FRAMES * frame_size) {
		for (i = 0; i < FRAMES; i++) {
			ac3_iec958_build_burst(frame_size, data_type, 1, 
					ac3buf + i * frame_size,
					iecbuf + i * 6144);
		}
		write(fd, iecbuf, FRAMES * 6144);
		bytes_read = read(in, ac3buf, FRAMES * frame_size);
	}
	return 0;
}
