#include "MqttClient.h"

#include <iostream>
#include <chrono>
#include <stdexcept>

namespace {
    const std::string BROKER_URL = "tcp://test.mosquitto.org:1883";
    const std::string CLIENT_ID = "cpp-energy-dashboard-client";
    const std::string TOPIC = "cbs/#";
}

MqttClient::MqttClient(Database& database)
    : database(database),
      client(BROKER_URL, CLIENT_ID),
      running(false) {
}

void MqttClient::startListening() {
    try {
        mqtt::connect_options options;
        options.set_automatic_reconnect(true);
        options.set_clean_session(true);
        options.set_connect_timeout(std::chrono::seconds(10));

        client.connect(options)->wait();

        std::cout << "MQTT verbunden: " << BROKER_URL << std::endl;

        client.start_consuming();
        client.subscribe(TOPIC, 1)->wait();

        running = true;

        while (running) {
            auto message = client.consume_message();

            if (!message) {
                continue;
            }

            saveMessageToDatabase(
                message->get_topic(),
                message->to_string()
            );
        }

    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT Fehler: " << e.what() << std::endl;
    }
}

void MqttClient::stopListening() {
    running = false;

    try {
        if (client.is_connected()) {
            client.unsubscribe(TOPIC)->wait();
            client.stop_consuming();
            client.disconnect()->wait();
        }
    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT Stop-Fehler: " << e.what() << std::endl;
    }
}

bool MqttClient::isConnected() const {
    return client.is_connected();
}

void MqttClient::saveMessageToDatabase(const std::string& topic, const std::string& payload) {
    try {
        const double value = std::stod(payload);

        MYSQL* connection = database.getConnection();

        std::string escapedTopic;
        escapedTopic.resize(topic.size() * 2 + 1);

        unsigned long topicLength = mysql_real_escape_string(
            connection,
            escapedTopic.data(),
            topic.c_str(),
            static_cast<unsigned long>(topic.size())
        );

        escapedTopic.resize(topicLength);

        const std::string selectQuery =
            "SELECT geraet_id FROM geraet "
            "WHERE mqtt_topic = '" + escapedTopic + "' "
            "LIMIT 1";

        if (mysql_query(connection, selectQuery.c_str()) != 0) {
            std::cerr << "Gerät-Suche fehlgeschlagen: "
                      << mysql_error(connection)
                      << std::endl;
            return;
        }

        MYSQL_RES* result = mysql_store_result(connection);

        if (result == nullptr) {
            std::cerr << "Result konnte nicht gelesen werden: "
                      << mysql_error(connection)
                      << std::endl;
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == nullptr) {
            std::cout << "Kein Gerät für MQTT-Topic gefunden: "
                      << topic
                      << std::endl;

            mysql_free_result(result);
            return;
        }

        const int geraetId = std::stoi(row[0]);
        mysql_free_result(result);

        const std::string insertQuery =
            "INSERT INTO sensordaten "
            "(geraet_id, zeitstempel, stromverbrauch, status) VALUES (" +
            std::to_string(geraetId) + ", NOW(), " +
            std::to_string(value) + ", 'OK')";

        if (mysql_query(connection, insertQuery.c_str()) != 0) {
            std::cerr << "Sensorwert konnte nicht gespeichert werden: "
                      << mysql_error(connection)
                      << std::endl;
            return;
        }

        std::cout << "MQTT gespeichert: "
                  << topic
                  << " = "
                  << value
                  << std::endl;

    } catch (const std::invalid_argument&) {
        std::cerr << "MQTT Payload ist keine Zahl: " << payload << std::endl;
    } catch (const std::out_of_range&) {
        std::cerr << "MQTT Payload außerhalb des Zahlenbereichs: " << payload << std::endl;
    }
}