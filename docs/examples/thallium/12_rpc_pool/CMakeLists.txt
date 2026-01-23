add_executable(12_tl_server server.cpp)
target_link_libraries(12_tl_server thallium)

add_executable(12_tl_client client.cpp)
target_link_libraries(12_tl_client thallium)
