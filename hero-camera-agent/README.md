# hero-camera-agent

`hero-camera-agent` 运行在英雄车上的 `x86_64 Ubuntu` 小电脑上，负责：

- 连接海康 `MV-CS016` USB 相机
- 采集实时画面
- 对画面做 `ROI`、降采样、静态信息剔除、可选高亮运动弹丸拖影增强
- 使用 `H.264` 编码为连续 `Annex-B` 字节流
- 将码流切为不超过 `300` 字节的自定义协议包
- 通过串口发送给 `MCU`

## 依赖

- `cmake`
- `g++`
- `OpenCV 4`
- `FFmpeg` 开发库：`libavcodec`、`libavutil`、`libswscale`
- 海康 `MVS SDK`

Ubuntu 常见依赖安装示例：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libopencv-dev \
  libavcodec-dev libavutil-dev libswscale-dev
```

如果 `MVS SDK` 不在默认位置 `/opt/MVS`，请先设置：

```bash
export MVS_SDK_ROOT=/path/to/MVS
```

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

如果构建时提示找不到 `MvCameraControl.h` 或 `MvCameraControl`，说明 `MVS SDK` 的包含目录或库目录还没有被正确识别。

## 运行

默认使用海康相机采集并从 `/dev/ttyUSB0` 发送：

推荐把参数写进 `config/hero-camera-agent.yaml`，启动时只带一个配置文件参数：

```bash
./build/hero-camera-agent --config config/hero-camera-agent.yaml
```

命令行参数仍然可用，并且会覆盖 `YAML` 里的同名配置，例如：

```bash
./build/hero-camera-agent --config config/hero-camera-agent.yaml --exposure-us 18000
```

当前主配置 `config/hero-camera-agent.yaml` 已经按“流畅、低延迟、能看到弹丸托影优先”整理，可以直接比赛使用。

```bash
./build/hero-camera-agent --serial /dev/ttyUSB0 --baud 1500000 --source mvs --device-index 0 --fps 60 --width 300 --height 300 --target-bytes 11000 --exposure-us 12000
```
```bash
./build/hero-camera-agent --serial /dev/ttyUSB0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 8000 --max-stream-pps 40 --exposure-us 12000
```

如果小电脑同时接了两台相同型号的海康相机，建议直接在 `config/hero-camera-agent.yaml` 里填写 `camera.device_serial`，之后启动命令保持不变。
如果你想先用本地视频验证编码和串口切片逻辑：

```bash
./build/hero-camera-agent --serial /dev/ttyUSB0 --baud 1500000 --video /path/to/test.mp4 --fps 60 --width 300 --height 300 --target-bytes 11000
```

## 参数说明

- `--serial`：发送到 `MCU` 的串口设备，例如 `/dev/ttyUSB0`
- `--config`：`YAML` 配置文件路径，推荐使用 `config/hero-camera-agent.yaml`
- `--baud`：串口波特率，默认 `1500000`
- `--source`：`mvs`、`opencv`、`file`
- `--device-index`：相机序号
- `--device-serial`：固定绑定的海康相机序列号，填了后优先于 `--device-index`
- `--video`：本地视频文件路径，提供后自动切到 `file` 模式
- `--fps`：采集和编码帧率
- `--gain`：手动增益，适合在较低曝光下提升亮弹丸可见度
- `--width` / `--height`：预处理后输出分辨率
- `--target-bytes`：目标码率，单位 `Byte/s`
- `--exposure-us`：曝光时间，单位微秒
- `--disable-trail`：关闭弹道拖影增强

当前拖影增强逻辑会优先保留“高亮且在运动”的小目标，因此对白色高亮弹丸和比赛中的绿色发光弹丸都适用；当整张画面的运动像素占比过大时，会临时关闭拖影，等画面重新稳定后再自动恢复，避免大范围运动导致拖影铺满整个画面。

如果你觉得一拉曝光就不流畅，推荐优先尝试“降低 `exposure_us` + 提高 `gain`”，而不是单纯继续拉曝光。

推荐在 `config/hero-camera-agent.yaml` 里优先调这几个参数：

- `preprocessor.trail_brightness_threshold`
- `preprocessor.trail_disable_motion_ratio`
- `preprocessor.trail_reenable_frames`
- `preprocessor.trail_history_frames`

当前主配置已经默认采用这组低延迟优先参数：

- `camera.exposure_us: 12000`
- `camera.gain: 10.0`
- `camera.fps: 60`
- `preprocessor.trail_brightness_threshold: 135`
- `preprocessor.trail_disable_motion_ratio: 0.06`

## 双相机固定绑定

1. 把两台海康相机都接到小电脑上。
2. 打开海康 `MVS` 客户端，确认两台相机都在线，并记下你想固定读取那台的序列号。
3. 编辑 `config/hero-camera-agent.yaml`，把 `camera.device_serial` 改成目标相机的序列号。
4. 使用 `./build/hero-camera-agent --config config/hero-camera-agent.yaml` 运行 `hero-camera-agent`。
6. 如果序列号填错，程序会直接报错，并把当前枚举到的相机序列号打印出来，方便重新核对。

## 输出协议

- 协议头定义见 `src/hero_protocol.h`
- 单包最大长度：`300` 字节
- 单包有效负载最大长度：`284` 字节
- 包类型：`config`、`stream_chunk`、`heartbeat`、`reset`
- 校验方式：`CRC16/CCITT-FALSE`

## 联调步骤

1. 先确认海康 `MVS` 相机在小电脑上能正常出图
2. 使用本地视频模式验证串口发包稳定性
3. 接上真实相机，观察终端中 `captured`、`encoded`、`sentPackets` 日志是否稳定增长
4. 再接入 `MCU` 侧 `rm-video-bridge`

## 常见问题

- 构建时报 `MVS SDK not found`
  - 检查 `/opt/MVS/include/MvCameraControl.h`
  - 检查 `/opt/MVS/lib/64/libMvCameraControl.so`
  - 或设置 `MVS_SDK_ROOT`

- 串口打不开
  - 检查设备名是否正确
  - 检查当前用户是否有串口权限
  - 检查波特率是否被 `USB-UART` 芯片支持

- 码流太大
  - 先把 `--target-bytes` 降到 `9000`
  - 降低输出分辨率到 `256x256`
  - 关闭拖影或缩小 `ROI`

- 拖影太容易误触发或满屏拖花
  - 提高 `trail_brightness_threshold`
  - 降低 `trail_disable_motion_ratio`
  - 缩短 `trail_history_frames`
