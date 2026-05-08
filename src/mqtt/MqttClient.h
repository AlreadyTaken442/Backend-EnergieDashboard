#pragma once

#include "../database/Database.h"

#include <mqtt/async_client.h>
#include <atomic>
#include <string>

class MqttClient {
public: 
    explicit MqttClient(Database& database);

    void startListening();
    void stopListening();

    bool isConnected() const;

private:
    void saveMessageToDatabase(const std::string& topic, const std::string& payload);

    Database& database;

    mqtt::async_client client;

    std::atomic<bool> running;
};