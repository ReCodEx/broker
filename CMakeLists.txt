cmake_minimum_required(VERSION 3.2)

project(codex_broker)

include_directories(AFTER, vendor/cppzmq)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main.cpp
        src/task_router.cpp
        src/task_router.hpp
        src/broker_config.cpp
        src/broker_config.hpp
        src/broker.cpp
        src/broker.hpp
)

add_executable(codex_broker ${SOURCE_FILES})
target_link_libraries(codex_broker -lzmq)

# Include Google Test libraries and then our very own unit tests
add_subdirectory(vendor/googletest)
add_subdirectory(tests)
