#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "crc.h"

struct FrameParserConfig {
    std::uint16_t commandId = 0x0306;
    std::size_t maxPayloadBytes = 30;
    Crc16Mode crc16Mode = Crc16Mode::Auto;
    bool dropBadCrc16 = false;
};

struct ParsedControlFrame {
    std::uint8_t sequence = 0;
    std::uint16_t commandId = 0;
    std::vector<std::uint8_t> payload;
    bool crc16Valid = false;
    Crc16Mode crc16Mode = Crc16Mode::None;
};

class FrameParser {
public:
    using Logger = std::function<void(const std::string&)>;

    explicit FrameParser(FrameParserConfig config);

    void append(const std::uint8_t* data, std::size_t size);
    std::vector<ParsedControlFrame> extractFrames(const Logger& logger);

private:
    bool verifyCrc16(const std::uint8_t* frameData,
                     std::size_t crcRegionSize,
                     std::uint16_t storedLittleEndian,
                     Crc16Mode* matchedMode,
                     const Logger& logger);
    void logOccasional(const Logger& logger, std::size_t count, const std::string& message) const;

    FrameParserConfig config_;
    std::vector<std::uint8_t> buffer_;
    std::optional<Crc16Mode> detectedCrc16Mode_;
    std::size_t skippedNoiseCount_ = 0;
    std::size_t badHeaderCount_ = 0;
    std::size_t badLengthCount_ = 0;
    std::size_t badCommandCount_ = 0;
    std::size_t badCrc16Count_ = 0;
};
