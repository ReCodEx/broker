cmake_minimum_required(VERSION 3.2)
project(codex_broker)

include_directories(AFTER, ../cppzmq)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lzmq")

set(SOURCE_FILES src/main.cpp src/task_router.cpp src/task_router.hpp src/broker_config.cpp src/broker_config.hpp src/broker.cpp src/broker.hpp)
add_executable(codex_broker ${SOURCE_FILES})
