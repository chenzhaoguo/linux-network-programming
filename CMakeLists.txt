CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(linux-network-programming C CXX)

# enabling the c99 for c and cpp11 for cpp
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF()
SET(SYSTEM_LIBS pthread m dl rt)

ADD_SUBDIRECTORY(socket-api)
ADD_SUBDIRECTORY(tests)
