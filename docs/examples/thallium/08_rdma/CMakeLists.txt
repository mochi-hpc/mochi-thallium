add_executable(08_tl_server server.cpp)
target_link_libraries(08_tl_server thallium)

add_executable(08_tl_client client.cpp)
target_link_libraries(08_tl_client thallium)
