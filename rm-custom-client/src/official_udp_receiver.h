#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtNetwork/QUdpSocket>

class OfficialUdpReceiver : public QObject {
    Q_OBJECT

public:
    explicit OfficialUdpReceiver(QObject* parent = nullptr);
    bool start(quint16 port);

signals:
    void hevcFrameReady(const QByteArray& frameBytes);
    void logMessage(const QString& text);

private slots:
    void onReadyRead();

private:
    enum class HeaderByteOrder {
        Unknown,
        LittleEndian,
        BigEndian,
    };

    struct ParsedHeader {
        quint16 frameId = 0;
        quint16 sliceIndex = 0;
        quint32 totalBytes = 0;
        HeaderByteOrder byteOrder = HeaderByteOrder::Unknown;
    };

    struct PendingFrame {
        quint32 totalBytes = 0;
        quint32 receivedBytes = 0;
        QMap<quint16, QByteArray> slices;
        qint64 firstSeenMs = 0;
    };

    static QString byteOrderName(HeaderByteOrder byteOrder);
    static QString firstBytesHex(const QByteArray& bytes, int maxCount);
    bool isPlausibleHeader(const ParsedHeader& header, int payloadSize) const;
    ParsedHeader parseHeader(const QByteArray& datagram, bool& ok, QString& reason);
    void dumpFrameIfNeeded(quint16 frameId, const QByteArray& frameBytes);
    void cleanupExpiredFrames();

    QUdpSocket socket_;
    QHash<quint16, PendingFrame> pendingFrames_;
    HeaderByteOrder lockedByteOrder_ = HeaderByteOrder::Unknown;
    int dumpedFrameCount_ = 0;
};
