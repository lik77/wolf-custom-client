#include "frame_parser.h"

#include <algorithm>
#include <sstream>

namespace {

std::uint16_t littleEndian16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1]) << 8U);
}

std::string hex16(std::uint16_t value) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase;
    if (value < 0x1000U) {
        stream << '0';
    }
    if (value < 0x100U) {
        stream << '0';
    }
    if (value < 0x10U) {
        stream << '0';
    }
    stream << value;
    return stream.str();
}

std::vector<Crc16Mode> candidateModes() {
    return {
        Crc16Mode::RoboMaster,
        Crc16Mode::X25,
        Crc16Mode::CcittFalse,
        Crc16Mode::XModem,
        Crc16Mode::Modbus,
    };
}

}  // namespace

FrameParser::FrameParser(FrameParserConfig config) : config_(config) {}

void FrameParser::append(const std::uint8_t* data, std::size_t size) {
    buffer_.insert(buffer_.end(), data, data + size);
}

std::vector<ParsedControlFrame> FrameParser::extractFrames(const Logger& logger) {
    std::vector<ParsedControlFrame> frames;

    while (true) {
        if (buffer_.size() < 5) {
            break;
        }

        const auto sofIt = std::find(buffer_.begin(), buffer_.end(), static_cast<std::uint8_t>(0xA5));
        if (sofIt != buffer_.begin()) {
            const std::size_t noiseBytes = static_cast<std::size_t>(std::distance(buffer_.begin(), sofIt));
            skippedNoiseCount_ += noiseBytes;
            logOccasional(logger, skippedNoiseCount_, "已跳过串口噪声字节，继续扫描 A5 帧头");
            buffer_.erase(buffer_.begin(), sofIt);
            if (buffer_.size() < 5) {
                break;
            }
        }

        const std::uint16_t payloadLength = littleEndian16(buffer_.data() + 1);
        if (payloadLength > config_.maxPayloadBytes) {
            ++badLengthCount_;
            logOccasional(logger,
                          badLengthCount_,
                          "丢弃疑似错误帧：长度字段为 " + std::to_string(payloadLength) + "，超过 CustomControl 上限");
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::uint8_t expectedHeaderCrc = crc8Robomaster(buffer_.data(), 4);
        if (buffer_[4] != expectedHeaderCrc) {
            ++badHeaderCount_;
            logOccasional(logger, badHeaderCount_, "丢弃疑似错误帧：CRC8 头校验失败");
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::size_t frameSize = 5U + 2U + payloadLength + 2U;
        if (buffer_.size() < frameSize) {
            break;
        }

        const std::uint16_t commandId = littleEndian16(buffer_.data() + 5);
        if (commandId != config_.commandId) {
            ++badCommandCount_;
            logOccasional(logger,
                          badCommandCount_,
                          "丢弃非目标命令帧：命令码为 " + hex16(commandId) + "，目标为 " + hex16(config_.commandId));
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::uint16_t storedCrc16 = littleEndian16(buffer_.data() + 7 + payloadLength);
        Crc16Mode matchedMode = Crc16Mode::None;
        const bool crc16Valid = verifyCrc16(buffer_.data(), 7U + payloadLength, storedCrc16, &matchedMode, logger);
        if (!crc16Valid) {
            ++badCrc16Count_;
            logOccasional(logger,
                          badCrc16Count_,
                          config_.dropBadCrc16
                              ? "丢弃帧：CRC16 校验失败"
                              : "CRC16 校验失败，当前按宽松模式继续转发 payload");
            if (config_.dropBadCrc16) {
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frameSize));
                continue;
            }
        }

        ParsedControlFrame frame;
        frame.sequence = buffer_[3];
        frame.commandId = commandId;
        frame.payload.assign(buffer_.begin() + 7, buffer_.begin() + static_cast<std::ptrdiff_t>(7U + payloadLength));
        frame.crc16Valid = crc16Valid;
        frame.crc16Mode = matchedMode;
        frames.push_back(std::move(frame));

        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frameSize));
    }

    return frames;
}

bool FrameParser::verifyCrc16(const std::uint8_t* frameData,
                              std::size_t crcRegionSize,
                              std::uint16_t storedLittleEndian,
                              Crc16Mode* matchedMode,
                              const Logger& logger) {
    if (config_.crc16Mode == Crc16Mode::None) {
        *matchedMode = Crc16Mode::None;
        return true;
    }

    if (config_.crc16Mode != Crc16Mode::Auto) {
        *matchedMode = config_.crc16Mode;
        return crc16Matches(config_.crc16Mode, frameData, crcRegionSize, storedLittleEndian);
    }

    if (detectedCrc16Mode_.has_value()) {
        *matchedMode = *detectedCrc16Mode_;
        return crc16Matches(*detectedCrc16Mode_, frameData, crcRegionSize, storedLittleEndian);
    }

    for (const Crc16Mode mode : candidateModes()) {
        if (crc16Matches(mode, frameData, crcRegionSize, storedLittleEndian)) {
            detectedCrc16Mode_ = mode;
            *matchedMode = mode;
            if (logger) {
                logger(std::string("已自动识别 CRC16 算法：") + crc16ModeName(mode));
            }
            return true;
        }
    }

    *matchedMode = Crc16Mode::None;
    return false;
}

void FrameParser::logOccasional(const Logger& logger, std::size_t count, const std::string& message) const {
    if (!logger) {
        return;
    }
    if (count <= 3 || count % 100 == 0) {
        logger(message + "（累计 " + std::to_string(count) + " 次）");
    }
}
