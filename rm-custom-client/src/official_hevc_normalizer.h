#pragma once

#include <QtCore/QObject>

class OfficialHevcNormalizer : public QObject {
    Q_OBJECT

public:
    explicit OfficialHevcNormalizer(QObject* parent = nullptr);

public slots:
    void handleFrame(const QByteArray& frameBytes);
    void reset();

signals:
    void normalizedHevcFrameReady(const QByteArray& frameBytes);
    void normalizedH264FrameReady(const QByteArray& frameBytes);
    void logMessage(const QString& text);

private:
    enum class CodecMode {
        Unknown,
        Hevc,
        H264,
    };

    struct NalScanResult {
        bool hevcVps = false;
        bool hevcSps = false;
        bool hevcPps = false;
        bool hevcIdr = false;
        bool h264Sps = false;
        bool h264Pps = false;
        bool h264Idr = false;
        QString hevcTypesSummary;
        QString h264TypesSummary;
    };

    static QByteArray convertLengthPrefixedToAnnexB(const QByteArray& bytes, bool& ok, QString& reason);
    static bool looksLikeAnnexB(const QByteArray& bytes);
    static bool looksLikeLengthPrefixedHevc(const QByteArray& bytes);
    static NalScanResult scanNalUnits(const QByteArray& annexB);
    static QString firstBytesHex(const QByteArray& bytes, int maxCount);
    static QString codecModeName(CodecMode codecMode);
    static QByteArray extractNalUnitByType(const QByteArray& annexB, CodecMode codecMode, int targetType);

    bool haveParameterSets_ = false;
    bool haveVps_ = false;
    bool haveSps_ = false;
    bool havePps_ = false;
    bool haveH264Sps_ = false;
    bool haveH264Pps_ = false;
    int droppedFramesBeforeSync_ = 0;
    CodecMode codecMode_ = CodecMode::Unknown;
    bool startedDecoding_ = false;
    QByteArray hevcVpsData_;
    QByteArray hevcSpsData_;
    QByteArray hevcPpsData_;
    QByteArray h264SpsData_;
    QByteArray h264PpsData_;
    bool loggedAnnexBMode_ = false;
    bool loggedEmbeddedStartCodeMode_ = false;
    bool loggedLengthPrefixedMode_ = false;
    bool loggedAmbiguousAnnexBMode_ = false;
};
