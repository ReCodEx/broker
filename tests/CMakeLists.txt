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
	set(TEST_FILES ${TEST_FILES} ${ARGN} PARENT_SCOPE)
	add_executable(run_test_${name} ${ARGN})
	target_link_libraries(run_test_${name} ${LIBS})
endfunction()

add_test_suite(broker_config
	broker_config.cpp
	${SRC_DIR}/broker_config.cpp
)

add_test_suite(task_router
	task_router.cpp
	${SRC_DIR}/task_router.cpp
)

add_test_suite(broker
	broker.cpp
	${SRC_DIR}/broker_config.cpp
	${SRC_DIR}/task_router.cpp
)

# The main test executable
list(REMOVE_DUPLICATES TEST_FILES)
add_executable(run_tests ${TEST_FILES})
target_link_libraries(run_tests ${LIBS})
