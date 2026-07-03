# custom-control-usb-bridge

这是一个独立命令行工具，不改动现有 `rm-custom-client` 代码。

已确认当前控制器接到客户端侧的串口参数为：`115200 8N1`。

它完成的链路是：

- 从客户端电脑上的 `RS232 转 USB` 设备读取串口字节流
- 自己扫描官方 `A5` 帧头
- 校验 `CRC8` 头校验
- 解析控制器送来的 `39` 字节官方帧
- 提取中间 `30` 字节有效载荷
- 封装为 `MQTT + Protobuf` 的 `CustomControl`
- 发布到服务器 `192.168.12.1:3333`
- 再由官方链路把这条 `CustomControl` 下发成 `0x0311` 给工程机器人

也就是说，这个工具发给 `MQTT` 的不是整帧 `39` 字节，而是 `CustomControl.data` 里的 `30` 字节 payload；官方后续再把它封装成 `0x0311` 下发给机器人。

## 1. 当前按什么帧格式解析

默认按下面结构找帧：

```text
buf[0]     = 0xA5
buf[1..2]  = payload length，小端，样例为 0x001E，即 30
buf[3]     = seq
buf[4]     = CRC8(header)
buf[5..6]  = cmd_id，小端，默认按控制器发送函数使用 `0x0302`
buf[7..36] = 30 字节有效载荷
buf[37..38]= CRC16(frame)
```

按你刚给的控制器发送函数，这 30 字节 payload 的含义是：

```text
payload[0..11]   = CAN1 六个电机位置，每个 int16 小端
payload[12..23]  = CAN2 六个电机位置，每个 int16 小端
payload[24..25]  = sensor_X，小端
payload[26..27]  = sensor_Y，小端
payload[28]      = button_pack
payload[29]      = 保留字节，当前固定 0x00
```

如果你现场设备实际发的不是 `0x0302`，运行时把输入帧命令码改掉即可：

```bash
./custom-control-usb-bridge/build/custom-control-usb-bridge \
  --serial /dev/ttyUSB0 \
  --cmd-id 0x0306
```

## 2. 依赖

Ubuntu 常见安装方式：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config protobuf-compiler libprotobuf-dev \
  libpaho-mqttpp3-dev libpaho-mqtt-dev
```

## 3. 编译

在项目根目录执行：

```bash
cmake -S custom-control-usb-bridge -B custom-control-usb-bridge/build
cmake --build custom-control-usb-bridge/build -j
```

## 4. 运行

先确认 USB 串口设备名：

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

一个常见启动例子：

```bash
./custom-control-usb-bridge/build/custom-control-usb-bridge \
  --serial /dev/ttyUSB0 \
  --baud 115200 \
  --data-bits 8 \
  --parity none \
  --stop-bits 1 \
  --mqtt-host 192.168.12.1 \
  --mqtt-port 3333 \
  --robot-id 2
```

如果是蓝方工程机器人，把 `--robot-id` 改成：

```bash
--robot-id 102
```

如果你想同时看每次发出去的 `30` 字节十六进制内容：

```bash
./custom-control-usb-bridge/build/custom-control-usb-bridge \
  --serial /dev/ttyUSB0 \
  --baud 115200 \
  --robot-id 2 \
  --verbose-payload
```

## 5. 参数说明

- `--serial`：串口设备路径，必填
- `--baud`：波特率，默认 `115200`
- `--data-bits`：数据位，支持 `7` 或 `8`
- `--parity`：校验位，支持 `none`、`even`、`odd`
- `--stop-bits`：停止位，支持 `1` 或 `2`
- `--mqtt-host`：默认 `192.168.12.1`
- `--mqtt-port`：默认 `3333`
- `--robot-id`：必填；红方工程机器人填 `2`，蓝方工程机器人填 `102`
- `--cmd-id`：串口输入帧里的命令码，默认 `0x0302`
- `--crc16-mode`：默认 `auto`
- `--drop-bad-crc16`：打开后，`CRC16` 失败的帧直接丢弃

## 6. 关于 CRC16

- `CRC8` 已按官方 `A5` 帧头规则校验
- `CRC16` 默认使用 `auto` 模式，会尝试几种常见算法自动匹配
- 如果没匹配上，默认会打印警告但继续转发，以免因为现场设备实现差异导致完全不出控制
- 如果你确认设备侧 `CRC16` 已经完全对齐，可以加 `--drop-bad-crc16` 切到严格模式

## 7. 运行时你应该看到什么

正常启动后，通常会看到：

- `MQTT 连接成功，准备发送 CustomControl`
- `串口打开成功，开始接收 USB 控制器数据`
- `已发布 CustomControl...`

如果串口参数不对，常见现象是：

- 一直在报 `CRC8` 失败
- 一直扫不到有效 `A5` 帧
- `CRC16` 持续失败

这时优先检查：

- 控制器实际波特率
- `8N1` 还是其他串口格式
- USB 转串口芯片对应设备名是否正确
