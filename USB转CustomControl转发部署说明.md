# USB 转 CustomControl 转发部署说明

本文档说明的是这一条链路：

- 自定义控制器通过 `RS232 -> USB` 接到客户端电脑
- 客户端电脑从串口接收控制器发来的 `39` 字节官方格式帧
- 客户端提取中间 `30` 字节有效载荷
- 客户端通过 `MQTT` 发布到官方 `CustomControl`
- 官方链路再把这条 `CustomControl` 转成 `0x0311` 下发给工程机器人

这条链路不修改现有 `rm-custom-client`，使用独立工具目录：`custom-control-usb-bridge`

## 1. 现在已经确认的协议参数

### 1.1 串口参数

- 波特率：`115200`
- 数据位：`8`
- 校验位：`none`
- 停止位：`1`

也就是标准 `115200 8N1`。

### 1.2 控制器发送到客户端的输入帧格式

控制器串口输出的是整帧 `39` 字节，不是裸 `30` 字节 payload。

结构如下：

```text
buf[0]     = 0xA5
buf[1..2]  = payload length，小端，固定 30
buf[3]     = seq
buf[4]     = CRC8(header)
buf[5..6]  = cmd_id，小端，当前按发送端代码为 0x0302
buf[7..36] = 30 字节 payload
buf[37..38]= CRC16(frame)
```

客户端并不会把这 `39` 字节整帧原样发给官方服务器。

客户端真正发到 `CustomControl` 的，是中间这 `30` 字节 payload。

### 1.3 官方下发到机器人的形式

- 客户端 MQTT Topic：`CustomControl`
- 客户端发布的是：`message CustomControl { bytes data = 1; }`
- `data` 里放的是控制器帧中的 `30` 字节 payload
- 官方链路后续会把这条 `CustomControl` 转成 `0x0311` 下发给工程机器人

所以：

- 串口输入帧命令码：当前是 `0x0302`
- 官方最终下发给机器人的命令码：是 `0x0311`

这两个不是同一个概念。

## 2. 工程机器人 robot-id

- 红方工程机器人：`2`
- 蓝方工程机器人：`102`

运行工具时必须显式传 `--robot-id`，不要省略。

## 3. 工具位置

独立工具目录：

```text
custom-control-usb-bridge
```

主要作用：

- 打开 `/dev/ttyUSB0` 之类的串口设备
- 自己扫描 `A5` 帧头
- 自己校验 `CRC8`
- 提取 `30` 字节 payload
- 发布 MQTT `CustomControl`

## 4. 安装依赖

在 Ubuntu 上安装：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config protobuf-compiler libprotobuf-dev \
  libpaho-mqttpp3-dev libpaho-mqtt-dev
```

## 5. 编译

在项目根目录执行：

```bash
cmake -S custom-control-usb-bridge -B custom-control-usb-bridge/build
cmake --build custom-control-usb-bridge/build -j
```

编译完成后，可执行文件是：

```text
custom-control-usb-bridge/build/custom-control-usb-bridge
```

## 6. 先确认 USB 串口设备名

插上控制器后，先看设备名：

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

常见情况会出现：

- `/dev/ttyUSB0`
- `/dev/ttyUSB1`
- `/dev/ttyACM0`

如果不确定最新插入的是哪个，可以在插拔前后分别执行一次：

```bash
dmesg | tail
```

## 7. 运行前的网络前提

客户端电脑仍然要满足官方网络要求：

- 官方选手端主机：`192.168.12.1/24`
- 客户端电脑：`192.168.12.2/24`
- 客户端通过 `MQTT` 连接 `192.168.12.1:3333`

可以先检查：

```bash
ping 192.168.12.1
nc -vz 192.168.12.1 3333
```

## 8. 运行命令

### 8.1 红方工程机器人

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

### 8.2 蓝方工程机器人

```bash
./custom-control-usb-bridge/build/custom-control-usb-bridge \
  --serial /dev/ttyUSB0 \
  --baud 115200 \
  --data-bits 8 \
  --parity none \
  --stop-bits 1 \
  --mqtt-host 192.168.12.1 \
  --mqtt-port 3333 \
  --robot-id 102
```

## 9. 如果要看每次转发的 payload

可以加：

```bash
--verbose-payload
```

完整例子：

```bash
./custom-control-usb-bridge/build/custom-control-usb-bridge \
  --serial /dev/ttyUSB0 \
  --baud 115200 \
  --data-bits 8 \
  --parity none \
  --stop-bits 1 \
  --mqtt-host 192.168.12.1 \
  --mqtt-port 3333 \
  --robot-id 2 \
  --verbose-payload
```

这时日志里会打印每次发出去的 `30` 字节十六进制内容。

## 10. 程序运行后应该看到什么

正常情况下应看到类似日志：

- `MQTT 连接成功，准备发送 CustomControl`
- `串口打开成功，开始接收 USB 控制器数据`
- `已发布 CustomControl...`

这说明：

- 串口已经读到控制器数据
- `A5` 帧已经被识别
- `30` 字节 payload 已经通过 MQTT 发出

## 11. 最常见问题

### 11.1 程序启动后没有任何转发日志

优先检查：

- 串口设备名是否写对，例如是不是其实是 `/dev/ttyUSB1`
- 控制器是否真的在持续输出数据
- USB 转串口线是否正常

### 11.2 一直报帧头或 CRC 错误

优先检查：

- 串口参数是否真的是 `115200 8N1`
- 控制器输出是否确实是这套 `39` 字节官方格式帧
- 串口线上是否有粘包、乱码、接线问题

### 11.3 能读到串口，但机器人没反应

优先检查：

- `--robot-id` 是否正确，红方工程是 `2`，蓝方工程是 `102`
- 官方选手端和客户端是否登录到同一台工程机器人
- `192.168.12.1:3333` 是否真的连通

### 11.4 想确认客户端到底发了什么

运行时加：

```bash
--verbose-payload
```

就能直接看到发到 `CustomControl.data` 的 `30` 字节 hex。

## 12. 当前实现的注意事项

- 现阶段工具默认按输入帧命令码 `0x0302` 解析
- 如果现场输入帧命令码不是 `0x0302`，可以改启动参数 `--cmd-id`
- 现阶段 `CRC16` 仍保留兼容模式，避免现场实现差异导致直接丢控制帧

如果后续你补充 `Append_CRC16_Check_Sum` 的具体实现，就可以把校验进一步收紧成严格官方版本。
