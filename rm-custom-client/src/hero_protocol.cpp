#include "hero_protocol.h"

#include <QtCore/QString>

#include <cstring>

namespace hero {

std::uint16_t crc16CcittFalse(const std::uint8_t* data, std::size_t size) {
    std::uint16_t crc = 0xFFFF;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= static_cast<std::uint16_t>(data[index]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000U) != 0U) {
                crc = static_cast<std::uint16_t>((crc << 1U) ^ 0x1021U);
            } else {
                crc <<= 1U;
            }
        }
    }
    return crc;
}

bool parsePacket(const QByteArray& bytes, VideoPacketHeaderV1& header, QByteArray& payload, QString& error) {
    if (bytes.size() < static_cast<int>(kHeaderSize)) {
        error = QStringLiteral("hero packet too short");
        return false;
    }

    std::memcpy(&header, bytes.constData(), kHeaderSize);
    if (header.magic != kMagic || header.version != kVersion) {
        error = QStringLiteral("invalid hero packet magic/version");
        return false;
    }
    if (header.payloadLen > kMaxPayloadSize) {
        error = QStringLiteral("hero payload too large");
        return false;
    }
    if (bytes.size() < static_cast<int>(kHeaderSize + header.payloadLen)) {
        error = QStringLiteral("hero packet size mismatch");
        return false;
    }

    if (bytes.size() > static_cast<int>(kHeaderSize + header.payloadLen)) {
        payload = bytes.mid(static_cast<int>(kHeaderSize), header.payloadLen);
    } else {
        payload = bytes.mid(static_cast<int>(kHeaderSize), header.payloadLen);
    }
    QByteArray crcInput;
    crcInput.reserve(static_cast<int>(offsetof(VideoPacketHeaderV1, packetCrc16) + payload.size()));
    crcInput.append(bytes.constData(), static_cast<int>(offsetof(VideoPacketHeaderV1, packetCrc16)));
    crcInput.append(payload);

    const auto actualCrc = crc16CcittFalse(
        reinterpret_cast<const std::uint8_t*>(crcInput.constData()),
        static_cast<std::size_t>(crcInput.size()));
    if (actualCrc != header.packetCrc16) {
        error = QStringLiteral("hero packet crc mismatch");
        return false;
    }

    return true;
}

}  // namespace hero
