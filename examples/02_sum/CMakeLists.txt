add_executable(02_sum_server server.cpp)
target_link_libraries(02_sum_server thallium)
add_executable(02_sum_client client.cpp)
target_link_libraries(02_sum_client thallium)
