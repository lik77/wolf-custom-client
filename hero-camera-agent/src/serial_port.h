#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class SerialPort {
public:
    SerialPort(const std::string& devicePath, int baudRate);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    void writeAll(const std::uint8_t* data, std::size_t size);

private:
    int fd_ = -1;
};
