#include "bridge.h"

#include <stddef.h>
#include <string.h>

#include "bridge_platform.h"
#include "crc16.h"
#include "hero_protocol.h"

enum {
    BRIDGE_QUEUE_CAPACITY = 24,
    BRIDGE_HIGH_PRIORITY = 2,
    BRIDGE_MEDIUM_PRIORITY = 1,
    BRIDGE_LOW_PRIORITY = 0,
};

static void debug_log(const char* message) {
    bridge_platform_debug_log(message);
}

static uint8_t packet_priority(const hero_video_packet_header_v1_t* header) {
    switch ((hero_packet_type_t)header->packet_type) {
        case HERO_PACKET_RESET:
        case HERO_PACKET_CONFIG:
            return BRIDGE_HIGH_PRIORITY;
        case HERO_PACKET_HEARTBEAT:
            return BRIDGE_MEDIUM_PRIORITY;
        case HERO_PACKET_STREAM_CHUNK:
        default:
            return BRIDGE_LOW_PRIORITY;
    }
}

static bool queue_push(bridge_context_t* context, const uint8_t* packet, uint16_t packet_size, uint8_t priority) {
    if (context->queue_size < BRIDGE_QUEUE_CAPACITY) {
        const uint8_t slot = context->queue_tail;
        memcpy(context->queue_storage[slot], packet, packet_size);
        context->queue_lengths[slot] = packet_size;
        context->queue_priority[slot] = priority;
        context->queue_tail = (uint8_t)((context->queue_tail + 1u) % BRIDGE_QUEUE_CAPACITY);
        ++context->queue_size;
        return true;
    }

    if (priority <= BRIDGE_LOW_PRIORITY) {
        ++context->stats.packets_queue_dropped;
        if (context->stats.packets_queue_dropped <= 3u || (context->stats.packets_queue_dropped % 50u) == 0u) {
            debug_log("[警告] 桥接发送队列已满，低优先级视频分片被丢弃");
        }
        return false;
    }

    uint8_t count = context->queue_size;
    uint8_t index = context->queue_head;
    while (count-- > 0u) {
        if (context->queue_priority[index] == BRIDGE_LOW_PRIORITY) {
            context->queue_lengths[index] = packet_size;
            context->queue_priority[index] = priority;
            memcpy(context->queue_storage[index], packet, packet_size);
            ++context->stats.packets_queue_dropped;
            debug_log("[警告] 桥接发送队列已满，已用高优先级包替换旧的视频分片");
            return true;
        }
        index = (uint8_t)((index + 1u) % BRIDGE_QUEUE_CAPACITY);
    }

    ++context->stats.packets_queue_dropped;
    debug_log("[警告] 桥接发送队列已满，当前高优先级包也无法入队");
    return false;
}

static bool queue_pop(bridge_context_t* context, const uint8_t** packet, uint16_t* packet_size) {
    if (context->queue_size == 0u) {
        return false;
    }
    const uint8_t slot = context->queue_head;
    *packet = context->queue_storage[slot];
    *packet_size = context->queue_lengths[slot];
    context->queue_head = (uint8_t)((context->queue_head + 1u) % BRIDGE_QUEUE_CAPACITY);
    --context->queue_size;
    return true;
}

