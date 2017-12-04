add_executable(04_stop_server server.cpp)
target_link_libraries(04_stop_server thallium)
add_executable(04_stop_client client.cpp)
target_link_libraries(04_stop_client thallium)
