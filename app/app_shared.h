#ifndef DRV2624_APP_SHARED_H
#define DRV2624_APP_SHARED_H

#include <stdint.h>

#include "drv2624.h"

#define DRV2624_MAX_SEQ_ARGS 8
#define DRV2624_STREAM_BIN_MAGIC "SBN1"
#define DRV2624_DEFAULT_MUSIC_BIN "/userdata/music.bin"
#define DRV2624_DEFAULT_SAMPLES_BIN "/userdata/samples.bin"
#define DRV2624_DEFAULT_WAV_MAX_AMP 80

struct drv2624_stream_bin_header {
	char magic[4];
	uint32_t interval_us;
	uint32_t sample_count;
};

enum app_command {
	CMD_INVALID = 0,
	CMD_CALIBRATE,
	CMD_WAVEFORM,
	CMD_BINTEST,
	CMD_STREAM,
	CMD_WAV,
	CMD_WAVOL,
	CMD_MUSIC,
	CMD_MUSICOL,
	CMD_SAMPLES,
	CMD_STREAMBIN,
	CMD_STATUS,
	CMD_INTERVAL,
	CMD_LOCALMODE,
	CMD_SYNCMODE,
};

enum app_command parse_command(const char *cmd);
int parse_device_arg(const char *arg);
int parse_sync_mode_arg(const char *arg);
int parse_int_arg(const char *arg, int *value);
void print_usage(const char *prog);

int configure_device(int fd, int mode, int trigger, const char *name);
int configure_local_mode(int fd, int mode, const char *name);
int configure_local_open_loop_mode(int fd, int mode, int ol_frequency,
				   const char *name);
int configure_sync_open_loop_mode(int fd, int mode, int ol_frequency,
				  const char *name);
int configure_sync_close_loop_mode(int fd, int mode, const char *name);
int get_status_value(int fd, int *status);
int get_playback_interval_value(int fd, int *interval_ms);
int set_playback_interval_value(int fd, int interval_ms, const char *name);

int calibrate_device(const char *path);
int run_waveform(const char *path, int effect);
int run_bin_test(const char *path, int argc, char const *argv[]);
int run_stream_test(const char *path, int level, int sync_mode);
int play_stream_samples(int fd, const signed char *samples,
			uint32_t sample_count, unsigned int interval_us,
			int sync_mode, int open_loop, int ol_frequency,
			const char *label);
int run_stream_bin_test(const char *path, const char *bin_path,
			int sync_mode, int open_loop, int ol_frequency);
int run_stream_wav_test(const char *path, const char *wav_path,
			int sync_mode, int open_loop, int ol_frequency,
			int max_amp,
			unsigned int target_rate_hz);

#endif
