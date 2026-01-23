add_executable(14_tl_server server.cpp)
target_link_libraries(14_tl_server thallium)

add_executable(14_tl_client client.cpp)
target_link_libraries(14_tl_client thallium)
