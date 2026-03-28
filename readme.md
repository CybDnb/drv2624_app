ioctl使用示例

1. 本地闭环waveform播放示例
```c
int fd;
int mode;
int effect;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_WAVEFORM;
effect = 1;

ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);
ioctl(fd, DRV2624_SELECT_EFFECT, &effect);
ioctl(fd, DRV2624_SET_START_LOCAL);
close(fd);
```
说明
  用于播放芯片内建effect或已经加载到drv2624.bin中的effect
  这里不是把bin路径传给ioctl
  而是驱动侧先加载drv2624.bin 然后用户态只需要传effect id

2. 使用sequencer播放bin中多个effect示例
```c
int fd;
int mode;
int main_loop;
struct drv2624_waveform_sequencer sequencer;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_WAVEFORM;
main_loop = DRV2624_MAIN_NO_LOOP;
memset(&sequencer, 0, sizeof(sequencer));

sequencer.waveform[0].effect = 1;
sequencer.waveform[0].loop = 0;
sequencer.waveform[1].effect = 2;
sequencer.waveform[1].loop = 1;

ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);
ioctl(fd, DRV2624_SET_MAIN_LOOP, &main_loop);
ioctl(fd, DRV2624_SET_SEQUENCER, &sequencer);
ioctl(fd, DRV2624_SET_START_LOCAL);
close(fd);
```
说明
  这里第一段播放effect 1一次
  第二段播放effect 2并在slot内额外循环1次
  适用于drv2624.bin中已经定义好的多个effect组合播放

3. 使用drv2624.bin的完整流程示例
```c
int fd;
int mode;
int main_loop;
struct drv2624_waveform_sequencer sequencer;

/*
 * 前提：
 * 1. 先用 script/make_drv2624_bin.py 生成 drv2624.bin
 * 2. 把 drv2624.bin 放到驱动会加载的位置
 * 3. 确认驱动已经完成bin加载
 */

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_WAVEFORM;
main_loop = DRV2624_MAIN_NO_LOOP;
memset(&sequencer, 0, sizeof(sequencer));

/* 这些effect id来自drv2624.bin中的effects[]定义 */
sequencer.waveform[0].effect = 1;
sequencer.waveform[0].loop = 0;
sequencer.waveform[1].effect = 2;
sequencer.waveform[1].loop = 0;

ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);
ioctl(fd, DRV2624_SET_MAIN_LOOP, &main_loop);
ioctl(fd, DRV2624_SET_SEQUENCER, &sequencer);
ioctl(fd, DRV2624_SET_START_LOCAL);
close(fd);
```
说明
  这就是ioctl使用waveform bin的典型方式
  用户态不会把drv2624.bin文件内容通过ioctl直接传入
  用户态只负责传effect id和sequencer配置
  如果effect id只存在于drv2624.bin且能成功播放 说明驱动已经加载到该bin

4. 同步闭环RTP输出示例
```c
int fd;
int mode;
int level;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_RTP;
level = 80;

ioctl(fd, DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE, &mode);
ioctl(fd, DRV2624_RUN_STREAM_CLOSE, &level);
close(fd);
```
说明
  用于双马达同步闭环输出一个RTP幅值
  level范围是-128到127

5. 本地开环RTP输出示例
```c
int fd;
int level;
struct drv2624_openloop_config config;

fd = open("/dev/drv2624-1", O_RDWR);
config.mode = DRV2624_MODE_RTP;
config.ol_frequency = 235;
level = 80;

ioctl(fd, DRV2624_CONFIG_LOCAL_OPEN_LOOP_MODE, &config);
ioctl(fd, DRV2624_RUN_STREAM_OPEN, &level);
close(fd);
```
说明
  用于本地开环模式下输出一个RTP幅值
  ol_frequency 是开环频率

6. 读取状态寄存器示例
```c
int fd;
int status;

fd = open("/dev/drv2624-1", O_RDWR);
ioctl(fd, DRV2624_GET_STATUS, &status);
printf("status=0x%02x\n", status & 0xff);
close(fd);
```
说明
  读取后会清除sticky bits

7. 查询和设置播放间隔示例
```c
int fd;
int interval_ms;

fd = open("/dev/drv2624-1", O_RDWR);

interval_ms = 1;
ioctl(fd, DRV2624_SET_PLAYBACK_INTERVAL, &interval_ms);

ioctl(fd, DRV2624_GET_PLAYBACK_INTERVAL, &interval_ms);
printf("playback_interval_ms=%d\n", interval_ms);
close(fd);
```
说明
  只允许设置为1ms或5ms

8. 同步开环RTP输出示例
```c
int fd;
int level;
struct drv2624_openloop_config config;

fd = open("/dev/drv2624-1", O_RDWR);
config.mode = DRV2624_MODE_RTP;
config.ol_frequency = 235;
level = 80;

ioctl(fd, DRV2624_CONFIG_SYNC_OPEN_LOOP_MODE, &config);
ioctl(fd, DRV2624_RUN_STREAM_OPEN, &level);
close(fd);
```
说明
  用于双马达同步开环模式下输出一个RTP幅值

