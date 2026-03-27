json文件格式
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
    ]
}
代表effect id为1
幅值为63执行8ms 然后幅值为0执行8ms
用make_drv2624_bin.py 将json解析为.bin文件 即可转化为适配drv2624 ram格式 被应用程序使用


music.txt文件格式
# Format:
#   tone <freq_hz> <duration_ms> [amplitude]
#   rest <duration_ms>
例如 
tone 261.63 300 72
rest 60
代表以261.63hz频率执行300ms 幅值为72
然后等待60ms
用make_music_bin,py 将其解析为.bin文件

samples.txt文件格式
# Source format: <time_s> <amplitude>; only amplitude column kept.
只是幅度值的连续变化 


