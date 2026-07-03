#include <google/protobuf/stubs/common.h>

#include <array>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "crc.h"
#include "frame_parser.h"
#include "mqtt_publisher.h"
#include "serial_port.h"

namespace {

std::atomic_bool gStopRequested = false;

struct Options {
    SerialSettings serial;
    std::string mqttHost = "192.168.12.1";
    int mqttPort = 3333;
    std::string robotId;
    std::string topic = "CustomControl";
    std::uint16_t commandId = 0x0302;
    int readTimeoutMs = 100;
    bool dropBadCrc16 = false;
    bool verbosePayload = false;
    Crc16Mode crc16Mode = Crc16Mode::Auto;
};

std::string nowString() {
    std::time_t now = std::time(nullptr);
    std::tm tmValue {};
    localtime_r(&now, &tmValue);
    std::ostringstream stream;
    stream << std::put_time(&tmValue, "%H:%M:%S");
    return stream.str();
}

void logLine(const std::string& message) {
    std::cout << '[' << nowString() << "] " << message << std::endl;
}

void handleSignal(int) {
    gStopRequested = true;
}

std::string bytesToHex(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index != 0) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<int>(bytes[index]);
    }
    return stream.str();
}

std::string hex16(std::uint16_t value) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value;
    return stream.str();
}

std::string requireValue(int argc, char* argv[], int* index) {
    if (*index + 1 >= argc) {
        throw std::runtime_error(std::string("Missing value for ") + argv[*index]);
    }
    ++(*index);
    return argv[*index];
}

int parseInt(const std::string& text, const std::string& name) {
    try {
        std::size_t parsedLength = 0;
        const int value = std::stoi(text, &parsedLength, 0);
        if (parsedLength != text.size()) {
            throw std::runtime_error("");
        }
        return value;
    } catch (...) {
        throw std::runtime_error("Invalid integer for " + name + ": " + text);
    }
}

std::uint16_t parseU16(const std::string& text, const std::string& name) {
    try {
        std::size_t parsedLength = 0;
        const unsigned long value = std::stoul(text, &parsedLength, 0);
        if (parsedLength != text.size() || value > 0xFFFFUL) {
            throw std::runtime_error("");
        }
        return static_cast<std::uint16_t>(value);
    } catch (...) {
        throw std::runtime_error("Invalid 16-bit integer for " + name + ": " + text);
    }
}

void printHelp() {
    std::cout
        << "custom-control-usb-bridge\n"
        << "\n"
        << "Reads 39-byte official control frames from a USB serial device,\n"
        << "extracts the 30-byte CustomControl payload, and publishes it to MQTT.\n"
        << "\n"
        << "Required:\n"
        << "  --serial <path>            Serial device path, e.g. /dev/ttyUSB0\n"
        << "\n"
        << "Common options:\n"
        << "  --baud <rate>             Default 115200\n"
        << "  --data-bits <7|8>         Default 8\n"
        << "  --parity <none|even|odd>  Default none\n"
        << "  --stop-bits <1|2>         Default 1\n"
        << "  --mqtt-host <host>        Default 192.168.12.1\n"
        << "  --mqtt-port <port>        Default 3333\n"
        << "  --robot-id <id>           Required. Red engineer=2, blue engineer=102\n"
        << "  --topic <name>            Default CustomControl\n"
        << "  --cmd-id <value>          Default 0x0302\n"
        << "  --crc16-mode <mode>       auto | robomaster | x25 | ccitt-false | xmodem | modbus | none\n"
        << "  --drop-bad-crc16          Drop frames whose CRC16 check fails\n"
        << "  --verbose-payload         Print payload hex on send\n"
        << "  --read-timeout-ms <ms>    Default 100\n"
        << "  --help                    Show this message\n";
}

