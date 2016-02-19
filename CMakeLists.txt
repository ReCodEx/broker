cmake_minimum_required(VERSION 3.2)

project(codex_broker)
enable_testing()

# ZeroMQ C++ bindings headers
include_directories(AFTER, vendor/cppzmq)

# Spdlog logging library
include_directories(AFTER, vendor/spdlog/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main.cpp
        src/task_router.cpp
        src/task_router.h
        src/broker_config.cpp
        src/broker_config.h
        src/broker.h
        src/connection_proxy.h
)

add_executable(codex_broker ${SOURCE_FILES})
target_link_libraries(codex_broker yaml-cpp)
target_link_libraries(codex_broker -lzmq)
target_link_libraries(codex_broker -lboost_system)
target_link_libraries(codex_broker -lboost_filesystem)

# Include Google Test libraries and then our very own unit tests
add_subdirectory(vendor/googletest)
add_subdirectory(tests)
