add_executable(07_rdma_server server.cpp)
target_link_libraries(07_rdma_server thallium)
add_executable(07_rdma_client client.cpp)
target_link_libraries(07_rdma_client thallium)
