add_executable(05_stl_server server.cpp)
target_link_libraries(05_stl_server thallium)
add_executable(05_stl_client client.cpp)
target_link_libraries(05_stl_client thallium)
