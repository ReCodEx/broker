project(codex_broker_tests)

set(SRC_DIR ../src)
set(LIBS_DIR ../vendor)

# Google Test and Google Mock headers
include_directories(${LIBS_DIR}/googletest/include)
include_directories(${LIBS_DIR}/googlemock/include)

set(LIBS
	gtest gtest_main
	gmock gmock_main
	yaml-cpp
	-lzmq
	-lboost_system -lboost_filesystem
)

function(add_test_suite name)
	add_executable(run_test_${name} ${ARGN})
	target_link_libraries(run_test_${name} ${LIBS})
	add_test(${name} run_test_${name})
endfunction()

add_test_suite(broker_config
	broker_config.cpp
	${SRC_DIR}/config/broker_config.cpp
)

add_test_suite(worker_registry
	worker_registry.cpp
	${SRC_DIR}/worker_registry.cpp
)

add_test_suite(broker
	broker.cpp
	${SRC_DIR}/config/broker_config.cpp
	${SRC_DIR}/worker_registry.cpp
	${SRC_DIR}/worker.cpp
)

add_test_suite(worker
	worker.cpp
	${SRC_DIR}/worker.cpp
)