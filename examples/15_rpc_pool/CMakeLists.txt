add_executable(15_sum_server server.cpp)
target_link_libraries(15_sum_server thallium)
add_executable(15_sum_client client.cpp)
target_link_libraries(15_sum_client thallium)
