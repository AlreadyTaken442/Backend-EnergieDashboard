cmake_minimum_required(VERSION 3.20)

project(Backend-EnergieDashboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(Backend-EnergieDashboard
    src/Backend-EnergieDashboard.cpp
)

target_link_libraries(Backend-EnergieDashboard
    Boost::system
    mysqlcppconn
    Threads::Threads
)