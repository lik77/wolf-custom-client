#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <mqtt/async_client.h>

class MqttPublisher : public virtual mqtt::callback {
public:
    MqttPublisher(std::string host, int port, std::string clientId, std::string topic);
    ~MqttPublisher() override;

    void connect();
    void publishCustomControl(const std::vector<std::uint8_t>& payload);
    bool isConnected() const;

private:
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;

    std::string uri_;
    std::string topic_;
    std::unique_ptr<mqtt::async_client> client_;
    mqtt::connect_options connectOptions_;
    bool connected_ = false;
};
