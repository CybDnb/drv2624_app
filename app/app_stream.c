#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app_shared.h"

static int send_stream_sample(int fd, int level, int *sample_count,
			      unsigned int interval_us, int open_loop)
{
	int ret;
	unsigned long cmd;
	const char *name;

	if (open_loop) {
		cmd = DRV2624_RUN_STREAM_OPEN;
		name = "ioctl DRV2624_RUN_STREAM_OPEN";
	} else {
		cmd = DRV2624_RUN_STREAM_CLOSE;
		name = "ioctl DRV2624_RUN_STREAM_CLOSE";
	}

	ret = ioctl(fd, cmd, &level);
	if (ret < 0) {
		perror(name);
		return ret;
	}

	if (sample_count)
		(*sample_count)++;
	usleep(interval_us);
	return 0;
}

int run_stream_test(const char *path, int level, int sync_mode)
{
	int fd;
	int ret;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	if (sync_mode)
		ret = configure_sync_close_loop_mode(fd, DRV2624_MODE_RTP,
				"ioctl DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE");
	else
		ret = configure_local_mode(fd, DRV2624_MODE_RTP,
					   "ioctl DRV2624_CONFIG_LOCAL_MODE");
	if (ret < 0)
		goto out_close;

	printf("running RTP stream on %s: level=%d mode=%s\n",
	       path, level, sync_mode ? "sync" : "local");
	ret = ioctl(fd, DRV2624_RUN_STREAM_CLOSE, &level);
	if (ret < 0) {
		perror("ioctl DRV2624_RUN_STREAM_CLOSE");
		goto out_close;
	}

	ret = 0;

out_close:
	close(fd);
	return ret;
}

int play_stream_samples(int fd, const signed char *samples,
			uint32_t sample_count, unsigned int interval_us,
			int sync_mode, int open_loop, int ol_frequency,
			const char *label)
{
	int ret;
	uint32_t i;
	int sent_count;

	if (open_loop) {
		if (sync_mode)
			ret = configure_sync_open_loop_mode(fd, DRV2624_MODE_RTP,
				ol_frequency,
				"ioctl DRV2624_CONFIG_SYNC_OPEN_LOOP_MODE");
		else
			ret = configure_local_open_loop_mode(fd, DRV2624_MODE_RTP,
				ol_frequency,
				"ioctl DRV2624_CONFIG_LOCAL_OPEN_LOOP_MODE");
	} else {
		if (sync_mode)
			ret = configure_sync_close_loop_mode(fd, DRV2624_MODE_RTP,
				"ioctl DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE");
		else
			ret = configure_local_mode(fd, DRV2624_MODE_RTP,
				"ioctl DRV2624_CONFIG_LOCAL_MODE");
	}
	if (ret < 0)
		return ret;

	printf("streaming %s in %s mode, samples=%u interval=%uus\n",
	       label,
	       open_loop ? (sync_mode ? "open-loop-sync" : "open-loop-local") :
	       (sync_mode ? "sync" : "local"),
	       sample_count, interval_us);

	sent_count = 0;
	for (i = 0; i < sample_count; i++) {
		ret = send_stream_sample(fd, samples[i], &sent_count,
					 interval_us, open_loop);
		if (ret < 0)
			return ret;
	}

	printf("%s finished: %d samples sent\n", label, sent_count);
	return 0;
}

int run_stream_bin_test(const char *path, const char *bin_path,
			int sync_mode, int open_loop, int ol_frequency)
{
	struct drv2624_stream_bin_header header;
	signed char *samples;
	FILE *fp;
	int fd;
	int ret;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	fp = fopen(bin_path, "rb");
	if (!fp) {
		perror(bin_path);
		close(fd);
		return -1;
	}

	if (fread(&header, sizeof(header), 1, fp) != 1) {
		fprintf(stderr, "failed to read stream bin header: %s\n", bin_path);
		ret = -1;
		goto out_close;
	}

	if (memcmp(header.magic, DRV2624_STREAM_BIN_MAGIC, sizeof(header.magic))) {
		fprintf(stderr, "invalid stream bin magic: %s\n", bin_path);
		ret = -1;
		goto out_close;
	}

	if (!header.interval_us || !header.sample_count) {
		fprintf(stderr, "invalid stream bin header values: %s\n", bin_path);
		ret = -1;
		goto out_close;
	}

	samples = malloc(header.sample_count);
	if (!samples) {
		fprintf(stderr, "failed to allocate sample buffer\n");
		ret = -1;
		goto out_close;
	}

	if (fread(samples, 1, header.sample_count, fp) != header.sample_count) {
		fprintf(stderr, "failed to read stream bin payload: %s\n", bin_path);
		free(samples);
		ret = -1;
		goto out_close;
	}

	ret = play_stream_samples(fd, samples, header.sample_count,
				  header.interval_us, sync_mode, open_loop,
				  ol_frequency,
				  "streambin");
	free(samples);

out_close:
	fclose(fp);
	close(fd);
	return ret;
}
