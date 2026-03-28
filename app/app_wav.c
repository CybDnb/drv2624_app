#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_shared.h"

static uint16_t read_le16(const unsigned char *buf)
{
	return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static uint32_t read_le32(const unsigned char *buf)
{
	return (uint32_t)buf[0] |
	       ((uint32_t)buf[1] << 8) |
	       ((uint32_t)buf[2] << 16) |
	       ((uint32_t)buf[3] << 24);
}

static int load_wav_as_stream(const char *wav_path, int max_amp,
			      unsigned int target_rate_hz,
			      signed char **out_samples,
			      uint32_t *out_sample_count,
			      unsigned int *out_interval_us)
{
	unsigned char chunk_header[8];
	unsigned char riff_header[12];
	unsigned char *data_buf;
	FILE *fp;
	uint32_t data_size = 0;
	uint32_t sample_rate = 0;
	uint16_t audio_format = 0;
	uint16_t channels = 0;
	uint16_t bits_per_sample = 0;
	uint16_t block_align = 0;
	uint32_t frame_count;
	uint32_t target_rate;
	uint32_t target_count;
	uint32_t i;
	signed char *samples;
	int ret;

	ret = -1;
	fp = fopen(wav_path, "rb");
	if (!fp) {
		perror(wav_path);
		return -1;
	}

	if (fread(riff_header, sizeof(riff_header), 1, fp) != 1) {
		fprintf(stderr, "failed to read wav header: %s\n", wav_path);
		goto out_close;
	}

	if (memcmp(riff_header, "RIFF", 4) || memcmp(riff_header + 8, "WAVE", 4)) {
		fprintf(stderr, "unsupported wav container: %s\n", wav_path);
		goto out_close;
	}

	while (fread(chunk_header, sizeof(chunk_header), 1, fp) == 1) {
		uint32_t chunk_size;
		long skip_size;

		chunk_size = read_le32(chunk_header + 4);
		if (!memcmp(chunk_header, "fmt ", 4)) {
			unsigned char fmt_buf[40];

			if (chunk_size < 16 || chunk_size > sizeof(fmt_buf)) {
				fprintf(stderr, "unsupported wav fmt chunk: %s\n",
					wav_path);
				goto out_close;
			}
			if (fread(fmt_buf, chunk_size, 1, fp) != 1) {
				fprintf(stderr, "failed to read wav fmt chunk: %s\n",
					wav_path);
				goto out_close;
			}
			audio_format = read_le16(fmt_buf);
			channels = read_le16(fmt_buf + 2);
			sample_rate = read_le32(fmt_buf + 4);
			block_align = read_le16(fmt_buf + 12);
			bits_per_sample = read_le16(fmt_buf + 14);
		} else if (!memcmp(chunk_header, "data", 4)) {
			data_size = chunk_size;
			break;
		} else {
			skip_size = chunk_size;
			if (chunk_size & 1)
				skip_size++;
			if (fseek(fp, skip_size, SEEK_CUR) < 0) {
				fprintf(stderr, "failed to skip wav chunk: %s\n",
					wav_path);
				goto out_close;
			}
		}
	}

	if (audio_format != 1 || !channels || !sample_rate || !block_align ||
	    !data_size) {
		fprintf(stderr, "unsupported wav format: %s\n", wav_path);
		goto out_close;
	}

	if (bits_per_sample != 8 && bits_per_sample != 16) {
		fprintf(stderr, "only 8-bit/16-bit PCM wav is supported: %s\n",
			wav_path);
		goto out_close;
	}

	data_buf = malloc(data_size);
	if (!data_buf) {
		fprintf(stderr, "failed to allocate wav buffer\n");
		goto out_close;
	}

	if (fread(data_buf, 1, data_size, fp) != data_size) {
		fprintf(stderr, "failed to read wav payload: %s\n", wav_path);
		free(data_buf);
		goto out_close;
	}

	frame_count = data_size / block_align;
	target_rate = target_rate_hz ? target_rate_hz : sample_rate;
	if (target_rate > sample_rate)
		target_rate = sample_rate;
	if (!target_rate)
		target_rate = 1;

	target_count = (uint32_t)(((uint64_t)frame_count * target_rate) /
				  sample_rate);
	if (!target_count)
		target_count = 1;

	samples = malloc(target_count);
	if (!samples) {
		fprintf(stderr, "failed to allocate wav samples\n");
		free(data_buf);
		goto out_close;
	}

	for (i = 0; i < target_count; i++) {
		uint32_t src_frame;
		const unsigned char *frame_ptr;
		int sample_value;

		src_frame = (uint32_t)(((uint64_t)i * sample_rate) / target_rate);
		if (src_frame >= frame_count)
			src_frame = frame_count - 1;
		frame_ptr = data_buf + ((size_t)src_frame * block_align);

		if (bits_per_sample == 8) {
			unsigned int sum;
			uint16_t ch;

			sum = 0;
			for (ch = 0; ch < channels; ch++)
				sum += frame_ptr[ch];
			sample_value = ((int)(sum / channels)) - 128;
		} else {
			int sum;
			uint16_t ch;

			sum = 0;
			for (ch = 0; ch < channels; ch++) {
				const unsigned char *sample_ptr;
				int16_t pcm;

				sample_ptr = frame_ptr + (ch * 2);
				pcm = (int16_t)read_le16(sample_ptr);
				sum += pcm;
			}
			sample_value = (sum / channels) >> 8;
		}

		sample_value = (sample_value * max_amp) / 127;
		if (sample_value > 127)
			sample_value = 127;
		else if (sample_value < -128)
			sample_value = -128;

		samples[i] = (signed char)sample_value;
	}

	free(data_buf);
	*out_samples = samples;
	*out_sample_count = target_count;
	*out_interval_us = 1000000U / target_rate;
	if (!*out_interval_us)
		*out_interval_us = 1;
	ret = 0;

out_close:
	fclose(fp);
	return ret;
}

int run_stream_wav_test(const char *path, const char *wav_path,
			int sync_mode, int open_loop, int ol_frequency,
			int max_amp,
			unsigned int target_rate_hz)
{
	signed char *samples;
	uint32_t sample_count;
	unsigned int interval_us;
	int fd;
	int ret;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	samples = NULL;
	sample_count = 0;
	interval_us = 0;
	ret = load_wav_as_stream(wav_path, max_amp, target_rate_hz,
				 &samples, &sample_count, &interval_us);
	if (ret < 0)
		goto out_close;

	printf("wav config: max_amp=%d target_rate=%uHz\n",
	       max_amp, target_rate_hz);
	ret = play_stream_samples(fd, samples, sample_count, interval_us,
				  sync_mode, open_loop, ol_frequency, wav_path);
	free(samples);

out_close:
	close(fd);
	return ret;
}
