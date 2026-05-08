#include "HttpController.h"

#include "../database/Database.h"

#include <ctime>
#include <functional>
#include <iomanip>
#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <string>

namespace {

std::string escapeJson(const std::string& value) {
    std::ostringstream escaped;
    for (const char character : value) {
        switch (character) {
            case '\\':
                escaped << "\\\\";
                break;
            case '"':
                escaped << "\\\"";
                break;
            case '\n':
                escaped << "\\n";
                break;
            case '\r':
                escaped << "\\r";
                break;
            case '\t':
                escaped << "\\t";
                break;
            default:
                escaped << character;
                break;
        }
    }
    return escaped.str();
}

std::string jsonValue(const std::string& body, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = body.find(marker);
    if (keyPosition == std::string::npos) {
        return "";
    }

    const std::size_t colonPosition = body.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos) {
        return "";
    }

    const std::size_t firstQuote = body.find('"', colonPosition + 1);
    if (firstQuote == std::string::npos) {
        return "";
    }

    const std::size_t secondQuote = body.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) {
        return "";
    }

    return body.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

int jsonIntValue(const std::string& body, const std::string& key, int fallbackValue) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = body.find(marker);
    if (keyPosition == std::string::npos) {
        return fallbackValue;
    }

    const std::size_t colonPosition = body.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos) {
        return fallbackValue;
    }

    std::size_t valueStart = body.find_first_of("0123456789-", colonPosition + 1);
    if (valueStart == std::string::npos) {
        return fallbackValue;
    }

    std::size_t valueEnd = body.find_first_not_of("0123456789-", valueStart);
    return std::stoi(body.substr(valueStart, valueEnd - valueStart));
}

bool jsonBoolValue(const std::string& body, const std::string& key, bool fallbackValue) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = body.find(marker);
    if (keyPosition == std::string::npos) {
        return fallbackValue;
    }

    const std::size_t colonPosition = body.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos) {
        return fallbackValue;
    }

    const std::size_t valueStart = body.find_first_not_of(" \t\n\r", colonPosition + 1);
    if (valueStart == std::string::npos) {
        return fallbackValue;
    }

    if (body.compare(valueStart, 4, "true") == 0) {
        return true;
    }

    if (body.compare(valueStart, 5, "false") == 0) {
        return false;
    }

    return fallbackValue;
}

std::string generateSalt() {
    static const char* alphabet =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<int> distribution(0, 61);

    std::string salt;
    salt.reserve(16);
    for (int index = 0; index < 16; ++index) {
        salt.push_back(alphabet[distribution(generator)]);
    }

    return salt;
}

std::string sha256Hash(const std::string& value) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), digest);

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char byte : digest) {
        out << std::setw(2) << static_cast<int>(byte);
    }
    return out.str();
}

std::string buildPasswordRecord(const std::string& plainPassword) {
    const std::string salt = generateSalt();
    return salt + "$" + sha256Hash(salt + plainPassword);
}

bool verifyPassword(const std::string& plainPassword, const std::string& record) {
    const std::size_t delimiterPosition = record.find('$');
    if (delimiterPosition == std::string::npos) {
        return false;
    }

    const std::string salt = record.substr(0, delimiterPosition);
    const std::string hash = record.substr(delimiterPosition + 1);
    return sha256Hash(salt + plainPassword) == hash;
}

std::string issueToken(const UserAuthRecord& user) {
    const std::time_t currentTime = std::time(nullptr);
    const std::string payload =
        std::to_string(user.userId) + ":" + user.email + ":" + std::to_string(currentTime);
    return "tok_" + sha256Hash(payload + generateSalt());
}

void writeJsonResponse(http::response<http::string_body>& res,
                       http::status status,
                       const std::string& body) {
    res.result(status);
    res.set(http::field::content_type, "application/json");
    res.body() = body;
    res.prepare_payload();
}

}  // namespace

HttpController::HttpController(Database& db) : db_(db) {}

