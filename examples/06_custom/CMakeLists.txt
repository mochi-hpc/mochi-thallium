add_executable(06_custom_server server.cpp)
target_link_libraries(06_custom_server thallium)
add_executable(06_custom_client client.cpp)
target_link_libraries(06_custom_client thallium)
