cmake_minimum_required(VERSION 3.20)

project(Backend-EnergieDashboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Boost finden (Beast benötigt oft system)
find_package(Boost REQUIRED COMPONENTS system)

# 2. Threads finden (Zwingend für Boost Asio/Beast)
find_package(Threads REQUIRED)

# 3. MySQL Konfiguration für Fedora (mysql-devel Paket)
# Wir definieren die Bibliothek direkt, da 'mysqlcppconn' bei dir fehlt
set(MYSQL_LIBRARIES "/usr/lib64/mysql/libmysqlclient.so")
set(MYSQL_INCLUDE_DIRS "/usr/include/mysql")

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
    ${MYSQL_LIBRARIES}
    dl    # Wird oft von mysqlclient benötigt
    m     # Math library
)
