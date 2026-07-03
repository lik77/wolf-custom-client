#include "official_hevc_normalizer.h"

#include <QtCore/QtEndian>

#include <algorithm>

#include <QtCore/QStringList>

namespace {

int findStartCode(const QByteArray& bytes, int from) {
    for (int index = from; index + 3 < bytes.size(); ++index) {
        const unsigned char b0 = static_cast<unsigned char>(bytes[index]);
        const unsigned char b1 = static_cast<unsigned char>(bytes[index + 1]);
        const unsigned char b2 = static_cast<unsigned char>(bytes[index + 2]);
        if (b0 == 0 && b1 == 0 && b2 == 1) {
            return index;
        }
        if (index + 4 < bytes.size()) {
            const unsigned char b3 = static_cast<unsigned char>(bytes[index + 3]);
            if (b0 == 0 && b1 == 0 && b2 == 0 && b3 == 1) {
                return index;
            }
        }
    }
    return -1;
}

int startCodeLengthAt(const QByteArray& bytes, int index) {
    if (index + 3 < bytes.size() &&
        static_cast<unsigned char>(bytes[index]) == 0 &&
        static_cast<unsigned char>(bytes[index + 1]) == 0 &&
        static_cast<unsigned char>(bytes[index + 2]) == 1) {
        return 3;
    }
    if (index + 4 < bytes.size() &&
        static_cast<unsigned char>(bytes[index]) == 0 &&
        static_cast<unsigned char>(bytes[index + 1]) == 0 &&
        static_cast<unsigned char>(bytes[index + 2]) == 0 &&
        static_cast<unsigned char>(bytes[index + 3]) == 1) {
        return 4;
    }
    return 0;
}

}  // namespace

OfficialHevcNormalizer::OfficialHevcNormalizer(QObject* parent) : QObject(parent) {}

bool OfficialHevcNormalizer::looksLikeAnnexB(const QByteArray& bytes) {
    return findStartCode(bytes, 0) == 0;
}

bool OfficialHevcNormalizer::looksLikeLengthPrefixedHevc(const QByteArray& bytes) {
    if (bytes.size() < 6) {
        return false;
    }
    const quint32 naluSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()));
    return naluSize > 0 && naluSize + 4u <= static_cast<quint32>(bytes.size());
}

QByteArray OfficialHevcNormalizer::convertLengthPrefixedToAnnexB(const QByteArray& bytes, bool& ok, QString& reason) {
    QByteArray output;
    int offset = 0;
    while (offset + 4 <= bytes.size()) {
        const quint32 naluSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
        offset += 4;
        if (naluSize == 0 || offset + static_cast<int>(naluSize) > bytes.size()) {
            reason = QStringLiteral("长度前缀 HEVC 数据不完整，naluSize=%1，offset=%2，总长=%3")
                         .arg(naluSize)
                         .arg(offset)
                         .arg(bytes.size());
            ok = false;
            return {};
        }
        output.append("\x00\x00\x00\x01", 4);
        output.append(bytes.constData() + offset, static_cast<int>(naluSize));
        offset += static_cast<int>(naluSize);
    }
    ok = (offset == bytes.size() && !output.isEmpty());
    if (!ok) {
        reason = QStringLiteral("长度前缀 HEVC 结尾存在残留字节");
    }
    return output;
}

QString OfficialHevcNormalizer::codecModeName(CodecMode codecMode) {
    switch (codecMode) {
        case CodecMode::Hevc:
            return QStringLiteral("HEVC");
        case CodecMode::H264:
            return QStringLiteral("H.264");
        case CodecMode::Unknown:
        default:
            return QStringLiteral("未知");
    }
}

QByteArray OfficialHevcNormalizer::extractNalUnitByType(const QByteArray& annexB, CodecMode codecMode, int targetType) {
    int index = findStartCode(annexB, 0);
    while (index >= 0) {
        const int startCodeLength = startCodeLengthAt(annexB, index);
        if (startCodeLength == 0) {
            break;
        }
        const int nalStart = index + startCodeLength;
        if (nalStart >= annexB.size()) {
            break;
        }
        const int nextIndex = findStartCode(annexB, nalStart);
        const int nalEnd = nextIndex >= 0 ? nextIndex : annexB.size();
        const unsigned char header0 = static_cast<unsigned char>(annexB[nalStart]);
        const int type = codecMode == CodecMode::Hevc ? ((header0 >> 1) & 0x3F) : (header0 & 0x1F);
        if (type == targetType) {
            return annexB.mid(index, nalEnd - index);
        }
        index = nextIndex;
    }
    return {};
}

