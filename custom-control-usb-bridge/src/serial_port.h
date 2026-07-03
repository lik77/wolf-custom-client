#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

enum class SerialParity {
    None,
    Even,
    Odd,
};

struct SerialSettings {
    std::string devicePath;
    int baudRate = 115200;
    int dataBits = 8;
    SerialParity parity = SerialParity::None;
    int stopBits = 1;
};

bool parseSerialParity(const std::string& text, SerialParity* parity);
const char* serialParityName(SerialParity parity);

class SerialPort {
public:
    explicit SerialPort(const SerialSettings& settings);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    int readSome(std::uint8_t* data, std::size_t size, int timeoutMs);

private:
    int fd_ = -1;
};
