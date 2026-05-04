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
    test_serialization_stl
    test_rpc_advanced
    test_provider
    test_bulk_transfers
    test_serialization_custom
    test_serialization_context
    test_async_operations
    test_finalization
    test_configuration
    test_argobots_pools
    test_argobots_xstreams
    test_argobots_threads
    test_argobots_schedulers
    test_sync_primitives
    test_timed_callbacks
    test_logging
    test_edge_cases
    test_bulk_buffer
)

# Create a separate test executable for each test file
foreach(test_name ${UNIT_TEST_FILES})
    add_executable(${test_name}
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

# Precompile the headers shared by every unit test into a single .gch
# that captures the full transitive include tree of thallium.hpp (engine, bulk,
# Argobots wrappers, Cereal, Margo, etc.).  All remaining targets reuse it via
# REUSE_FROM, so the compiler parses those headers only once instead of once
# per test executable.  test_check_types is kept separate because it links
# thallium_check_types which may add compile definitions incompatible with the
# shared PCH.
list(GET UNIT_TEST_FILES 0 _pch_donor)
target_precompile_headers(${_pch_donor}
    PRIVATE
        <thallium.hpp>
        "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_helpers.hpp"
)
foreach(test_name IN LISTS UNIT_TEST_FILES)
    if(NOT test_name STREQUAL _pch_donor)
        target_precompile_headers(${test_name} REUSE_FROM ${_pch_donor})
    endif()
endforeach()

# test_check_types links against thallium_check_types to enable RPC type
# checking (-DTHALLIUM_DEBUG_RPC_TYPES).  It is kept separate from the
# standard foreach loop because it needs a different link line.
add_executable(test_check_types unit/test_check_types.cpp)
target_include_directories(test_check_types PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit)
target_link_libraries(test_check_types thallium thallium_check_types doctest::doctest)
add_test(NAME test_check_types COMMAND test_check_types)

# Optional: Add a target to run all unit tests
add_custom_target(run_all_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${UNIT_TEST_FILES} test_check_types
    COMMENT "Running all unit tests"
)
