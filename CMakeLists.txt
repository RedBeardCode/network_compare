cmake_minimum_required(VERSION 3.6)
project(network_compare)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES_SERVER server.cpp)
set(SOURCE_FILES_CLIENT client.cpp)
add_executable(server ${SOURCE_FILES_SERVER})
add_executable(client ${SOURCE_FILES_CLIENT})
target_link_libraries(server PRIVATE  zmq pthread)
target_link_libraries(client PRIVATE  zmq)