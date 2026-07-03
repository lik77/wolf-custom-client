#pragma once

#include <QtCore/QObject>

#include <optional>

#include "hero_protocol.h"

class CustomStreamHandler : public QObject {
    Q_OBJECT

public:
    explicit CustomStreamHandler(QObject* parent = nullptr);

public slots:
    void handleCustomByteBlock(const QByteArray& bytes);

signals:
    void heroConfigSummary(const QString& text);
    void heroChunkReady(const QByteArray& chunk);
    void heroResetRequired();
    void logMessage(const QString& text);

private:
    std::optional<std::uint32_t> expectedSeq_;
    bool waitingForKeyframe_ = true;
    hero::VideoConfigV1 config_ {};
};