Options parseArgs(int argc, char* argv[]) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help") {
            printHelp();
            std::exit(0);
        }
        if (argument == "--serial") {
            options.serial.devicePath = requireValue(argc, argv, &index);
            continue;
        }
        if (argument == "--baud") {
            options.serial.baudRate = parseInt(requireValue(argc, argv, &index), "--baud");
            continue;
        }
        if (argument == "--data-bits") {
            options.serial.dataBits = parseInt(requireValue(argc, argv, &index), "--data-bits");
            continue;
        }
        if (argument == "--parity") {
            const std::string value = requireValue(argc, argv, &index);
            if (!parseSerialParity(value, &options.serial.parity)) {
                throw std::runtime_error("Invalid parity: " + value);
            }
            continue;
        }
        if (argument == "--stop-bits") {
            options.serial.stopBits = parseInt(requireValue(argc, argv, &index), "--stop-bits");
            continue;
        }
        if (argument == "--mqtt-host") {
            options.mqttHost = requireValue(argc, argv, &index);
            continue;
        }
        if (argument == "--mqtt-port") {
            options.mqttPort = parseInt(requireValue(argc, argv, &index), "--mqtt-port");
            continue;
        }
        if (argument == "--robot-id" || argument == "--client-id") {
            options.robotId = requireValue(argc, argv, &index);
            continue;
        }
        if (argument == "--topic") {
            options.topic = requireValue(argc, argv, &index);
            continue;
        }
        if (argument == "--cmd-id") {
            options.commandId = parseU16(requireValue(argc, argv, &index), "--cmd-id");
            continue;
        }
        if (argument == "--crc16-mode") {
            const std::string value = requireValue(argc, argv, &index);
            if (!parseCrc16Mode(value, &options.crc16Mode)) {
                throw std::runtime_error("Invalid CRC16 mode: " + value);
            }
            continue;
        }
        if (argument == "--drop-bad-crc16") {
            options.dropBadCrc16 = true;
            continue;
        }
        if (argument == "--verbose-payload") {
            options.verbosePayload = true;
            continue;
        }
        if (argument == "--read-timeout-ms") {
            options.readTimeoutMs = parseInt(requireValue(argc, argv, &index), "--read-timeout-ms");
            continue;
        }

        throw std::runtime_error("Unknown argument: " + argument);
    }

    if (options.serial.devicePath.empty()) {
        throw std::runtime_error("--serial is required");
    }
    if (options.robotId.empty()) {
        throw std::runtime_error("--robot-id is required");
    }
    if (options.readTimeoutMs < 1) {
        throw std::runtime_error("--read-timeout-ms must be positive");
    }

    return options;
}

}  // namespace

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    try {
        const Options options = parseArgs(argc, argv);

        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        logLine("启动 custom-control-usb-bridge");
        logLine("串口=" + options.serial.devicePath +
                " baud=" + std::to_string(options.serial.baudRate) +
                " data=" + std::to_string(options.serial.dataBits) +
                " parity=" + std::string(serialParityName(options.serial.parity)) +
                " stop=" + std::to_string(options.serial.stopBits));
        logLine("MQTT=" + options.mqttHost + ':' + std::to_string(options.mqttPort) +
                " clientID=" + options.robotId +
                " topic=" + options.topic +
                " cmdId=" + hex16(options.commandId) +
                " crc16=" + crc16ModeName(options.crc16Mode));

        MqttPublisher publisher(options.mqttHost, options.mqttPort, options.robotId, options.topic);
        publisher.connect();
        logLine("MQTT 连接成功，准备发送 CustomControl");

        SerialPort serialPort(options.serial);
        logLine("串口打开成功，开始接收 USB 控制器数据");

        FrameParser parser({options.commandId, 30, options.crc16Mode, options.dropBadCrc16});
        std::array<std::uint8_t, 512> readBuffer {};
        std::size_t publishedCount = 0;

        while (!gStopRequested.load()) {
            const int readCount = serialPort.readSome(readBuffer.data(), readBuffer.size(), options.readTimeoutMs);
            if (readCount <= 0) {
                continue;
            }

            parser.append(readBuffer.data(), static_cast<std::size_t>(readCount));
            const std::vector<ParsedControlFrame> frames = parser.extractFrames([](const std::string& message) {
                logLine(message);
            });

            for (const ParsedControlFrame& frame : frames) {
                publisher.publishCustomControl(frame.payload);
                ++publishedCount;
                if (publishedCount <= 3 || publishedCount % 100 == 0 || options.verbosePayload) {
                    std::string message = "已发布 CustomControl，seq=" + std::to_string(frame.sequence) +
                                          " payload_len=" + std::to_string(frame.payload.size()) +
                                          " crc16=" + (frame.crc16Valid ? std::string("OK") : std::string("WARN"));
                    if (frame.crc16Mode != Crc16Mode::None) {
                        message += "(" + std::string(crc16ModeName(frame.crc16Mode)) + ')';
                    }
                    if (options.verbosePayload) {
                        message += " payload=" + bytesToHex(frame.payload);
                    }
                    logLine(message);
                }
            }
        }

        logLine("收到退出信号，程序结束");
        google::protobuf::ShutdownProtobufLibrary();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << '[' << nowString() << "] [错误] " << exception.what() << std::endl;
        google::protobuf::ShutdownProtobufLibrary();
        return 1;
    }
}
