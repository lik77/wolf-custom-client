# 客户端异机部署命令流程

本文档只说明：把 `rm-custom-client` 部署到另一台 Ubuntu 电脑时，如何连到 `192.168.12.1:3333`，以及如何接收官方 `UDP 3334` 图传。

## 1. 网络拓扑

- 官方选手端主机网卡固定为 `192.168.12.1/24`
- 你的客户端电脑网卡建议固定为 `192.168.12.2/24`
- 两台机器使用网线直连，或接到同一个二层交换网络
- `3333` 用于客户端主动连接官方主机的 `MQTT`
- `3334/udp` 用于客户端被动监听官方图传码流

## 2. 先确认客户端电脑网卡名

```bash
ip addr
一般第三个是
```

先找到你实际要接官方选手端的网卡名，例如可能是 `enp3s0`、`eth0`、`enx...`。

下面命令中的 `<网卡名>` 都替换成你的实际网卡名。

## 3. 给客户端电脑配置固定 IP

临时配置方式：

```bash
sudo ip addr flush dev <网卡名>
sudo ip addr add 192.168.12.2/24 dev <网卡名>
sudo ip link set <网卡名> up
```

确认配置成功：

```bash
ip addr show dev <网卡名>
```

你应该能看到类似：

```text
inet 192.168.12.2/24
```

## 4. 先测试能否连到 `192.168.12.1`

```bash
ping 192.168.12.1
```

如果 `ping` 不通，先不要启动客户端，优先检查：

- 网线是否接对
- 官方选手端对应网卡是否真的是 `192.168.12.1`
- 两边是否在同一网段

## 5. 安装客户端依赖

如果另一台电脑还没有编译环境，先安装：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config protobuf-compiler libprotobuf-dev \
  qt6-base-dev libavcodec-dev libavutil-dev libswscale-dev \
  libpaho-mqttpp3-dev libpaho-mqtt-dev
```

如果系统使用 `Qt5`，把 `qt6-base-dev` 换成对应的 `Qt5` 开发包。

## 6. 编译客户端

在项目根目录执行：

```bash
cmake -S rm-custom-client -B rm-custom-client/build
cmake --build rm-custom-client/build -j
```

如果你已经把可执行文件拷过去了，这一步可以跳过。

## 7. 检查 `3333` 端口是否可连

`MQTT` 是客户端主动连接官方主机的 `TCP 3333`，可以先用下面命令试连：

```bash
nc -vz 192.168.12.1 3333
```

如果看到 `succeeded` 或 `open`，说明到 `3333` 的基本连通性没问题。

如果没有 `nc`，先安装：

```bash
sudo apt install -y netcat-openbsd
```

## 8. 启动客户端

进入项目根目录后执行：

```bash
./rm-custom-client/build/rm-custom-client --mqtt-host 192.168.12.1 --mqtt-port 3333 --udp-port 3334 --robot-id 1
```

参数含义：

- `--mqtt-host 192.168.12.1`：连接官方选手端主机
- `--mqtt-port 3333`：连接官方 `MQTT` 端口
- `--udp-port 3334`：本机监听官方图传 `UDP` 端口
- `--robot-id 1`：客户端 `clientID`，红方英雄常用 `1`，蓝方英雄常用 `11`

## 9. `3334/udp` 监听的正确理解

`3334` 不是你手工再开一个单独进程去收，而是 `rm-custom-client` 启动后自动监听：

```bash
./rm-custom-client/build/rm-custom-client --mqtt-host 192.168.12.1 --mqtt-port 3333 --udp-port 3334 --robot-id 1
```

也就是说：

- 连 `192.168.12.1:3333` 和监听本机 `3334/udp` 是同一个启动命令完成的
- 只要客户端程序起来了，就已经在做这两件事

## 10. 如何确认本机真的在监听 `3334`

客户端启动后，另开一个终端执行：

```bash
ss -lunp | grep 3334
```

正常情况下应能看到本机 `*:3334` 或 `0.0.0.0:3334` 的监听信息。

## 11. 如何确认官方图传真的有发到 `3334`

先满足前提：官方选手端本身必须已经正常显示图传画面，否则通常不会向外发 `UDP 3334` 码流。

然后你可以看客户端日志，是否出现类似信息：

- 已收到官方图传 `UDP` 包
- 已开始监听 `UDP 3334`
- `HEVC` 解码成功

如果想在系统层确认有没有收到包，可以执行：

```bash
sudo tcpdump -ni <网卡名> udp port 3334
```

如果这里完全抓不到包，优先检查：

- 官方选手端当前是否真的已经出图传
- 客户端电脑 IP 是否真的是 `192.168.12.2`
- 防火墙是否拦截了 `3334/udp`

## 12. 防火墙排查

如果系统开了 `ufw`，可以先检查：

```bash
sudo ufw status
```

必要时放行 `3334/udp`：

```bash
sudo ufw allow 3334/udp
```

如果还怀疑 `3333` 被拦截，也可以放行：

```bash
sudo ufw allow 3333/tcp
```

## 13. 最短命令流程

如果你已经装好依赖、编译也没问题，最短流程就是：

```bash
ip addr
sudo ip addr flush dev <网卡名>
sudo ip addr add 192.168.12.2/24 dev <网卡名>
sudo ip link set <网卡名> up
ping 192.168.12.1
nc -vz 192.168.12.1 3333
./rm-custom-client/build/rm-custom-client --mqtt-host 192.168.12.1 --mqtt-port 3333 --udp-port 3334 --robot-id 1
```

## 14. 最常见的三类问题

### 14.1 `3333` 连不上

优先检查：

- `192.168.12.1` 能不能 `ping` 通
- `robot-id` 是否填对
- 官方选手端和客户端是否登录同一台机器人

### 14.2 能连上 `3333`，但没有 `3334` 画面

优先检查：

- 官方选手端本机画面是否已经正常显示
- `ss -lunp | grep 3334` 是否确认程序已监听
- `sudo tcpdump -ni <网卡名> udp port 3334` 是否抓得到包

### 14.3 程序能启动，但另一台电脑跑不起来

优先检查：

- 是否缺少 `Qt`、`FFmpeg`、`protobuf`、`paho mqtt` 运行库
- 是否是在带桌面环境的 Ubuntu 上运行图形界面程序
- 是否和当前编译机器架构一致
