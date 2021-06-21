
#add_executable(my_test test.cpp)
#target_link_libraries(my_test cereal)
add_executable(16_context_server server.cpp)
target_link_libraries(16_context_server thallium)
add_executable(16_context_client client.cpp)
target_link_libraries(16_context_client thallium)
