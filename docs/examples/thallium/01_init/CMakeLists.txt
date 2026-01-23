add_executable(01_tl_server server.cpp)
target_link_libraries(01_tl_server thallium)

add_executable(01_tl_client client.cpp)
target_link_libraries(01_tl_client thallium)
