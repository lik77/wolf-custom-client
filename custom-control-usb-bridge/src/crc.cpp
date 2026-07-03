#include "crc.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace {

std::uint16_t reflect16(std::uint16_t value) {
    std::uint16_t result = 0;
    for (int bit = 0; bit < 16; ++bit) {
        if ((value & (1U << bit)) != 0U) {
            result |= static_cast<std::uint16_t>(1U << (15 - bit));
        }
    }
    return result;
}

std::uint16_t crc16Normal(const std::uint8_t* data, std::size_t size, std::uint16_t poly, std::uint16_t init, std::uint16_t xorOut) {
    std::uint16_t crc = init;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= static_cast<std::uint16_t>(data[index]) << 8U;
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000U) != 0U) {
                crc = static_cast<std::uint16_t>((crc << 1U) ^ poly);
            } else {
                crc <<= 1U;
            }
        }
    }
    return static_cast<std::uint16_t>(crc ^ xorOut);
}

std::uint16_t crc16Reflected(const std::uint8_t* data, std::size_t size, std::uint16_t poly, std::uint16_t init, std::uint16_t xorOut) {
    std::uint16_t crc = init;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = static_cast<std::uint16_t>((crc >> 1U) ^ poly);
            } else {
                crc >>= 1U;
            }
        }
    }
    return static_cast<std::uint16_t>(crc ^ xorOut);
}

std::uint16_t byteSwap16(std::uint16_t value) {
    return static_cast<std::uint16_t>((value >> 8U) | (value << 8U));
}

}  // namespace

std::uint8_t crc8Robomaster(const std::uint8_t* data, std::size_t size) {
    std::uint8_t crc = 0xFF;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x01U) != 0U) {
                crc = static_cast<std::uint8_t>((crc >> 1U) ^ 0x8CU);
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

std::uint16_t crc16RoboMaster(const std::uint8_t* data, std::size_t size) {
    return crc16Reflected(data, size, 0x8408U, 0xFFFFU, 0x0000U);
}

std::uint16_t crc16X25(const std::uint8_t* data, std::size_t size) {
    return crc16Reflected(data, size, 0x8408U, 0xFFFFU, 0xFFFFU);
}

std::uint16_t crc16CcittFalse(const std::uint8_t* data, std::size_t size) {
    return crc16Normal(data, size, 0x1021U, 0xFFFFU, 0x0000U);
}

std::uint16_t crc16XModem(const std::uint8_t* data, std::size_t size) {
    return crc16Normal(data, size, 0x1021U, 0x0000U, 0x0000U);
}

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size) {
    return crc16Reflected(data, size, 0xA001U, 0xFFFFU, 0x0000U);
}

std::uint16_t crc16Value(Crc16Mode mode, const std::uint8_t* data, std::size_t size) {
    switch (mode) {
        case Crc16Mode::RoboMaster:
            return crc16RoboMaster(data, size);
        case Crc16Mode::X25:
            return crc16X25(data, size);
        case Crc16Mode::CcittFalse:
            return crc16CcittFalse(data, size);
        case Crc16Mode::XModem:
            return crc16XModem(data, size);
        case Crc16Mode::Modbus:
            return crc16Modbus(data, size);
        case Crc16Mode::None:
        case Crc16Mode::Auto:
            break;
    }
    throw std::runtime_error("CRC16 mode does not map to a concrete algorithm");
}

bool crc16Matches(Crc16Mode mode, const std::uint8_t* data, std::size_t size, std::uint16_t storedLittleEndian) {
    const std::uint16_t computed = crc16Value(mode, data, size);
    return computed == storedLittleEndian || computed == byteSwap16(storedLittleEndian) || reflect16(computed) == storedLittleEndian;
}

bool parseCrc16Mode(const std::string& text, Crc16Mode* mode) {
    if (text == "auto") {
        *mode = Crc16Mode::Auto;
        return true;
    }
    if (text == "robomaster" || text == "rm") {
        *mode = Crc16Mode::RoboMaster;
        return true;
    }
    if (text == "x25") {
        *mode = Crc16Mode::X25;
        return true;
    }
    if (text == "ccitt-false") {
        *mode = Crc16Mode::CcittFalse;
        return true;
    }
    if (text == "xmodem") {
        *mode = Crc16Mode::XModem;
        return true;
    }
    if (text == "modbus") {
        *mode = Crc16Mode::Modbus;
        return true;
    }
    if (text == "none") {
        *mode = Crc16Mode::None;
        return true;
    }
    return false;
}

const char* crc16ModeName(Crc16Mode mode) {
    switch (mode) {
        case Crc16Mode::Auto:
            return "auto";
        case Crc16Mode::RoboMaster:
            return "robomaster";
        case Crc16Mode::X25:
            return "x25";
        case Crc16Mode::CcittFalse:
            return "ccitt-false";
        case Crc16Mode::XModem:
            return "xmodem";
        case Crc16Mode::Modbus:
            return "modbus";
        case Crc16Mode::None:
            return "none";
    }
    return "unknown";
}
