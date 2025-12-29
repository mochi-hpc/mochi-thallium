add_executable(TestHelloWorldServer TestHelloWorldServer.cpp)
target_link_libraries(TestHelloWorldServer thallium)

add_executable(TestHelloWorldClient TestHelloWorldClient.cpp)
target_link_libraries(TestHelloWorldClient thallium)

#add_executable(TestSerialization TestSerialization.cpp)
#target_link_libraries(TestSerialization thallium)

add_executable(TestBulkClient TestBulkClient.cpp)
target_link_libraries(TestBulkClient thallium)

add_executable(TestBulkServer TestBulkServer.cpp)
target_link_libraries(TestBulkServer thallium)

# Unit tests with doctest
find_package(doctest REQUIRED)

# List of unit test files
set(UNIT_TEST_FILES
    test_engine
    test_rpc_basic
    test_serialization_primitives
    test_error_handling
    test_endpoint
)

# Create a separate test executable for each test file
foreach(test_name ${UNIT_TEST_FILES})
    add_executable(${test_name}
        unit/test_main.cpp
        unit/${test_name}.cpp
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )

    target_link_libraries(${test_name}
        thallium
        doctest::doctest
    )

    # Add as CTest test
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Optional: Add a target to run all unit tests
add_custom_target(run_all_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${UNIT_TEST_FILES}
    COMMENT "Running all unit tests"
)
