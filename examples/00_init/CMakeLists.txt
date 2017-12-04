add_executable(00_init_server server.cpp)
target_link_libraries(00_init_server thallium)
add_executable(00_init_client client.cpp)
target_link_libraries(00_init_client thallium)
