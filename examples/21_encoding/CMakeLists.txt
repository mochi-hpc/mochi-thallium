add_executable (21_encoding_server server.cpp)
target_link_libraries (21_encoding_server thallium)
add_executable (21_encoding_client client.cpp)
target_link_libraries (21_encoding_client thallium)
