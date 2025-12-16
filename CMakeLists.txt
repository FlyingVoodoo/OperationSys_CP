cmake_minimum_required(VERSION 3.10)
project(BullsAndCows LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для macOS/Linux
if(APPLE OR UNIX)
    find_package(Threads REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(server)
add_subdirectory(client)
