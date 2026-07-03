# hero-camera-agent 调参建议

## 1. 调参原则

- 先锁定一组“能稳定出画面”的基线参数。
- 一次只改一个维度，不要同时改 3 个以上参数。
- 推荐顺序：先调亮度，再调流畅度，最后调清晰度。
- 一旦出现 `序号跳变`、`non-existing PPS`、`缓冲重置` 增多，优先回退最近一次调参。

## 2. 推荐基线参数

推荐优先修改 `hero-camera-agent/config/hero-camera-agent.yaml`，然后统一用下面这条命令启动：

```bash
./build/hero-camera-agent --config config/hero-camera-agent.yaml
```

如果你暂时还想直接用命令行，也可以先从下面这组开始：

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 8000 --max-stream-pps 40 --exposure-us 12000
```

这组参数的特点：

- 对链路压力较小
- 更容易稳定出画面
- 适合先验证 `MCU -> 0x0310 -> CustomByteBlock -> 客户端` 这条链路

## 3. 参数含义

### `--fps`

- 采集和编码帧率
- 越高越顺，但更容易增加链路压力和编码负担

### `--width --height`

- 编码前输出分辨率
- 越高越清晰，但需要更高码率

### `--target-bytes`

- 目标码率，单位 `Byte/s`
- 越高画面越清楚，但更容易让 `MCU` 队列积压

### `--max-stream-pps`

- 自研视频分片的最大发包速率，单位“包/秒”
- 越高越容易把 `MCU` 的 `0x0310` 节奏压满

### `--exposure-us`

- 相机曝光时间，单位微秒
- 越高越亮，但拖影和模糊风险也越大

## 4. 现象与调参建议

### 4.1 画面偏黑

先只调曝光，不动其他参数。

#### 推荐步骤

第一档：

```bash
--exposure-us 18000
```

第二档：

```bash
--exposure-us 22000
```

第三档：

```bash
--exposure-us 26000
```

#### 注意

- 如果亮了但拖影明显，说明曝光太长，要往回降。
- 一般建议优先试 `18000`，再试 `22000`。

## 4.2 画面卡顿

先判断是哪种卡：

- 只是画面不够顺：优先调 `fps`
- 明显丢帧/解码错误/序号跳变：优先降链路负载

### 链路不稳时，优先回退

```bash
--target-bytes 7000 --max-stream-pps 36
```

更保守：

```bash
--target-bytes 6500 --max-stream-pps 32
```

### 链路稳定，但想更顺一点

先试：

```bash
--fps 40
```

如果一改 `40fps` 就出问题，退回 `30fps`。

## 4.3 画面太糊

先提分辨率，再提码率。

### 第一步：提分辨率

```bash
--width 300 --height 300
```

### 第二步：提码率

```bash
--target-bytes 9000
```

进一步：

```bash
--target-bytes 10000
```

#### 注意

- 不建议一开始就同时改 `300x300 + 10000 + 40fps + 44pps`
- 这样很容易重新把链路压坏

## 4.4 拖影太重

优先降低曝光：

```bash
--exposure-us 15000
```

如果还重：

```bash
--exposure-us 12000
```

## 4.5 延迟偏大

通常先降下面两项：

```bash
--target-bytes 7000 --fps 30
```

如果当前 `fps` 已经不高，就优先降码率，不先降分辨率。

## 5. 三组推荐方案

### 方案 A：先解决偏黑

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 8000 --max-stream-pps 40 --exposure-us 18000
```

适合：

- 当前能出画面，但偏黑
- 想先把亮度提起来

### 方案 B：亮度可以，再提一点清晰度

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 300 --height 300 --target-bytes 9000 --max-stream-pps 40 --exposure-us 18000
```

适合：

- 链路已经稳定
- 想让画面更清楚

### 方案 C：链路不稳时回退

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 7000 --max-stream-pps 36 --exposure-us 18000
```

适合：

- 重新出现丢帧、跳变、解码异常
- `MCU` 队列开始吃紧

## 6. 推荐调参顺序

### 第一步

先试：

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 8000 --max-stream-pps 40 --exposure-us 18000
```

### 第二步

如果亮度够了，但还是糊：

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 300 --height 300 --target-bytes 9000 --max-stream-pps 40 --exposure-us 18000
```

### 第三步

如果链路又开始不稳，退回：

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 7000 --max-stream-pps 36 --exposure-us 18000
```

## 7. 现场观察要点

### 看小电脑端日志

重点看这些关键词：

- `当前待发送码流缓冲`
- `待发送码流积压过多`
- `缓冲重置`

如果出现：

- `缓冲重置 0 次`
  - 说明发送端比较稳定
- `缓冲重置` 开始增长
  - 说明参数过激了，要回退

### 看客户端日志

重点看这些：

- `已收到自研视频配置包`
- `已收到新的关键帧，自研视频开始恢复解码`
- `已成功解码图像帧`

如果开始频繁出现：

- `non-existing PPS`
- `decode_slice_header error`

通常说明最近一次参数调整把链路又压坏了，建议退回上一组。

## 8. 最后建议

- 先稳，再清楚
- 先亮，再顺
- 每次只改一个方向

当前最推荐先试的是：

```bash
./build/hero-camera-agent --serial /dev/ttyACM0 --baud 1500000 --source mvs --device-index 0 --fps 30 --width 256 --height 256 --target-bytes 8000 --max-stream-pps 40 --exposure-us 18000
```