QString OfficialHevcNormalizer::firstBytesHex(const QByteArray& bytes, int maxCount) {
    QStringList parts;
    const int count = std::min(maxCount, bytes.size());
    for (int index = 0; index < count; ++index) {
        parts.push_back(QStringLiteral("%1")
                            .arg(static_cast<unsigned char>(bytes[index]), 2, 16, QLatin1Char('0'))
                            .toUpper());
    }
    return parts.join(QStringLiteral(" "));
}

OfficialHevcNormalizer::NalScanResult OfficialHevcNormalizer::scanNalUnits(const QByteArray& annexB) {
    NalScanResult result;
    QStringList hevcTypes;
    QStringList h264Types;

    int index = findStartCode(annexB, 0);
    while (index >= 0) {
        const int startCodeLength = startCodeLengthAt(annexB, index);
        if (startCodeLength == 0) {
            break;
        }
        const int nalStart = index + startCodeLength;
        if (nalStart + 2 > annexB.size()) {
            break;
        }
        const unsigned char header0 = static_cast<unsigned char>(annexB[nalStart]);
        const int nalType = (header0 >> 1) & 0x3F;
        const int h264NalType = header0 & 0x1F;
        if (hevcTypes.size() < 8) {
            hevcTypes.push_back(QString::number(nalType));
        }
        if (h264Types.size() < 8) {
            h264Types.push_back(QString::number(h264NalType));
        }
        if (nalType == 32) {
            result.hevcVps = true;
        } else if (nalType == 33) {
            result.hevcSps = true;
        } else if (nalType == 34) {
            result.hevcPps = true;
        } else if (nalType == 19 || nalType == 20 || nalType == 21) {
            result.hevcIdr = true;
        }

        if (h264NalType == 7) {
            result.h264Sps = true;
        } else if (h264NalType == 8) {
            result.h264Pps = true;
        } else if (h264NalType == 5) {
            result.h264Idr = true;
        }

        index = findStartCode(annexB, nalStart);
    }

    result.hevcTypesSummary = hevcTypes.join(QStringLiteral(","));
    result.h264TypesSummary = h264Types.join(QStringLiteral(","));
    return result;
}

