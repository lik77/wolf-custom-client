#pragma once

#include <QtCore/QObject>

#include <memory>

#include <mqtt/async_client.h>

#include "control_state.h"

class MqttClient : public QObject, public virtual mqtt::callback {
    Q_OBJECT

public:
    explicit MqttClient(QObject* parent = nullptr);
    ~MqttClient() override;

    void connectToBroker(const QString& host, int port, const QString& clientId);

public slots:
    void publishControlFrame(const ControlState& state);
    void publishCustomControl(const QByteArray& payload);

signals:
    void customByteBlockReceived(const QByteArray& payload);
    void gameStatusUpdated(const QString& text);
    void dynamicStatusUpdated(const QString& text);
    void moduleStatusUpdated(const QString& text);
    void positionStatusUpdated(const QString& text);
    void connectionStateChanged(const QString& text);
    void logMessage(const QString& text);

private:
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr) override;

    void subscribeAll();
    void publishBinary(const std::string& topic, const std::string& payload);

    QString host_;
    int port_ = 3333;
    QString clientId_;
    std::unique_ptr<mqtt::async_client> client_;
    mqtt::connect_options connectOptions_;
    bool connected_ = false;
};
