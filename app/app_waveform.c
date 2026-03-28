#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app_shared.h"

static int parse_seq_arg(const char *arg, struct drv2624_waveform *waveform)
{
	char *end;
	long effect;
	long loop;

	effect = strtol(arg, &end, 10);
	if (end == arg || effect <= 0 || effect > 127)
		return -1;

	loop = DRV2624_SEQ_NO_LOOP;
	if (*end == ':') {
		char *loop_end;

		loop = strtol(end + 1, &loop_end, 10);
		if (loop_end == end + 1 || *loop_end != '\0' ||
		    loop < DRV2624_SEQ_NO_LOOP ||
		    loop > DRV2624_SEQ_LOOP_THREE_TIMES)
			return -1;
	} else if (*end != '\0') {
		return -1;
	}

	waveform->effect = (unsigned char)effect;
	waveform->loop = (unsigned char)loop;
	return 0;
}

static int build_sequencer_from_args(int argc, char const *argv[],
				     struct drv2624_waveform_sequencer *sequencer)
{
	int i;

	memset(sequencer, 0, sizeof(*sequencer));

	if (argc <= 0 || argc > DRV2624_MAX_SEQ_ARGS)
		return -1;

	for (i = 0; i < argc; i++) {
		if (parse_seq_arg(argv[i], &sequencer->waveform[i]) < 0)
			return -1;
	}

	return 0;
}

static void dump_sequencer(const struct drv2624_waveform_sequencer *sequencer)
{
	int i;

	printf("sequencer:\n");
	for (i = 0; i < DRV2624_MAX_SEQ_ARGS; i++) {
		if (!sequencer->waveform[i].effect)
			break;

		printf("  slot%d: effect=%u loop=%u\n",
		       i + 1,
		       sequencer->waveform[i].effect,
		       sequencer->waveform[i].loop);
	}
}

int run_waveform(const char *path, int effect)
{
	int fd;
	int mode;
	int ret;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	mode = DRV2624_MODE_WAVEFORM;
	ret = configure_local_mode(fd, mode, "ioctl configure waveform");
	if (ret < 0)
		goto out_close;

	ret = ioctl(fd, DRV2624_SELECT_EFFECT, &effect);
	if (ret < 0) {
		perror("ioctl DRV2624_SELECT_EFFECT");
		goto out_close;
	}

	printf("playing effect %d on %s\n", effect, path);
	ret = ioctl(fd, DRV2624_SET_START_LOCAL);
	if (ret < 0) {
		perror("ioctl DRV2624_SET_START_LOCAL");
		goto out_close;
	}

	ret = 0;

out_close:
	close(fd);
	return ret;
}

int run_bin_test(const char *path, int argc, char const *argv[])
{
	struct drv2624_waveform_sequencer sequencer;
	int fd;
	int mode;
	int main_loop;
	int ret;

	if (build_sequencer_from_args(argc, argv, &sequencer) < 0) {
		fprintf(stderr, "invalid bintest sequence\n");
		return -1;
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	mode = DRV2624_MODE_WAVEFORM;
	ret = configure_local_mode(fd, mode, "ioctl configure bintest");
	if (ret < 0)
		goto out_close;

	main_loop = DRV2624_MAIN_NO_LOOP;
	ret = ioctl(fd, DRV2624_SET_MAIN_LOOP, &main_loop);
	if (ret < 0) {
		perror("ioctl DRV2624_SET_MAIN_LOOP");
		goto out_close;
	}

	ret = ioctl(fd, DRV2624_SET_SEQUENCER, &sequencer);
	if (ret < 0) {
		perror("ioctl DRV2624_SET_SEQUENCER");
		goto out_close;
	}

	dump_sequencer(&sequencer);
	printf("starting bin waveform on %s\n", path);
	ret = ioctl(fd, DRV2624_SET_START_LOCAL);
	if (ret < 0) {
		perror("ioctl DRV2624_SET_START_LOCAL");
		goto out_close;
	}

	printf("bintest started\n");
	printf("tip: if you use an effect id that only exists in drv2624.bin,\n");
	printf("     successful playback means the bin was loaded.\n");
	ret = 0;

out_close:
	close(fd);
	return ret;
}
