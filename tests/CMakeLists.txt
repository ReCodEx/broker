project(recodex-broker_tests)

set(SRC_DIR ../src)
set(LIBS_DIR ../vendor)
set(HELPERS_DIR ${SRC_DIR}/helpers)

# Google Test and Google Mock headers
include_directories(${LIBS_DIR}/googletest/include)
include_directories(${LIBS_DIR}/googlemock/include)

set(LIBS
	gtest gtest_main
	gmock gmock_main
	yaml-cpp
	-lzmq
	-lcurl
	-lboost_system -lboost_filesystem
	-lgcov --coverage
)

function(add_test_suite name)
	add_executable(run_test_${name} ${ARGN})
	target_link_libraries(run_test_${name} ${LIBS})
	set_target_properties(run_test_${name} PROPERTIES COMPILE_FLAGS "-fprofile-arcs -ftest-coverage --coverage")
	add_test(${name} run_test_${name})
endfunction()

add_test_suite(broker_config
	mocks.h
	broker_config.cpp
	${SRC_DIR}/config/broker_config.cpp
)

add_test_suite(worker_registry
	mocks.h
	worker_registry.cpp
	${SRC_DIR}/worker_registry.cpp
	${SRC_DIR}/worker.cpp
	${HELPERS_DIR}/logger.cpp
	${HELPERS_DIR}/string_to_hex.cpp
)

add_test_suite(multi_queue_manager
	multi_queue_manager.cpp
	${SRC_DIR}/queuing/multi_queue_manager.cpp
    ${SRC_DIR}/worker.cpp
    ${HELPERS_DIR}/string_to_hex.cpp
)

add_test_suite(broker
	mocks.h
	broker.cpp
	${SRC_DIR}/config/broker_config.cpp
	${SRC_DIR}/broker_connect.cpp
	${SRC_DIR}/handlers/broker_handler.cpp
	${SRC_DIR}/handlers/status_notifier_handler.cpp
	${HELPERS_DIR}/string_to_hex.cpp
	${HELPERS_DIR}/logger.cpp
	${SRC_DIR}/worker_registry.cpp
	${SRC_DIR}/worker.cpp
	${SRC_DIR}/helpers/string_to_hex.cpp
	${SRC_DIR}/helpers/curl.cpp
	${SRC_DIR}/reactor/message_container.cpp
	${SRC_DIR}/reactor/reactor.cpp
	${SRC_DIR}/reactor/socket_wrapper_base.cpp
	${SRC_DIR}/reactor/router_socket_wrapper.cpp
	${SRC_DIR}/reactor/command_holder.cpp
	${SRC_DIR}/notifier/reactor_status_notifier.cpp
    ${SRC_DIR}/queuing/multi_queue_manager.cpp
)

add_test_suite(worker
	mocks.h
	worker.cpp
	${SRC_DIR}/worker.cpp
	${SRC_DIR}/helpers/string_to_hex.cpp
)

add_test_suite(reactor
	reactor.cpp
	${SRC_DIR}/reactor/message_container.cpp
	${SRC_DIR}/reactor/reactor.cpp
	${SRC_DIR}/reactor/socket_wrapper_base.cpp
	${SRC_DIR}/reactor/router_socket_wrapper.cpp
)