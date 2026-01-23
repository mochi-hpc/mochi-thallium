add_executable(04_tl_server server.cpp)
target_link_libraries(04_tl_server thallium)

add_executable(04_tl_client client.cpp)
target_link_libraries(04_tl_client thallium)
