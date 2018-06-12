add_executable(09_sum_provider server.cpp)
target_link_libraries(09_sum_provider thallium)
add_executable(09_sum_client client.cpp)
target_link_libraries(09_sum_client thallium)
