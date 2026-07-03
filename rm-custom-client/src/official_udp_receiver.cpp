#include "official_udp_receiver.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <QtCore/QFile>
#include <QtCore/QtEndian>
#include <QtNetwork/QHostAddress>

OfficialUdpReceiver::OfficialUdpReceiver(QObject* parent) : QObject(parent) {
    connect(&socket_, &QUdpSocket::readyRead, this, &OfficialUdpReceiver::onReadyRead);
}

QString OfficialUdpReceiver::byteOrderName(HeaderByteOrder byteOrder) {
    switch (byteOrder) {
        case HeaderByteOrder::LittleEndian:
            return QStringLiteral("小端");
        case HeaderByteOrder::BigEndian:
            return QStringLiteral("大端");
        case HeaderByteOrder::Unknown:
        default:
            return QStringLiteral("未知");
    }
}

QString OfficialUdpReceiver::firstBytesHex(const QByteArray& bytes, int maxCount) {
    QStringList parts;
    const int count = std::min(maxCount, bytes.size());
    for (int index = 0; index < count; ++index) {
        parts.push_back(QStringLiteral("%1")
                            .arg(static_cast<unsigned char>(bytes[index]), 2, 16, QLatin1Char('0'))
                            .toUpper());
    }
    return parts.join(QStringLiteral(" "));
}

bool OfficialUdpReceiver::isPlausibleHeader(const ParsedHeader& header, int payloadSize) const {
    if (payloadSize <= 0) {
        return false;
    }
    if (header.totalBytes < static_cast<quint32>(payloadSize)) {
        return false;
    }
    if (header.totalBytes > 1024u * 1024u) {
        return false;
    }
    if (header.sliceIndex > 4096u) {
        return false;
    }
    return true;
}

OfficialUdpReceiver::ParsedHeader OfficialUdpReceiver::parseHeader(const QByteArray& datagram, bool& ok, QString& reason) {
    const auto* bytes = reinterpret_cast<const uchar*>(datagram.constData());
    const int payloadSize = datagram.size() - 8;

    ParsedHeader little;
    little.frameId = qFromLittleEndian<quint16>(bytes);
    little.sliceIndex = qFromLittleEndian<quint16>(bytes + 2);
    little.totalBytes = qFromLittleEndian<quint32>(bytes + 4);
    little.byteOrder = HeaderByteOrder::LittleEndian;

    ParsedHeader big;
    big.frameId = qFromBigEndian<quint16>(bytes);
    big.sliceIndex = qFromBigEndian<quint16>(bytes + 2);
    big.totalBytes = qFromBigEndian<quint32>(bytes + 4);
    big.byteOrder = HeaderByteOrder::BigEndian;

    const bool littlePlausible = isPlausibleHeader(little, payloadSize);
    const bool bigPlausible = isPlausibleHeader(big, payloadSize);

    if (lockedByteOrder_ == HeaderByteOrder::LittleEndian) {
        ok = littlePlausible;
        if (!ok) {
            reason = QStringLiteral("已锁定为小端，但当前包头字段不合理，frameId=%1 slice=%2 total=%3 payload=%4")
                         .arg(little.frameId)
                         .arg(little.sliceIndex)
                         .arg(little.totalBytes)
                         .arg(payloadSize);
        }
        return little;
    }

    if (lockedByteOrder_ == HeaderByteOrder::BigEndian) {
        ok = bigPlausible;
        if (!ok) {
            reason = QStringLiteral("已锁定为大端，但当前包头字段不合理，frameId=%1 slice=%2 total=%3 payload=%4")
                         .arg(big.frameId)
                         .arg(big.sliceIndex)
                         .arg(big.totalBytes)
                         .arg(payloadSize);
        }
        return big;
    }

    if (littlePlausible && !bigPlausible) {
        lockedByteOrder_ = HeaderByteOrder::LittleEndian;
        emit logMessage(QStringLiteral("[信息] 已自动判定官方图传 3334 包头为小端格式"));
        ok = true;
        return little;
    }

    if (bigPlausible && !littlePlausible) {
        lockedByteOrder_ = HeaderByteOrder::BigEndian;
        emit logMessage(QStringLiteral("[信息] 已自动判定官方图传 3334 包头为大端格式"));
        ok = true;
        return big;
    }

    if (littlePlausible && bigPlausible) {
        if (little.sliceIndex <= big.sliceIndex) {
            lockedByteOrder_ = HeaderByteOrder::LittleEndian;
            emit logMessage(QStringLiteral("[警告] 官方图传 3334 包头大小端都看似合理，暂时优先采用小端"));
            ok = true;
            return little;
        }
        lockedByteOrder_ = HeaderByteOrder::BigEndian;
        emit logMessage(QStringLiteral("[警告] 官方图传 3334 包头大小端都看似合理，暂时优先采用大端"));
        ok = true;
        return big;
    }

    reason = QStringLiteral("大小端解析都不合理，小端(frame=%1 slice=%2 total=%3) 大端(frame=%4 slice=%5 total=%6) payload=%7")
                 .arg(little.frameId)
                 .arg(little.sliceIndex)
                 .arg(little.totalBytes)
                 .arg(big.frameId)
                 .arg(big.sliceIndex)
                 .arg(big.totalBytes)
                 .arg(payloadSize);
    ok = false;
    return {};
}

