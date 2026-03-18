#include "Database.h"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {

std::string escapeSql(MYSQL* connection, const std::string& value) {
    std::string escaped;
    escaped.resize(value.size() * 2 + 1);
    const auto escapedLength = mysql_real_escape_string(
        connection,
        escaped.data(),
        value.c_str(),
        static_cast<unsigned long>(value.size()));
    escaped.resize(escapedLength);
    return escaped;
}

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

std::optional<std::string> executeJsonQuery(MYSQL* connection, const std::string& query) {
    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Query failed: " << mysql_error(connection) << std::endl;
        return std::nullopt;
    }

    MYSQL_RES* rawResult = mysql_store_result(connection);
    if (rawResult == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(connection) << std::endl;
        return std::nullopt;
    }

    std::ostringstream json;
    json << "[";

    const unsigned int fieldCount = mysql_num_fields(rawResult);
    MYSQL_FIELD* fields = mysql_fetch_fields(rawResult);
    MYSQL_ROW row = nullptr;
    unsigned long* lengths = nullptr;
    bool firstRow = true;

    while ((row = mysql_fetch_row(rawResult)) != nullptr) {
        lengths = mysql_fetch_lengths(rawResult);
        if (!firstRow) {
            json << ",";
        }

        json << "{";
        for (unsigned int index = 0; index < fieldCount; ++index) {
            if (index > 0) {
                json << ",";
            }

            json << "\"" << fields[index].name << "\":";
            if (row[index] == nullptr) {
                json << "null";
                continue;
            }

            const std::string value(row[index], lengths[index]);
            json << "\"" << escapeJson(value) << "\"";
        }
        json << "}";
        firstRow = false;
    }

    json << "]";
    mysql_free_result(rawResult);
    return json.str();
}

const std::map<std::string, std::string>& tableQueries() {
    static const std::map<std::string, std::string> queries = {
        {"users", "SELECT benutzer_id, name, email, erstellt_am, aktiv FROM benutzer ORDER BY benutzer_id"},
        {"notifications", "SELECT benachrichtigung_id, geraet_id, benutzer_id, zeitstempel, nachricht, schweregrad FROM benachrichtigung ORDER BY benachrichtigung_id"},
        {"user-roles", "SELECT benutzer_id, rolle_id FROM benutzer_rolle ORDER BY benutzer_id, rolle_id"},
        {"permissions", "SELECT berechtigung_id, name, beschreibung FROM berechtigung ORDER BY berechtigung_id"},
        {"buildings", "SELECT gebaeude_id, name, adresse FROM gebaeude ORDER BY gebaeude_id"},
        {"devices", "SELECT geraet_id, name, hersteller, update_status, raum_id, typ_id, erstellt_am FROM geraet ORDER BY geraet_id"},
        {"device-types", "SELECT typ_id, bezeichnung, beschreibung FROM geraetetyp ORDER BY typ_id"},
        {"usage-statistics", "SELECT nutzung_id, geraet_id, datum, nutzungsdauer_minuten FROM nutzungsstatistik ORDER BY nutzung_id"},
        {"rooms", "SELECT raum_id, gebaeude_id, raum_nummer, bezeichnung FROM raum ORDER BY raum_id"},
        {"roles", "SELECT rolle_id, name, beschreibung FROM rolle ORDER BY rolle_id"},
        {"role-permissions", "SELECT rolle_id, berechtigung_id FROM rollen_berechtigungen ORDER BY rolle_id, berechtigung_id"},
        {"sensor-data", "SELECT sensordaten_id, geraet_id, zeitstempel, stromverbrauch, status FROM sensordaten ORDER BY sensordaten_id"}
    };

    return queries;
}

}  // namespace

Database::Database(const std::string& host,
                   const std::string& user,
                   const std::string& pass,
                   const std::string& dbname)
    : conn(nullptr), host(host), user(user), pass(pass), dbname(dbname) {
    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        throw std::runtime_error("MySQL initialization failed.");
    }

    if (mysql_real_connect(conn,
                           host.c_str(),
                           user.c_str(),
                           pass.c_str(),
                           dbname.c_str(),
                           3306,
                           nullptr,
                           0) == nullptr) {
        throw std::runtime_error(
            "MySQL connection failed: " + std::string(mysql_error(conn)));
    }
}

