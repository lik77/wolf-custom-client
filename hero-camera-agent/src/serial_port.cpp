#include "serial_port.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t baudToConstant(int baudRate) {
    switch (baudRate) {
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 921600:
            return B921600;
#ifdef B1500000
        case 1500000:
            return B1500000;
#endif
#ifdef B2000000
        case 2000000:
            return B2000000;
#endif
        default:
            throw std::runtime_error("Unsupported baud rate: " + std::to_string(baudRate));
    }
}

}  // namespace

SerialPort::SerialPort(const std::string& devicePath, int baudRate) {
    fd_ = open(devicePath.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open serial device " + devicePath + ": " + std::strerror(errno));
    }

    termios tty {};
    if (tcgetattr(fd_, &tty) != 0) {
        const std::string message = "tcgetattr failed: " + std::string(std::strerror(errno));
        close(fd_);
        fd_ = -1;
        throw std::runtime_error(message);
    }

    cfmakeraw(&tty);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    const speed_t speed = baudToConstant(baudRate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        const std::string message = "tcsetattr failed: " + std::string(std::strerror(errno));
        close(fd_);
        fd_ = -1;
        throw std::runtime_error(message);
    }

    tcflush(fd_, TCIOFLUSH);
}

SerialPort::~SerialPort() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void SerialPort::writeAll(const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t written = write(fd_, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("Serial write failed: " + std::string(std::strerror(errno)));
        }
        offset += static_cast<std::size_t>(written);
    }
}
