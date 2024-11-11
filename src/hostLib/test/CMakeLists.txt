cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(testHostLib STATIC ${SRC})

target_link_libraries(testHostLib
  PUBLIC
    hostLib
    gtest_main
    gmock_main
)

target_include_directories(testHostLib
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/test>"
    "${PROJECT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_LIST_DIR}/mocks")