Database::~Database() {
    if (conn != nullptr) {
        mysql_close(conn);
    }
}

MYSQL* Database::getConnection() {
    return conn;
}

bool Database::createUser(const std::string& name,
                          const std::string& email,
                          const std::string& passwordHash) {
    MYSQL* connection = getConnection();

    const std::string safeName = escapeSql(connection, name);
    const std::string safeEmail = escapeSql(connection, email);
    const std::string safePasswordHash = escapeSql(connection, passwordHash);

    const std::string query =
        "INSERT INTO benutzer (name, email, passwort_hash) VALUES ('" +
        safeName + "', '" + safeEmail + "', '" + safePasswordHash + "')";

    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Insert failed: " << mysql_error(connection) << std::endl;
        return false;
    }

    return true;
}

bool Database::userExistsByEmail(const std::string& email) {
    MYSQL* connection = getConnection();
    const std::string safeEmail = escapeSql(connection, email);
    const std::string query =
        "SELECT 1 FROM benutzer WHERE email='" + safeEmail + "' LIMIT 1";

    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Query failed: " << mysql_error(connection) << std::endl;
        return false;
    }

    MYSQL_RES* rawResult = mysql_store_result(connection);
    if (rawResult == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(connection) << std::endl;
        return false;
    }

    const bool exists = mysql_num_rows(rawResult) > 0;
    mysql_free_result(rawResult);
    return exists;
}

std::optional<UserAuthRecord> Database::getUserAuthByEmail(const std::string& email) {
    MYSQL* connection = getConnection();
    const std::string safeEmail = escapeSql(connection, email);
    const std::string query =
        "SELECT benutzer_id, name, email, passwort_hash, aktiv "
        "FROM benutzer WHERE email='" + safeEmail + "' LIMIT 1";

    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Query failed: " << mysql_error(connection) << std::endl;
        return std::nullopt;
    }

    MYSQL_RES* rawResult = mysql_store_result(connection);
    if (rawResult == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(connection) << std::endl;
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(rawResult);
    if (row == nullptr) {
        mysql_free_result(rawResult);
        return std::nullopt;
    }

    UserAuthRecord record{
        row[0] == nullptr ? 0 : std::stoi(row[0]),
        row[1] == nullptr ? "" : row[1],
        row[2] == nullptr ? "" : row[2],
        row[3] == nullptr ? "" : row[3],
        row[4] != nullptr && std::string(row[4]) == "1"
    };

    mysql_free_result(rawResult);
    return record;
}

bool Database::updateUser(int id,
                          const std::string& name,
                          const std::string& email,
                          const std::string& passwordHash,
                          bool active) {
    MYSQL* connection = getConnection();

    const std::string safeName = escapeSql(connection, name);
    const std::string safeEmail = escapeSql(connection, email);
    const std::string safePasswordHash = escapeSql(connection, passwordHash);
    const std::string query =
        "UPDATE benutzer SET "
        "name='" + safeName + "', "
        "email='" + safeEmail + "', "
        "passwort_hash='" + safePasswordHash + "', "
        "aktiv=" + std::string(active ? "1" : "0") +
        " WHERE benutzer_id=" + std::to_string(id);

    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Update failed: " << mysql_error(connection) << std::endl;
        return false;
    }

    return true;
}

bool Database::deleteUser(int id) {
    MYSQL* connection = getConnection();
    const std::string query =
        "DELETE FROM benutzer WHERE benutzer_id=" + std::to_string(id);

    if (mysql_query(connection, query.c_str()) != 0) {
        std::cerr << "Delete failed: " << mysql_error(connection) << std::endl;
        return false;
    }

    return true;
}

std::optional<std::string> Database::getTableRowsAsJson(const std::string& tableName) {
    const auto queryIt = tableQueries().find(tableName);
    if (queryIt == tableQueries().end()) {
        return std::nullopt;
    }

    return executeJsonQuery(getConnection(), queryIt->second);
}