static void consume_byte(bridge_context_t* context, uint8_t byte) {
    ++context->stats.uart_bytes_in;

    if (context->parse_size >= sizeof(context->parse_buffer)) {
        context->parse_size = 0u;
    }

    context->parse_buffer[context->parse_size++] = byte;

    if (context->parse_size == 1u && byte != (uint8_t)(HERO_MAGIC & 0xFFu)) {
        context->parse_size = 0u;
        return;
    }

    if (context->parse_size == 2u) {
        const uint16_t magic = (uint16_t)context->parse_buffer[0] | ((uint16_t)context->parse_buffer[1] << 8u);
        if (magic != HERO_MAGIC) {
            context->parse_buffer[0] = context->parse_buffer[1];
            context->parse_size = 1u;
            return;
        }
    }

    if (context->parse_size < HERO_HEADER_SIZE) {
        return;
    }

    {
        hero_video_packet_header_v1_t header;
        memcpy(&header, context->parse_buffer, sizeof(header));
        if (!hero_header_looks_valid(&header)) {
            ++context->stats.packets_length_error;
            if (context->stats.packets_length_error <= 3u || (context->stats.packets_length_error % 50u) == 0u) {
                debug_log("[错误] 串口收到的包头无效，长度或版本字段异常");
            }
            memmove(context->parse_buffer, context->parse_buffer + 1u, context->parse_size - 1u);
            --context->parse_size;
            return;
        }

        const uint16_t total_size = (uint16_t)(HERO_HEADER_SIZE + header.payload_len);
        if (context->parse_size < total_size) {
            return;
        }

        {
            const uint16_t expected_crc = header.packet_crc16;
            uint8_t crc_buffer[HERO_MAX_PACKET_SIZE - sizeof(uint16_t)];
            const uint16_t prefix_size = (uint16_t)offsetof(hero_video_packet_header_v1_t, packet_crc16);
            memcpy(crc_buffer, context->parse_buffer, prefix_size);
            memcpy(crc_buffer + prefix_size, context->parse_buffer + HERO_HEADER_SIZE, header.payload_len);
            const uint16_t actual_crc = crc16_ccitt_false(crc_buffer, (size_t)(prefix_size + header.payload_len));

            if (actual_crc != expected_crc) {
                ++context->stats.packets_crc_error;
                if (context->stats.packets_crc_error <= 3u || (context->stats.packets_crc_error % 50u) == 0u) {
                    debug_log("[错误] 串口包 CRC 校验失败，请检查波特率、地线或 DMA 写指针");
                }
                memmove(context->parse_buffer, context->parse_buffer + 1u, context->parse_size - 1u);
                --context->parse_size;
                return;
            }
        }

        if (queue_push(context, context->parse_buffer, total_size, packet_priority(&header))) {
            ++context->stats.packets_ok;
            if (context->stats.packets_ok == 1u) {
                debug_log("[信息] 已成功解析到第一包来自小电脑的视频数据");
            }
        }

        if (context->parse_size > total_size) {
            memmove(context->parse_buffer, context->parse_buffer + total_size, context->parse_size - total_size);
        }
        context->parse_size = (uint16_t)(context->parse_size - total_size);
    }
}

void bridge_init(bridge_context_t* context) {
    memset(context, 0, sizeof(*context));
    bridge_platform_uart_start_rx_dma(context->rx_dma_buffer, sizeof(context->rx_dma_buffer));
    context->last_send_ms = bridge_platform_tick_ms();
    debug_log("[信息] rm-video-bridge 初始化完成，已启动串口 DMA 接收");
}

void bridge_poll(bridge_context_t* context) {
    const size_t write_index = bridge_platform_uart_rx_write_index();
    while (context->rx_read_index != write_index) {
        consume_byte(context, context->rx_dma_buffer[context->rx_read_index]);
        context->rx_read_index = (context->rx_read_index + 1u) % sizeof(context->rx_dma_buffer);
    }
}

void bridge_on_20ms_tick(bridge_context_t* context) {
    const uint8_t* packet;
    uint16_t packet_size;

    context->last_send_ms = bridge_platform_tick_ms();
    if (!queue_pop(context, &packet, &packet_size)) {
        return;
    }

    if (bridge_platform_send_custom_0310(packet, packet_size)) {
        ++context->stats.packets_sent_0310;
        if (context->stats.packets_sent_0310 == 1u) {
            debug_log("[信息] 已成功发送第一包 0x0310 自定义数据");
        }
    } else {
        ++context->stats.packets_send_failed;
        if (context->stats.packets_send_failed <= 3u || (context->stats.packets_send_failed % 20u) == 0u) {
            debug_log("[错误] 发送 0x0310 失败，请检查裁判系统发送接口或链路状态");
        }
    }
}

const bridge_stats_t* bridge_stats(const bridge_context_t* context) {
    return &context->stats;
}
