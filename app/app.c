#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "app_shared.h"

int main(int argc, char const *argv[])
{
	enum app_command cmd;
	int ret0;
	int ret1;
	int fd;
	char path[32];
	int device;
	int effect;
	int mode;
	int level;
	int status;
	int interval_ms;
	int sync_mode;
	int open_loop;
	int ol_frequency;
	int max_amp;
	int target_rate_hz;
	int parsed_value;
	int i;
	int max_amp_set;
	int target_rate_set;

	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	cmd = parse_command(argv[1]);

	switch (cmd) {
	case CMD_CALIBRATE:
		ret0 = calibrate_device("/dev/drv2624-1");
		ret1 = calibrate_device("/dev/drv2624-2");
		return (ret0 < 0 || ret1 < 0) ? EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_WAVEFORM:
		if (argc != 4) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		effect = atoi(argv[3]);
		if (device < 0 ||
		    (effect != DRV2624_EFFECT_CLICK &&
		     effect != DRV2624_EFFECT_DOUBLE_CLICK)) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_waveform(path, effect) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_BINTEST:
		if (argc < 4) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_bin_test(path, argc - 3, &argv[3]) < 0 ?
			EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_STREAM:
		if (argc != 4 && argc != 5) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		level = atoi(argv[3]);
		if (device < 0 || level < -128 || level > 127) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		sync_mode = 0;
		if (argc == 5) {
			sync_mode = parse_sync_mode_arg(argv[4]);
			if (sync_mode < 0) {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_stream_test(path, level, sync_mode) < 0 ?
			EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_WAV:
	case CMD_WAVOL:
		if ((cmd == CMD_WAV && (argc < 4 || argc > 7)) ||
		    (cmd == CMD_WAVOL && (argc < 4 || argc > 7))) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		sync_mode = 0;
		open_loop = (cmd == CMD_WAVOL);
		ol_frequency = 0;
		max_amp = DRV2624_DEFAULT_WAV_MAX_AMP;
		target_rate_hz = 0;
		max_amp_set = 0;
		target_rate_set = 0;
		for (i = 4; i < argc; i++) {
			int parsed_mode;

			parsed_mode = parse_sync_mode_arg(argv[i]);
			if (!open_loop && parsed_mode >= 0) {
				sync_mode = parsed_mode;
				continue;
			}
			if (parse_int_arg(argv[i], &parsed_value) < 0) {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
			if (!max_amp_set) {
				if (parsed_value < 1 || parsed_value > 127) {
					print_usage(argv[0]);
					return EXIT_FAILURE;
				}
				max_amp = parsed_value;
				max_amp_set = 1;
			} else if (!target_rate_set) {
				if (parsed_value < 0) {
					print_usage(argv[0]);
					return EXIT_FAILURE;
				}
				target_rate_hz = (unsigned int)parsed_value;
				target_rate_set = 1;
			} else if (open_loop && ol_frequency == 0) {
				if (parsed_value <= 0) {
					print_usage(argv[0]);
					return EXIT_FAILURE;
				}
				ol_frequency = parsed_value;
			} else {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_stream_wav_test(path, argv[3], sync_mode, open_loop,
					   ol_frequency,
					   max_amp, target_rate_hz) < 0 ?
			EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_MUSIC:
	case CMD_MUSICOL:
	case CMD_SAMPLES: {
		const char *bin_path;

		if ((cmd == CMD_MUSICOL && argc != 3 && argc != 4) ||
		    (cmd != CMD_MUSICOL && argc != 3 && argc != 4)) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		sync_mode = 0;
		open_loop = 0;
		ol_frequency = 0;
		if (argc == 4) {
			if (cmd == CMD_MUSICOL) {
				if (parse_int_arg(argv[3], &ol_frequency) < 0 ||
				    ol_frequency <= 0) {
					print_usage(argv[0]);
					return EXIT_FAILURE;
				}
			} else {
				sync_mode = parse_sync_mode_arg(argv[3]);
				if (sync_mode < 0) {
					print_usage(argv[0]);
					return EXIT_FAILURE;
				}
			}
		}
		if (cmd == CMD_MUSICOL)
			open_loop = 1;
		bin_path = (cmd == CMD_MUSIC) ?
			DRV2624_DEFAULT_MUSIC_BIN :
			((cmd == CMD_MUSICOL) ?
			 DRV2624_DEFAULT_MUSIC_BIN :
			 DRV2624_DEFAULT_SAMPLES_BIN);
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_stream_bin_test(path, bin_path, sync_mode, open_loop,
					   ol_frequency) < 0 ?
			EXIT_FAILURE : EXIT_SUCCESS;
	}

	case CMD_STREAMBIN:
		if (argc != 4 && argc != 5) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		sync_mode = 0;
		if (argc == 5) {
			sync_mode = parse_sync_mode_arg(argv[4]);
			if (sync_mode < 0) {
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		return run_stream_bin_test(path, argv[3], sync_mode, 0, 0) < 0 ?
			EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_STATUS:
		if (argc != 3) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			perror(path);
			return EXIT_FAILURE;
		}
		ret0 = get_status_value(fd, &status);
		close(fd);
		if (ret0 < 0)
			return EXIT_FAILURE;
		printf("status=0x%02x\n", status & 0xff);
		return EXIT_SUCCESS;

	case CMD_INTERVAL:
		if (argc != 3 && argc != 4) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		if (device < 0) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			perror(path);
			return EXIT_FAILURE;
		}
		if (argc == 4) {
			if (parse_int_arg(argv[3], &interval_ms) < 0 ||
			    (interval_ms != 1 && interval_ms != 5)) {
				close(fd);
				print_usage(argv[0]);
				return EXIT_FAILURE;
			}
			ret0 = set_playback_interval_value(
				fd, interval_ms, "ioctl DRV2624_SET_PLAYBACK_INTERVAL");
			if (ret0 < 0) {
				close(fd);
				return EXIT_FAILURE;
			}
		}
		ret0 = get_playback_interval_value(fd, &interval_ms);
		close(fd);
		if (ret0 < 0)
			return EXIT_FAILURE;
		printf("playback_interval_ms=%d\n", interval_ms);
		return EXIT_SUCCESS;

	case CMD_LOCALMODE:
	case CMD_SYNCMODE:
		if (argc != 4) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		device = parse_device_arg(argv[2]);
		mode = atoi(argv[3]);
		if (device < 0 ||
		    mode < DRV2624_MODE_RTP ||
		    mode > DRV2624_MODE_CALIBRATION) {
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		snprintf(path, sizeof(path), "/dev/drv2624-%d", device);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			perror(path);
			return EXIT_FAILURE;
		}
		if (cmd == CMD_LOCALMODE)
			effect = configure_local_mode(fd, mode,
					"ioctl DRV2624_CONFIG_LOCAL_MODE");
		else
			effect = configure_sync_close_loop_mode(fd, mode,
					"ioctl DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE");
		close(fd);
		return effect < 0 ? EXIT_FAILURE : EXIT_SUCCESS;

	case CMD_INVALID:
	default:
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
}