bool OfficialUdpReceiver::start(quint16 port) {
    const bool ok = socket_.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!ok) {
        emit logMessage(QStringLiteral("[错误] 绑定 UDP %1 失败: %2").arg(port).arg(socket_.errorString()));
        return false;
    }
    emit logMessage(QStringLiteral("[信息] 已开始监听 UDP %1，等待官方图传 HEVC 码流").arg(port));
    return true;
}

void OfficialUdpReceiver::dumpFrameIfNeeded(quint16 frameId, const QByteArray& frameBytes) {
    if (dumpedFrameCount_ >= 10) {
        return;
    }

    QDir dumpDir(QDir::homePath() + QStringLiteral("/尝试1/rm-custom-client/dump_frames"));
    if (!dumpDir.exists()) {
        dumpDir.mkpath(QStringLiteral("."));
    }

    const QString filePath = dumpDir.filePath(QStringLiteral("official_frame_%1_%2.bin")
                                                  .arg(dumpedFrameCount_, 2, 10, QLatin1Char('0'))
                                                  .arg(frameId));
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit logMessage(QStringLiteral("[错误] 保存官方图传重组帧失败: %1").arg(filePath));
        return;
    }
    file.write(frameBytes);
    file.close();
    ++dumpedFrameCount_;

    emit logMessage(QStringLiteral("[信息] 已保存官方图传重组帧到 %1，前64字节=%2")
                        .arg(filePath)
                        .arg(firstBytesHex(frameBytes, 64)));
}

