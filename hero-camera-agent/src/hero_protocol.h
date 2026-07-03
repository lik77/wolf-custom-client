#pragma once

#include <cstddef>
#include <cstdint>

namespace hero {

constexpr std::uint16_t kMagic = 0x4856;
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaxPacketSize = 300;
constexpr std::size_t kMaxPayloadSize = kMaxPacketSize - kHeaderSize;

enum class PacketType : std::uint8_t {
    kConfig = 1,
    kStreamChunk = 2,
    kHeartbeat = 3,
    kReset = 4,
};

enum class CodecId : std::uint8_t {
    kH264AnnexB = 1,
    kHevcAnnexB = 2,
    kMjpeg = 3,
};

enum PacketFlags : std::uint8_t {
    kFlagKeyframeHint = 1 << 0,
    kFlagConfigRepeat = 1 << 1,
    kFlagDiscontinuity = 1 << 2,
};

#pragma pack(push, 1)
struct VideoPacketHeaderV1 {
    std::uint16_t magic;
    std::uint8_t version;
    std::uint8_t packetType;
    std::uint8_t streamId;
    std::uint8_t codec;
    std::uint8_t flags;
    std::uint8_t reserved0;
    std::uint32_t seq;
    std::uint16_t payloadLen;
    std::uint16_t packetCrc16;
};

struct VideoConfigV1 {
    std::uint16_t width;
    std::uint16_t height;
    std::uint16_t fps;
    std::uint16_t targetBytesPerSec;
};

struct VideoHeartbeatV1 {
    std::uint32_t capturedFrames;
    std::uint32_t encodedFrames;
    std::uint32_t sentPackets;
    std::uint32_t droppedPackets;
};
#pragma pack(pop)

inline bool headerLooksValid(const VideoPacketHeaderV1& header) {
    return header.magic == kMagic &&
           header.version == kVersion &&
           header.payloadLen <= kMaxPayloadSize;
}

}  // namespace hero