void OfficialHevcNormalizer::handleFrame(const QByteArray& frameBytes) {
    if (frameBytes.isEmpty()) {
        return;
    }

    QByteArray annexB;
    const bool annexBLike = looksLikeAnnexB(frameBytes);
    const bool lengthPrefixedLike = looksLikeLengthPrefixedHevc(frameBytes);
    const int embeddedStartCode = findStartCode(frameBytes, 1);

    if (annexBLike) {
        annexB = frameBytes;
        if (lengthPrefixedLike) {
            if (!loggedAmbiguousAnnexBMode_) {
                loggedAmbiguousAnnexBMode_ = true;
                emit logMessage(QStringLiteral("[信息] 官方图传 HEVC 同时看起来像 Annex-B 和长度前缀格式，因检测到起始码，优先按 Annex-B 处理"));
            }
        } else {
            if (!loggedAnnexBMode_) {
                loggedAnnexBMode_ = true;
                emit logMessage(QStringLiteral("[信息] 官方图传 HEVC 当前帧本身就是 Annex-B 格式"));
            }
        }
    } else if (embeddedStartCode > 0 && embeddedStartCode <= 64) {
        annexB = frameBytes.mid(embeddedStartCode);
        if (!loggedEmbeddedStartCodeMode_) {
            loggedEmbeddedStartCodeMode_ = true;
            emit logMessage(QStringLiteral("[信息] 官方图传 HEVC 在帧内偏移 %1 处检测到起始码，已裁掉前导 %2 字节后按 Annex-B 处理")
                                .arg(embeddedStartCode)
                                .arg(embeddedStartCode));
        }
    } else if (lengthPrefixedLike) {
        bool ok = false;
        QString reason;
        annexB = convertLengthPrefixedToAnnexB(frameBytes, ok, reason);
        if (!ok) {
            emit logMessage(QStringLiteral("[错误] 官方图传 HEVC 长度前缀转 Annex-B 失败: %1；帧前16字节=%2")
                                .arg(reason)
                                .arg(firstBytesHex(frameBytes, 16)));
            return;
        }
        if (!loggedLengthPrefixedMode_) {
            loggedLengthPrefixedMode_ = true;
            emit logMessage(QStringLiteral("[信息] 官方图传 HEVC 已从长度前缀格式转换为 Annex-B"));
        }
    } else {
        emit logMessage(QStringLiteral("[错误] 官方图传 HEVC 既不像 Annex-B，也不像长度前缀格式，当前帧长度=%1，帧前16字节=%2")
                            .arg(frameBytes.size())
                            .arg(firstBytesHex(frameBytes, 16)));
        return;
    }

    const NalScanResult scan = scanNalUnits(annexB);

    if (codecMode_ == CodecMode::Unknown) {
        if (scan.hevcVps || scan.hevcSps || scan.hevcPps) {
            codecMode_ = CodecMode::Hevc;
            emit logMessage(QStringLiteral("[信息] 官方图传编码格式判定为 HEVC，HEVC NAL 类型序列=%1")
                                .arg(scan.hevcTypesSummary));
        } else if (scan.h264Sps || scan.h264Pps) {
            codecMode_ = CodecMode::H264;
            emit logMessage(QStringLiteral("[信息] 官方图传编码格式判定为 H.264，H.264 NAL 类型序列=%1")
                                .arg(scan.h264TypesSummary));
        }
    }

    if (codecMode_ == CodecMode::Unknown) {
        static int unknownCodecFrames = 0;
        ++unknownCodecFrames;
        if (unknownCodecFrames <= 5 || unknownCodecFrames % 50 == 0) {
            emit logMessage(QStringLiteral("[警告] 仍无法判定官方图传编码格式，HEVC NAL 类型序列=%1，H.264 NAL 类型序列=%2")
                                .arg(scan.hevcTypesSummary)
                                .arg(scan.h264TypesSummary));
        }
    }

    if (codecMode_ == CodecMode::Hevc && scan.hevcVps) {
        haveVps_ = true;
        hevcVpsData_ = extractNalUnitByType(annexB, CodecMode::Hevc, 32);
    }
    if (codecMode_ == CodecMode::Hevc && scan.hevcSps) {
        haveSps_ = true;
        hevcSpsData_ = extractNalUnitByType(annexB, CodecMode::Hevc, 33);
    }
    if (codecMode_ == CodecMode::Hevc && scan.hevcPps) {
        havePps_ = true;
        hevcPpsData_ = extractNalUnitByType(annexB, CodecMode::Hevc, 34);
    }

    if (codecMode_ == CodecMode::H264 && scan.h264Sps) {
        haveH264Sps_ = true;
        h264SpsData_ = extractNalUnitByType(annexB, CodecMode::H264, 7);
    }
    if (codecMode_ == CodecMode::H264 && scan.h264Pps) {
        haveH264Pps_ = true;
        h264PpsData_ = extractNalUnitByType(annexB, CodecMode::H264, 8);
    }

    if (codecMode_ == CodecMode::Hevc) {
        haveParameterSets_ = haveVps_ && haveSps_ && havePps_;
        if (scan.hevcVps || scan.hevcSps || scan.hevcPps || scan.hevcIdr) {
            emit logMessage(QStringLiteral("[信息] 官方图传 HEVC 参数集累计状态：VPS=%1 SPS=%2 PPS=%3，本帧IDR=%4")
                                .arg(haveVps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                .arg(haveSps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                .arg(havePps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                .arg(scan.hevcIdr ? QStringLiteral("有") : QStringLiteral("无")));
        }
    } else if (codecMode_ == CodecMode::H264) {
        haveParameterSets_ = haveH264Sps_ && haveH264Pps_;
        if (scan.h264Sps || scan.h264Pps || scan.h264Idr) {
            emit logMessage(QStringLiteral("[信息] 官方图传 H.264 参数集累计状态：SPS=%1 PPS=%2，本帧IDR=%3")
                                .arg(haveH264Sps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                .arg(haveH264Pps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                .arg(scan.h264Idr ? QStringLiteral("有") : QStringLiteral("无")));
        }
    } else {
        haveParameterSets_ = false;
    }

    if (!haveParameterSets_) {
        ++droppedFramesBeforeSync_;
        if (droppedFramesBeforeSync_ <= 5 || droppedFramesBeforeSync_ % 50 == 0) {
            if (codecMode_ == CodecMode::Hevc) {
                emit logMessage(QStringLiteral("[警告] 官方图传 HEVC 尚未拿到完整 VPS/SPS/PPS，已丢弃 %1 帧，累计状态 VPS=%2 SPS=%3 PPS=%4")
                                    .arg(droppedFramesBeforeSync_)
                                    .arg(haveVps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                    .arg(haveSps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                    .arg(havePps_ ? QStringLiteral("有") : QStringLiteral("无")));
            } else if (codecMode_ == CodecMode::H264) {
                emit logMessage(QStringLiteral("[警告] 官方图传 H.264 尚未拿到完整 SPS/PPS，已丢弃 %1 帧，累计状态 SPS=%2 PPS=%3")
                                    .arg(droppedFramesBeforeSync_)
                                    .arg(haveH264Sps_ ? QStringLiteral("有") : QStringLiteral("无"))
                                    .arg(haveH264Pps_ ? QStringLiteral("有") : QStringLiteral("无")));
            }
        }
        return;
    }

    const bool hasIdr = (codecMode_ == CodecMode::Hevc && scan.hevcIdr) ||
                        (codecMode_ == CodecMode::H264 && scan.h264Idr);

    if (!startedDecoding_) {
        if (!hasIdr) {
            static int waitingIdrFrames = 0;
            ++waitingIdrFrames;
            if (waitingIdrFrames <= 5 || waitingIdrFrames % 50 == 0) {
                emit logMessage(QStringLiteral("[警告] 官方图传 %1 参数集已齐，但还没等到 IDR 关键帧，继续等待")
                                    .arg(codecModeName(codecMode_)));
            }
            return;
        }

        emit logMessage(QStringLiteral("[信息] 官方图传 %1 已检测到 IDR 关键帧，开始向解码器注入参数集并起播")
                            .arg(codecModeName(codecMode_)));
        if (codecMode_ == CodecMode::Hevc) {
            QByteArray bootstrap;
            bootstrap.append(hevcVpsData_);
            bootstrap.append(hevcSpsData_);
            bootstrap.append(hevcPpsData_);
            emit normalizedHevcFrameReady(bootstrap);
        } else if (codecMode_ == CodecMode::H264) {
            QByteArray bootstrap;
            bootstrap.append(h264SpsData_);
            bootstrap.append(h264PpsData_);
            emit normalizedH264FrameReady(bootstrap);
        }
        startedDecoding_ = true;
    }

    if (codecMode_ == CodecMode::Hevc) {
        emit normalizedHevcFrameReady(annexB);
    } else if (codecMode_ == CodecMode::H264) {
        emit normalizedH264FrameReady(annexB);
    }
}

void OfficialHevcNormalizer::reset() {
    haveParameterSets_ = false;
    haveVps_ = false;
    haveSps_ = false;
    havePps_ = false;
    haveH264Sps_ = false;
    haveH264Pps_ = false;
    droppedFramesBeforeSync_ = 0;
    codecMode_ = CodecMode::Unknown;
    startedDecoding_ = false;
    hevcVpsData_.clear();
    hevcSpsData_.clear();
    hevcPpsData_.clear();
    h264SpsData_.clear();
    h264PpsData_.clear();
    loggedAnnexBMode_ = false;
    loggedEmbeddedStartCodeMode_ = false;
    loggedLengthPrefixedMode_ = false;
    loggedAmbiguousAnnexBMode_ = false;
    emit logMessage(QStringLiteral("[警告] 官方图传规范化器已重置，重新等待参数集和 IDR 关键帧"));
}
