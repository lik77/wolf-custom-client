#include "crc16.h"

uint16_t crc16_ccitt_false(const uint8_t* data, size_t size) {
    uint16_t crc = 0xFFFFu;
    size_t index;
    for (index = 0; index < size; ++index) {
        crc ^= (uint16_t)data[index] << 8u;
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1u) ^ 0x1021u);
            } else {
                crc <<= 1u;
            }
        }
    }
    return crc;
}
