# rm-video-bridge

`rm-video-bridge` 是运行在 `MCU` 电控侧的桥接模块，职责如下：

- 从小电脑串口接收 `hero-camera-agent` 发来的自定义视频包
- 做包头扫描、长度检查、`CRC16` 校验
- 将合法包排入待发送队列
- 以不高于 `50Hz` 的速率把整包塞入官方 `0x0310` 上行链路

## 需要你接入的板级适配函数

你需要在工程里实现下面 4 个函数：

- `bridge_platform_uart_start_rx_dma()`
- `bridge_platform_uart_rx_write_index()`
- `bridge_platform_tick_ms()`
- `bridge_platform_send_custom_0310()`

具体声明见 `Inc/bridge_platform.h`。

## 典型集成方式

1. 在串口 DMA 空闲中断或半满/满中断中持续更新 DMA 写指针
2. 主循环或任务中周期性调用 `bridge_poll()`
3. 每 `20ms` 调用 `bridge_on_20ms_tick()`

## 约束

- 每个自定义包总长度不得超过 `300` 字节
- 默认 `UART 8N1 1500000`
- 不做重传
- 队列拥塞时优先丢弃低优先级 `stream_chunk`

## 建议工程结构

将本目录中的文件合并到你的 `MCU` 工程中：

- `Inc/hero_protocol.h`
- `Inc/crc16.h`
- `Inc/bridge_platform.h`
- `Inc/bridge.h`
- `Src/crc16.c`
- `Src/bridge.c`

`Src/bridge_platform_stub.c` 只是示例占位文件，实际集成时请删除或改为你的板级实现。

## 建议串口配置

- 独立串口，不和调试口复用
- `8N1`
- 波特率先用 `1500000`
- `DMA` 循环接收
- 若硬件稳定性不足，可退到 `921600`

## 发送时序

- 小电脑端会连续发送自定义包
- `bridge_poll()` 负责从 `DMA` 环形缓冲区取字节并拼包
- `bridge_on_20ms_tick()` 负责按 `50Hz` 节奏发到官方 `0x0310`

推荐调用频率：

- `bridge_poll()`：主循环尽可能高频调用
- `bridge_on_20ms_tick()`：严格 `20ms` 一次

## 板级适配说明

### `bridge_platform_uart_start_rx_dma()`

- 在这里启动串口 `DMA` 循环接收
- `buffer` 就是桥接模块提供的接收缓冲区
- `size` 是缓冲区长度，当前为 `2048`

### `bridge_platform_uart_rx_write_index()`

- 返回当前 `DMA` 已经写到的下标
- 返回值必须是 `0 ~ size-1`
- 如果你使用 `DMA NDTR` 反推写指针，注意和环形长度对应

### `bridge_platform_tick_ms()`

- 返回系统毫秒计数
- 可以直接用 `HAL_GetTick()` 或等价实现

### `bridge_platform_send_custom_0310()`

- 把 `data[0..size-1]` 整包塞入官方 `0x0310`
- 这里的 `size` 已经包含自定义协议头和 payload
- 如果发送成功返回 `true`，失败返回 `false`

## 建议接法

1. 小电脑串口 TX -> `MCU` 对应 RX
2. 小电脑串口 GND -> `MCU` GND
3. 如果需要状态回传，再补 RX 方向

## 联调步骤

1. 不接裁判系统，先只验证 `DMA` 能否稳定收到连续数据
2. 统计 `packets_ok`、`packets_crc_error`、`packets_length_error`
3. CRC 和长度错误降到很低后，再接入 `0x0310`
4. 接入 Ubuntu 客户端后，观察 `CustomByteBlock` 是否持续到达

## 故障排查

- `CRC` 错误很多
  - 优先排查串口波特率和地线
  - 检查 `DMA` 写指针计算是否正确
  - 检查是否和调试日志复用了同一个串口

- `packets_ok` 有增长但画面不出
  - 检查 `bridge_platform_send_custom_0310()` 是否真的把整包发给了官方链路
  - 检查 Ubuntu 客户端是否已订阅 `CustomByteBlock`

- 队列堆积严重
  - 降低小电脑端 `target_bytes`
  - 降低分辨率或帧率
