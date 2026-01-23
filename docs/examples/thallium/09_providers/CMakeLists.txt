add_executable(09_tl_server server.cpp)
target_link_libraries(09_tl_server thallium)

add_executable(09_tl_client client.cpp)
target_link_libraries(09_tl_client thallium)
