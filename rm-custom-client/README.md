# rm-custom-client

`rm-custom-client` 运行在 `Ubuntu` 上，完成以下工作：

- 连接 `192.168.12.1:3333`，通过 `MQTT + Protobuf` 收发官方自定义客户端数据
- 订阅 `CustomByteBlock`，接收 `0x0310` 上来的英雄车自研视频字节流
- 监听 `UDP 3334`，接收官方 `HEVC` 图传码流
- 分别解码两路视频并显示
- 提供 `KeyboardMouseControl` 和 `CustomControl` 的基础发送能力

## 依赖

- `Qt6 Widgets/Network` 或兼容的 `Qt5`
- `protobuf`
- `FFmpeg` 的 `libavcodec`、`libavutil`、`libswscale`
- `paho-mqttpp3`
- `paho-mqtt3as`

Ubuntu 常见安装示例：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config protobuf-compiler libprotobuf-dev \
  qt6-base-dev libavcodec-dev libavutil-dev libswscale-dev \
  libpaho-mqttpp3-dev libpaho-mqtt-dev
```

如果你的系统是 `Qt5`，可以把 `qt6-base-dev` 替换为相应的 `Qt5` 开发包。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
./build/rm-custom-client --mqtt-host 192.168.12.1 --mqtt-port 3333 --udp-port 3334 --robot-id 1
```

其中：

- `--mqtt-host`：官方选手端主机对自定义客户端网卡地址，默认 `192.168.12.1`
- `--mqtt-port`：官方 MQTT 端口，默认 `3333`
- `--udp-port`：官方图传码流端口，默认 `3334`
- `--robot-id`：客户端 `clientID`，例如红方英雄填 `1`，蓝方英雄填 `101`

## 网络要求

- 官方选手端主机连接自定义客户端的网卡固定为 `192.168.12.1/24`
- Ubuntu 自定义客户端固定为 `192.168.12.2/24`
- 官方选手端和自定义客户端必须登录同一台机器人
- 只有当官方选手端已经正常显示图传画面时，`UDP 3334` 才会出流

## 界面说明

- 左侧画面：官方 `UDP 3334` 图传 `HEVC` 画面
- 右侧画面：通过 `CustomByteBlock` 收到的英雄车自研视频画面
- 状态区：显示比赛状态、机器人动态状态、模块状态、位置和英雄车流配置
- 日志区：显示 MQTT、UDP、解码器和自研协议解析日志

## 控制说明

- 键盘按位映射按官方协议：`W S A D Shift Ctrl Q E R F G Z X C V B`
- 鼠标移动和滚轮在官方图传画面区域内捕获
- `CustomControl` 通过界面上的十六进制输入框发送，自动截断到 `30` 字节

## 联调顺序

1. 先只运行官方选手端，确认其本身能正常出图传画面
2. 启动本客户端，确认左侧官方 `HEVC` 画面出现
3. 再接入 `MCU + hero-camera-agent`，确认 `CustomByteBlock` 开始到达
4. 观察右侧英雄车自研画面是否开始解码显示
5. 最后再验证键鼠和 `CustomControl` 是否能正常下发

## 常见问题

- `UDP 3334` 没有数据
  - 先确认官方选手端画面已经正常显示
  - 检查 Ubuntu 端是否真的是 `192.168.12.2`
  - 检查是否有防火墙阻止 `3334/udp`

- MQTT 连接不上
  - 检查选手端主机网卡是否配置为 `192.168.12.1`
  - 检查 `clientID` 是否填写为对应机器人 ID
  - 检查双方是否登录同一台机器人

- 右侧英雄车画面不显示
  - 检查 `MCU` 是否确实把整包透传到 `0x0310`
  - 检查 `CustomByteBlock` 是否持续到达
  - 检查自研链路是否已收到 `config` 和关键帧
