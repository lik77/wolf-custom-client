#ifndef BRIDGE_PLATFORM_H
#define BRIDGE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void bridge_platform_uart_start_rx_dma(uint8_t* buffer, size_t size);
size_t bridge_platform_uart_rx_write_index(void);
uint32_t bridge_platform_tick_ms(void);
bool bridge_platform_send_custom_0310(const uint8_t* data, uint16_t size);
void bridge_platform_debug_log(const char* message);

#endif
