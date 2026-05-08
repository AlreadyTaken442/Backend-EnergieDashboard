cmake_minimum_required(VERSION 3.20)

project(Backend-EnergieDashboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost
find_package(Boost REQUIRED COMPONENTS system)

# Threads
find_package(Threads REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)

# MQTT
find_package(PahoMqttCpp REQUIRED)

# MySQL
find_library(MYSQLCLIENT_LIBRARY
    NAMES mysqlclient mariadbclient
    PATHS /usr/lib64/mysql /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
)

find_path(MYSQL_INCLUDE_DIRS
    NAMES mysql/mysql.h
    PATHS /usr/include /usr/local/include /usr/include/mysql
)

if (NOT MYSQLCLIENT_LIBRARY)
    message(FATAL_ERROR "Could not find libmysqlclient")
endif()

if (NOT MYSQL_INCLUDE_DIRS)
    message(FATAL_ERROR "Could not find MySQL headers")
endif()

set(MYSQL_LIBRARIES "${MYSQLCLIENT_LIBRARY}")

# Executable
add_executable(Backend-EnergieDashboard

    src/Backend-EnergieDashboard.cpp

    src/database/Database.cpp

    src/http/HttpServer.cpp

    src/controllers/HttpController.cpp

    src/mqtt/MqttClient.cpp
)

# Includes
target_include_directories(Backend-EnergieDashboard PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt

    ${MYSQL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Linker
target_link_libraries(Backend-EnergieDashboard PRIVATE

    Boost::system

    Threads::Threads

    OpenSSL::Crypto

    ${MYSQL_LIBRARIES}

    paho-mqttpp3
    paho-mqtt3as

    dl
    m
)