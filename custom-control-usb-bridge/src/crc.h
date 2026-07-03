#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

enum class Crc16Mode {
    Auto,
    RoboMaster,
    X25,
    CcittFalse,
    XModem,
    Modbus,
    None,
};

std::uint8_t crc8Robomaster(const std::uint8_t* data, std::size_t size);
std::uint16_t crc16RoboMaster(const std::uint8_t* data, std::size_t size);
std::uint16_t crc16X25(const std::uint8_t* data, std::size_t size);
std::uint16_t crc16CcittFalse(const std::uint8_t* data, std::size_t size);
std::uint16_t crc16XModem(const std::uint8_t* data, std::size_t size);
std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size);

std::uint16_t crc16Value(Crc16Mode mode, const std::uint8_t* data, std::size_t size);
bool crc16Matches(Crc16Mode mode, const std::uint8_t* data, std::size_t size, std::uint16_t storedLittleEndian);
bool parseCrc16Mode(const std::string& text, Crc16Mode* mode);
const char* crc16ModeName(Crc16Mode mode);
