#include "serial_port.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t baudToConstant(int baudRate) {
    switch (baudRate) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 921600:
            return B921600;
#ifdef B1000000
        case 1000000:
            return B1000000;
#endif
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

tcflag_t dataBitsFlag(int dataBits) {
    switch (dataBits) {
        case 7:
            return CS7;
        case 8:
            return CS8;
        default:
            throw std::runtime_error("Unsupported data bits: " + std::to_string(dataBits));
    }
}

}  // namespace

bool parseSerialParity(const std::string& text, SerialParity* parity) {
    if (text == "none") {
        *parity = SerialParity::None;
        return true;
    }
    if (text == "even") {
        *parity = SerialParity::Even;
        return true;
    }
    if (text == "odd") {
        *parity = SerialParity::Odd;
        return true;
    }
    return false;
}

const char* serialParityName(SerialParity parity) {
    switch (parity) {
        case SerialParity::None:
            return "none";
        case SerialParity::Even:
            return "even";
        case SerialParity::Odd:
            return "odd";
    }
    return "unknown";
}

SerialPort::SerialPort(const SerialSettings& settings) {
    fd_ = open(settings.devicePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open serial device " + settings.devicePath + ": " + std::strerror(errno));
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
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= dataBitsFlag(settings.dataBits);

    switch (settings.parity) {
        case SerialParity::None:
            tty.c_cflag &= ~PARENB;
            tty.c_iflag &= ~INPCK;
            break;
        case SerialParity::Even:
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            tty.c_iflag |= INPCK;
            break;
        case SerialParity::Odd:
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            tty.c_iflag |= INPCK;
            break;
    }

    if (settings.stopBits == 2) {
        tty.c_cflag |= CSTOPB;
    } else if (settings.stopBits == 1) {
        tty.c_cflag &= ~CSTOPB;
    } else {
        close(fd_);
        fd_ = -1;
        throw std::runtime_error("Unsupported stop bits: " + std::to_string(settings.stopBits));
    }

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    const speed_t speed = baudToConstant(settings.baudRate);
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

int SerialPort::readSome(std::uint8_t* data, std::size_t size, int timeoutMs) {
    pollfd descriptor {};
    descriptor.fd = fd_;
    descriptor.events = POLLIN;

    while (true) {
        const int pollResult = poll(&descriptor, 1, timeoutMs);
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("poll failed: " + std::string(std::strerror(errno)));
        }
        if (pollResult == 0) {
            return 0;
        }
        if ((descriptor.revents & POLLIN) == 0) {
            return 0;
        }

        const ssize_t readCount = read(fd_, data, size);
        if (readCount < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            throw std::runtime_error("Serial read failed: " + std::string(std::strerror(errno)));
        }
        return static_cast<int>(readCount);
    }
}
