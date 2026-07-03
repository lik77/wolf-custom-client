#include "custom_stream_handler.h"

#include <QtCore/QString>

#include <cstring>

CustomStreamHandler::CustomStreamHandler(QObject* parent) : QObject(parent) {}

void CustomStreamHandler::handleCustomByteBlock(const QByteArray& bytes) {
    hero::VideoPacketHeaderV1 header {};
    QByteArray payload;
    QString error;
    if (!hero::parsePacket(bytes, header, payload, error)) {
        emit logMessage(QStringLiteral("[错误] 自研视频包解析失败: %1").arg(error));
        return;
    }

    if (bytes.size() > static_cast<int>(hero::kHeaderSize + header.payloadLen)) {
        static int paddedPacketCount = 0;
        ++paddedPacketCount;
        if (paddedPacketCount <= 5 || paddedPacketCount % 100 == 0) {
            emit logMessage(QStringLiteral("[信息] 收到带补零外层的自研视频包：总长度=%1，有效长度=%2，已忽略尾部填充")
                                .arg(bytes.size())
                                .arg(static_cast<int>(hero::kHeaderSize + header.payloadLen)));
        }
    }

    static int packetCount = 0;
    ++packetCount;
    if (packetCount == 1 || packetCount % 200 == 0) {
        emit logMessage(QStringLiteral("[信息] 已解析自研视频包 %1 个，当前 seq=%2，payload=%3 字节")
                            .arg(packetCount)
                            .arg(header.seq)
                            .arg(payload.size()));
    }

    if (expectedSeq_.has_value() && header.seq != *expectedSeq_) {
        waitingForKeyframe_ = true;
        emit heroResetRequired();
        emit logMessage(QStringLiteral("[错误] 自研视频包序号跳变，期望 %1，实际 %2，已等待新的关键帧恢复")
                            .arg(*expectedSeq_)
                            .arg(header.seq));
    }
    expectedSeq_ = header.seq + 1U;

    switch (static_cast<hero::PacketType>(header.packetType)) {
        case hero::PacketType::kConfig: {
            if (payload.size() < static_cast<int>(sizeof(hero::VideoConfigV1))) {
                emit logMessage(QStringLiteral("[错误] 自研视频配置包长度不足"));
                return;
            }
            std::memcpy(&config_, payload.constData(), sizeof(config_));
            emit heroConfigSummary(
                QStringLiteral("英雄车视频 %1x%2 @ %3fps，目标码率 %4 B/s")
                    .arg(config_.width)
                    .arg(config_.height)
                    .arg(config_.fps)
                    .arg(config_.targetBytesPerSec));
            emit logMessage(QStringLiteral("[信息] 已收到自研视频配置包，开始等待关键帧"));
            return;
        }
        case hero::PacketType::kReset:
            waitingForKeyframe_ = true;
            emit heroResetRequired();
            emit logMessage(QStringLiteral("[警告] 已收到自研视频重置包，清空解码缓存并等待新关键帧"));
            return;
        case hero::PacketType::kHeartbeat:
            emit logMessage(QStringLiteral("[信息] 已收到自研视频心跳包"));
            return;
        case hero::PacketType::kStreamChunk:
            break;
    }

    const bool keyframe = (header.flags & hero::kFlagKeyframeHint) != 0;
    if (waitingForKeyframe_) {
        if (!keyframe) {
            static int skippedNonKeyframes = 0;
            ++skippedNonKeyframes;
            if (skippedNonKeyframes <= 3 || skippedNonKeyframes % 100 == 0) {
                emit logMessage(QStringLiteral("[警告] 当前正在等待关键帧，已跳过 %1 个普通分片")
                                    .arg(skippedNonKeyframes));
            }
            return;
        }
        waitingForKeyframe_ = false;
        emit heroResetRequired();
        emit logMessage(QStringLiteral("[信息] 已收到新的关键帧，自研视频开始恢复解码"));
    }

    emit heroChunkReady(payload);
}
