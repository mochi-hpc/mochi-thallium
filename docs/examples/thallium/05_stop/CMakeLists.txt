add_executable(05_tl_server server.cpp)
target_link_libraries(05_tl_server thallium)

add_executable(05_tl_client client.cpp)
target_link_libraries(05_tl_client thallium)
