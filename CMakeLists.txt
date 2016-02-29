cmake_minimum_required(VERSION 3.2)
project(recodex-broker)
set(RECODEX_VERSION "0.1.2")

set(EXEC_NAME ${PROJECT_NAME})
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
	src/config/broker_config.cpp
	src/config/broker_config.h
	src/config/log_config.h
	src/broker_connect.h
	src/broker_core.h
	src/broker_core.cpp
	src/connection_proxy.h
	src/commands/command_holder.h
	src/commands/client_commands.h
	src/commands/worker_commands.h
)

add_executable(${EXEC_NAME} ${SOURCE_FILES})
target_link_libraries(${EXEC_NAME} yaml-cpp)
target_link_libraries(${EXEC_NAME} -lzmq)
target_link_libraries(${EXEC_NAME} -lboost_system)
target_link_libraries(${EXEC_NAME} -lboost_filesystem)
target_link_libraries(${EXEC_NAME} -lboost_program_options)

# Include Google Test libraries and then our very own unit tests
add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)
add_subdirectory(tests)


# ========== Install targets - 'sudo make install' ==========
include(InstallRequiredSystemLibraries)
if(UNIX)
	install(TARGETS recodex-broker DESTINATION /usr/bin COMPONENT binaries)
	install(FILES install/recodex-broker.service DESTINATION /etc/systemd/system COMPONENT config)
	install(FILES examples/config.yml DESTINATION /etc/recodex/broker COMPONENT config)

	install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install/postinst)")
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