void OfficialUdpReceiver::onReadyRead() {
    cleanupExpiredFrames();
    while (socket_.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(socket_.pendingDatagramSize()));
        socket_.readDatagram(datagram.data(), datagram.size());

        if (datagram.size() <= 8) {
            continue;
        }

        bool ok = false;
        QString reason;
        const ParsedHeader header = parseHeader(datagram, ok, reason);
        if (!ok) {
            static int invalidHeaderCount = 0;
            ++invalidHeaderCount;
            if (invalidHeaderCount <= 5 || invalidHeaderCount % 100 == 0) {
                emit logMessage(QStringLiteral("[错误] 官方图传包头解析失败: %1").arg(reason));
            }
            continue;
        }

        const quint16 frameId = header.frameId;
        const quint16 sliceIndex = header.sliceIndex;
        const quint32 totalBytes = header.totalBytes;
        const QByteArray payload = datagram.mid(8);

        static int datagramCount = 0;
        ++datagramCount;
        if (datagramCount == 1 || datagramCount % 1000 == 0) {
            emit logMessage(QStringLiteral("[信息] 已收到官方图传 UDP 包 %1 个，frameId=%2，slice=%3，payload=%4 字节，总帧长=%5，包头字节序=%6")
                                .arg(datagramCount)
                                .arg(frameId)
                                .arg(sliceIndex)
                                .arg(payload.size())
                                .arg(totalBytes)
                                .arg(byteOrderName(header.byteOrder)));
        }

        auto& frame = pendingFrames_[frameId];
        if (frame.firstSeenMs == 0) {
            frame.firstSeenMs = QDateTime::currentMSecsSinceEpoch();
            frame.totalBytes = totalBytes;
        } else if (frame.totalBytes != totalBytes) {
            emit logMessage(QStringLiteral("[警告] 同一 frameId=%1 的总长度前后不一致，旧值=%2，新值=%3，已重置该帧缓存")
                                .arg(frameId)
                                .arg(frame.totalBytes)
                                .arg(totalBytes));
            frame = PendingFrame{};
            frame.firstSeenMs = QDateTime::currentMSecsSinceEpoch();
            frame.totalBytes = totalBytes;
        }
        if (!frame.slices.contains(sliceIndex)) {
            frame.slices.insert(sliceIndex, payload);
            frame.receivedBytes += static_cast<quint32>(payload.size());
        }

        if (frame.receivedBytes > frame.totalBytes) {
            emit logMessage(QStringLiteral("[警告] frameId=%1 的累计重组字节数超过总长度，received=%2 total=%3，已丢弃该帧")
                                .arg(frameId)
                                .arg(frame.receivedBytes)
                                .arg(frame.totalBytes));
            pendingFrames_.remove(frameId);
            continue;
        }

        if (frame.receivedBytes < frame.totalBytes) {
            continue;
        }

        QByteArray assembled;
        assembled.reserve(static_cast<int>(frame.totalBytes));
        for (auto it = frame.slices.cbegin(); it != frame.slices.cend(); ++it) {
            assembled.append(it.value());
            if (assembled.size() >= static_cast<int>(frame.totalBytes)) {
                assembled.truncate(static_cast<int>(frame.totalBytes));
                break;
            }
        }

        if (assembled.size() == static_cast<int>(frame.totalBytes)) {
            static int assembledFrameCount = 0;
            ++assembledFrameCount;
            if (assembledFrameCount == 1 || assembledFrameCount % 120 == 0) {
                emit logMessage(QStringLiteral("[信息] 已完成官方图传重组 %1 帧，当前 frameId=%2，总长度=%3 字节")
                                    .arg(assembledFrameCount)
                                    .arg(frameId)
                                    .arg(frame.totalBytes));
            }
            dumpFrameIfNeeded(frameId, assembled);
            assembled.append(QByteArray(AV_INPUT_BUFFER_PADDING_SIZE, '\0'));
            emit hevcFrameReady(assembled);
        }
        pendingFrames_.remove(frameId);
    }
}

void OfficialUdpReceiver::cleanupExpiredFrames() {
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QList<quint16> toRemove;
    for (auto it = pendingFrames_.cbegin(); it != pendingFrames_.cend(); ++it) {
        if (nowMs - it.value().firstSeenMs > 300) {
            toRemove.push_back(it.key());
        }
    }
    for (quint16 frameId : toRemove) {
        const auto frame = pendingFrames_.value(frameId);
        emit logMessage(QStringLiteral("[警告] 官方图传帧重组超时，已丢弃 frameId=%1，已收 %2 / %3 字节，分片数 %4，当前包头字节序=%5")
                            .arg(frameId)
                            .arg(frame.receivedBytes)
                            .arg(frame.totalBytes)
                            .arg(frame.slices.size())
                            .arg(byteOrderName(lockedByteOrder_)));
        pendingFrames_.remove(frameId);
    }
}