void HttpController::handleListResource(const std::string& resourceName,
                                        http::response<http::string_body>& res) {
    const auto rows = db_.getTableRowsAsJson(resourceName);
    if (!rows.has_value()) {
        writeJsonResponse(res,
                          http::status::not_found,
                          "{\"error\":\"resource not found\"}");
        return;
    }

    writeJsonResponse(res, http::status::ok, rows.value());
}

void HttpController::handleCreateUser(http::request<http::string_body>& req,
                                      http::response<http::string_body>& res) {
    const std::string name = jsonValue(req.body(), "name");
    const std::string email = jsonValue(req.body(), "email");
    const std::string password = jsonValue(req.body(), "password");

    if (name.empty() || email.empty() || password.empty()) {
        writeJsonResponse(
            res,
            http::status::bad_request,
            "{\"error\":\"name, email and password are required\"}");
        return;
    }

    if (db_.userExistsByEmail(email)) {
        writeJsonResponse(
            res,
            http::status::conflict,
            "{\"error\":\"email already exists\"}");
        return;
    }

    if (!db_.createUser(name, email, buildPasswordRecord(password))) {
        writeJsonResponse(
            res,
            http::status::internal_server_error,
            "{\"error\":\"failed to create user\"}");
        return;
    }

    writeJsonResponse(res, http::status::created, "{\"message\":\"user created\"}");
}

void HttpController::handleUpdateUser(http::request<http::string_body>& req,
                                      http::response<http::string_body>& res) {
    const int userId = jsonIntValue(req.body(), "benutzer_id", 0);
    const std::string name = jsonValue(req.body(), "name");
    const std::string email = jsonValue(req.body(), "email");
    const std::string password = jsonValue(req.body(), "password");
    const bool active = jsonBoolValue(req.body(), "aktiv", true);

    if (userId <= 0 || name.empty() || email.empty() || password.empty()) {
        writeJsonResponse(
            res,
            http::status::bad_request,
            "{\"error\":\"benutzer_id, name, email and password are required\"}");
        return;
    }

    if (!db_.updateUser(userId, name, email, buildPasswordRecord(password), active)) {
        writeJsonResponse(
            res,
            http::status::internal_server_error,
            "{\"error\":\"failed to update user\"}");
        return;
    }

    writeJsonResponse(res, http::status::ok, "{\"message\":\"user updated\"}");
}

void HttpController::handleDeleteUser(http::request<http::string_body>& req,
                                      http::response<http::string_body>& res) {
    const int userId = jsonIntValue(req.body(), "benutzer_id", 0);
    if (userId <= 0) {
        writeJsonResponse(
            res,
            http::status::bad_request,
            "{\"error\":\"benutzer_id is required\"}");
        return;
    }

    if (!db_.deleteUser(userId)) {
        writeJsonResponse(
            res,
            http::status::internal_server_error,
            "{\"error\":\"failed to delete user\"}");
        return;
    }

    writeJsonResponse(res, http::status::ok, "{\"message\":\"user deleted\"}");
}

void HttpController::handleLogin(http::request<http::string_body>& req,
                                 http::response<http::string_body>& res) {
    const std::string email = jsonValue(req.body(), "email");
    const std::string password = jsonValue(req.body(), "password");

    if (email.empty() || password.empty()) {
        writeJsonResponse(
            res,
            http::status::bad_request,
            "{\"error\":\"email and password are required\"}");
        return;
    }

    const auto user = db_.getUserAuthByEmail(email);
    if (!user.has_value() || !user->active ||
        !verifyPassword(password, user->passwordHash)) {
        writeJsonResponse(
            res,
            http::status::unauthorized,
            "{\"error\":\"invalid credentials\"}");
        return;
    }

    writeJsonResponse(
        res,
        http::status::ok,
        "{\"message\":\"login successful\",\"token\":\"" + issueToken(user.value()) +
            "\",\"benutzer_id\":" + std::to_string(user->userId) +
            ",\"name\":\"" + escapeJson(user->name) +
            "\",\"email\":\"" + escapeJson(user->email) + "\"}");
}
