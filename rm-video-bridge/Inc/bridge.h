#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t uart_bytes_in;
    uint32_t packets_ok;
    uint32_t packets_crc_error;
    uint32_t packets_length_error;
    uint32_t packets_queue_dropped;
    uint32_t packets_sent_0310;
    uint32_t packets_send_failed;
} bridge_stats_t;

typedef struct {
    uint8_t rx_dma_buffer[2048];
    uint8_t parse_buffer[300];
    uint8_t queue_storage[24][300];
    uint16_t queue_lengths[24];
    uint8_t queue_priority[24];
    uint8_t queue_head;
    uint8_t queue_tail;
    uint8_t queue_size;
    uint16_t parse_size;
    size_t rx_read_index;
    uint32_t last_send_ms;
    bridge_stats_t stats;
} bridge_context_t;

void bridge_init(bridge_context_t* context);
void bridge_poll(bridge_context_t* context);
void bridge_on_20ms_tick(bridge_context_t* context);
const bridge_stats_t* bridge_stats(const bridge_context_t* context);

#endif
