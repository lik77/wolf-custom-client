#pragma once

#include <QtCore/QObject>
#include <QtGui/QImage>

extern "C" {
#include <libavcodec/avcodec.h>
}

class VideoDecoder : public QObject {
    Q_OBJECT

public:
    explicit VideoDecoder(AVCodecID codecId, QObject* parent = nullptr);
    ~VideoDecoder() override;

public slots:
    void feedBytes(const QByteArray& bytes);
    void reset();

signals:
    void frameDecoded(const QImage& image);
    void logMessage(const QString& text);

private:
    void initDecoder();
    void releaseDecoder();
    void drainDecoder();

    AVCodecID codecId_;
    AVCodecParserContext* parser_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    struct SwsContext* swsContext_ = nullptr;
};
