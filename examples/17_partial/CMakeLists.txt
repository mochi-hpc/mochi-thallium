add_executable(17_partial_server server.cpp)
target_link_libraries(17_partial_server thallium)
add_executable(17_partial_client client.cpp)
target_link_libraries(17_partial_client thallium)
