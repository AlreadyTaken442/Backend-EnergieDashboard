cmake_minimum_required(VERSION 3.20)

project(Backend-EnergieDashboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Boost finden (Beast benötigt oft system)
find_package(Boost REQUIRED COMPONENTS system)

# 2. Threads finden (Zwingend für Boost Asio/Beast)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# 3. MySQL Konfiguration
# Versuche, die MySQL-Clientbibliothek (libmysqlclient) automatisch zu finden.
find_library(MYSQLCLIENT_LIBRARY
    NAMES mysqlclient mariadbclient
    PATHS /usr/lib64/mysql /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
    NO_DEFAULT_PATH
)

find_path(MYSQL_INCLUDE_DIRS
    NAMES mysql/mysql.h
    PATHS /usr/include /usr/local/include /usr/include/mysql
    NO_DEFAULT_PATH
)

if (NOT MYSQLCLIENT_LIBRARY)
    message(FATAL_ERROR "Could not find libmysqlclient. Install libmysqlclient-dev (Debian/Ubuntu) or mariadb-connector-c and/or set MYSQL_LIBRARIES manually.")
endif()

if (NOT MYSQL_INCLUDE_DIRS)
    message(FATAL_ERROR "Could not find MySQL headers. Install libmysqlclient-dev (Debian/Ubuntu) or set MYSQL_INCLUDE_DIRS manually.")
endif()

set(MYSQL_LIBRARIES "${MYSQLCLIENT_LIBRARY}")

# 4. Das Executable erstellen
add_executable(Backend-EnergieDashboard
    src/Backend-EnergieDashboard.cpp
    src/database/Database.cpp
    src/http/HttpServer.cpp
    src/controllers/HttpController.cpp
)

# 5. Header-Pfade zuweisen
target_include_directories(Backend-EnergieDashboard PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src            # <--- WICHTIG: Dein Source-Ordner für lokale Header
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database   # Falls Header direkt dort liegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${MYSQL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# 6. Bibliotheken linken
target_link_libraries(Backend-EnergieDashboard PRIVATE
    Boost::system
    Threads::Threads
    OpenSSL::Crypto
    ${MYSQL_LIBRARIES}
    dl    # Wird oft von mysqlclient benötigt
    m     # Math library
)
