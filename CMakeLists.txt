cmake_minimum_required(VERSION 2.8.11)
project(recodex-broker)
set(RECODEX_VERSION "1.0.0")

set(EXEC_NAME ${PROJECT_NAME})
enable_testing()

# ZeroMQ C++ bindings headers
include_directories(AFTER, vendor/cppzmq)

# Spdlog logging library
include_directories(AFTER, vendor/spdlog/include)

# Find CURL
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(SOURCE_FILES
	src/main.cpp
	src/worker_registry.cpp
	src/worker_registry.h
	src/config/broker_config.cpp
	src/config/broker_config.h
	src/config/log_config.h
	src/config/notifier_config.h
	src/broker_connect.h
	src/broker_core.h
	src/broker_core.cpp
	src/reactor/command_holder.h
	src/helpers/string_to_hex.h
	src/helpers/string_to_hex.cpp
	src/helpers/logger.h
	src/helpers/logger.cpp
	src/helpers/curl.h
	src/helpers/curl.cpp
	src/worker.cpp
	src/notifier/status_notifier.h
	src/notifier/http_status_notifier.h
	src/notifier/http_status_notifier.cpp
	src/notifier/empty_status_notifier.h
	src/reactor/reactor.cpp
	src/reactor/socket_wrapper_base.cpp
	src/reactor/socket_wrapper_base.h
	src/reactor/message_container.cpp
	src/reactor/handler_interface.h
	src/reactor/router_socket_wrapper.h
	src/reactor/router_socket_wrapper.cpp
	src/handlers/broker_handler.h
	src/handlers/status_notifier_handler.h
	src/handlers/status_notifier_handler.cpp
	src/handlers/broker_handler.cpp
	src/notifier/reactor_status_notifier.cpp
	src/notifier/reactor_status_notifier.h
	src/broker_connect.cpp
	src/reactor/command_holder.cpp
	src/queuing/queue_manager_interface.h
	src/queuing/multi_queue_manager.cpp
	src/queuing/multi_queue_manager.h
)

add_executable(${EXEC_NAME} ${SOURCE_FILES})
target_link_libraries(${EXEC_NAME} yaml-cpp)
target_link_libraries(${EXEC_NAME} -lzmq)
target_link_libraries(${EXEC_NAME} -lpthread)
target_link_libraries(${EXEC_NAME} -lboost_system)
target_link_libraries(${EXEC_NAME} -lboost_filesystem)
target_link_libraries(${EXEC_NAME} -lboost_program_options)
target_link_libraries(${EXEC_NAME} ${CURL_LIBRARIES})

if(NOT DISABLE_TESTS)
	# Include Google Test libraries and then our very own unit tests
	add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)
	add_subdirectory(tests)
endif()


# ========== Install targets - 'sudo make install' ==========
include(InstallRequiredSystemLibraries)
if(UNIX)
	install(TARGETS recodex-broker DESTINATION /usr/bin COMPONENT binaries)
	install(FILES install/recodex-broker.service DESTINATION /lib/systemd/system COMPONENT config)
	install(FILES examples/config.yml DESTINATION /etc/recodex/broker COMPONENT config)
	install(FILES install/munin-broker DESTINATION /etc/munin/plugins COMPONENT config)
else()
	install(TARGETS recodex-broker DESTINATION recodex/broker/bin COMPONENT binaries)
	install(FILES examples/config.yml DESTINATION recodex/broker/conf COMPONENT config)
endif()


# ========== Packaging targets - 'sudo make package' ==========
set(CPACK_PACKAGE_NAME "recodex-broker")
set(CPACK_PACKAGE_VENDOR "ReCodEx Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ReCodEx Broker - Job distribution part of ReCodEx programmer testing solution")
set(CPACK_COMPONENTS_ALL "binaries" "config")
set(CPACK_PACKAGE_VERSION ${RECODEX_VERSION})
set(CPACK_GENERATOR "RPM" "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_PACKAGE_RELOCATABLE "FALSE")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Job distribution part of ReCodEx programmer testing solution.")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/install/postinst")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Petr Stefan <ps@stdin.cz>")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/install/postinst;")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${RECODEX_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}-${CPACK_RPM_PACKAGE_ARCHITECTURE}")
include (CPack)


# ========== Formatting ==========
file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp src/*.h tests/*.cpp tests/*.h)
add_custom_target(format
	COMMAND clang-format --style=file -i ${ALL_SOURCE_FILES}
	COMMENT "Running clang-format"
	VERBATIM
)

# Count sum of lines in source files (for Martin)
add_custom_target(lines
	COMMAND echo -n "Total lines of code: " && cat ${ALL_SOURCE_FILES} | wc -l
	COMMENT "Counting lines"
	VERBATIM
)

