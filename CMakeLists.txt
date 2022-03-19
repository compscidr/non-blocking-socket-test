cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 17)

project(non-blocking-socket-test)

add_executable(nbs-server nbs-server.cpp common.cpp)
add_executable(nbs-client nbs-client.cpp common.cpp)
