CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(linux-network-programming C CXX)

# enabling the c99 for c and cpp11 for cpp
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


ADD_SUBDIRECTORY(socket-api)