9. 只做模式切换示例
```c
int fd;
int mode;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_RTP;
ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);

mode = DRV2624_MODE_WAVEFORM;
ioctl(fd, DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE, &mode);
close(fd);
```
说明
  localmode命令本质上就是调用DRV2624_CONFIG_LOCAL_MODE
  syncmode命令本质上就是调用DRV2624_CONFIG_SYNC_CLOSE_LOOP_MODE

10. 使用CHANGE_MODE和CHANGE_TRIGGERFUNC示例
```c
int fd;
int mode;
int trigger;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_WAVEFORM;
trigger = 0;

ioctl(fd, DRV2624_CHANGE_MODE, &mode);
ioctl(fd, DRV2624_CHANGE_TRIGGERFUNC, &trigger);
close(fd);
```
说明
  这是app_common.c中configure_device()封装的两步
  适合在需要分别切换mode和trigger功能时使用

11. 顺序发送一组RTP采样点示例
```c
int fd;
int mode;
int samples[] = {0, 20, 40, 60, 40, 20, 0, -20, -40};
unsigned int i;

fd = open("/dev/drv2624-1", O_RDWR);
mode = DRV2624_MODE_RTP;
ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);

for (i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
    int level = samples[i];
    ioctl(fd, DRV2624_RUN_STREAM_CLOSE, &level);
    usleep(650);
}

close(fd);
```
说明
  这就是stream播放的基本方式
  如果改成开环播放则把DRV2624_RUN_STREAM_CLOSE替换成DRV2624_RUN_STREAM_OPEN

12. stream bin文件的ioctl使用方式示例
```c
int fd;
FILE *fp;
struct drv2624_stream_bin_header header;
signed char *samples;
uint32_t i;
int mode;

fd = open("/dev/drv2624-1", O_RDWR);
fp = fopen("/userdata/music.bin", "rb");
fread(&header, sizeof(header), 1, fp);

samples = malloc(header.sample_count);
fread(samples, 1, header.sample_count, fp);

mode = DRV2624_MODE_RTP;
ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);

for (i = 0; i < header.sample_count; i++) {
    int level = samples[i];
    ioctl(fd, DRV2624_RUN_STREAM_CLOSE, &level);
    usleep(header.interval_us);
}

free(samples);
fclose(fp);
close(fd);
```
说明
  这里的music.bin或samples.bin不是waveform effect bin
  它们是带SBN1头的stream bin
  用法是用户态先读出采样值 再循环调用DRV2624_RUN_STREAM_CLOSE或DRV2624_RUN_STREAM_OPEN发送给驱动

13. 切到DIAGNOSTICS或CALIBRATION模式示例
```c
int fd;
int mode;

fd = open("/dev/drv2624-1", O_RDWR);

mode = DRV2624_MODE_CALIBRATION;
ioctl(fd, DRV2624_CONFIG_LOCAL_MODE, &mode);

close(fd);
```
说明
  用于进入校准模式


脚本使用示例

1. 生成drv2624.bin示例
```bash
python3 script/make_drv2624_bin.py drv2624_waveform_example.json
python3 script/make_drv2624_bin.py drv2624_waveform_example.json /userdata/drv2624.bin
```
说明
  第一个命令会在当前目录输出默认文件名 drv2624.bin
  第二个命令会把输出文件写到指定路径

json示例
```json
{
  "fw_date": 20260325,
  "revision": 0,
  "effects": [
    {
      "id": 1,
      "name": "click",
      "repeat": 0,
      "data": [63, 8, 0, 8]
    },
    {
      "id": 2,
      "name": "double_click",
      "repeat": 0,
      "data": [63, 6, 0, 6, 63, 6, 0, 8]
    }
  ]
}
```
说明
  effect id必须从1开始连续递增
  repeat范围是0到7
  data必须是偶数个字节 表示幅值和时长成对出现

2. 由music.txt生成music.bin示例
```bash
python3 script/make_music_bin.py music.txt music.bin
python3 script/make_music_bin.py music.txt /userdata/music.bin --interval-us 650
```
说明
  第一个参数是输入文本
  第二个参数是输出bin文件
  --interval-us 用于设置采样间隔 默认650us

music.txt示例
```txt
tone 261.63 300 72
rest 60
tone 329.63 300 72
rest 60
```
说明
  tone格式为 tone <freq_hz> <duration_ms> [amplitude]
  amplitude可省略 省略时默认70
  rest格式为 rest <duration_ms>

3. 由samples.txt生成samples.bin示例
```bash
python3 script/make_samples_bin.py samples.txt samples.bin
python3 script/make_samples_bin.py samples.txt /userdata/samples.bin --interval-us 650
```
说明
  输入文件中的每个采样值用空格或换行分隔
  采样值范围必须在-128到127之间
  输出bin同样带有SBN1头部 可直接用于流播放

samples.txt示例
```txt
0 20 40 60 40 20 0 -20 -40 -60 -40 -20
```
