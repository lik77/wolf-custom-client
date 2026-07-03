#include "bridge_platform.h"

void bridge_platform_uart_start_rx_dma(uint8_t* buffer, size_t size) {
    (void)buffer;
    (void)size;
}

size_t bridge_platform_uart_rx_write_index(void) {
    return 0u;
}

uint32_t bridge_platform_tick_ms(void) {
    return 0u;
}

bool bridge_platform_send_custom_0310(const uint8_t* data, uint16_t size) {
    (void)data;
    (void)size;
    return false;
}

void bridge_platform_debug_log(const char* message) {
    (void)message;
}
