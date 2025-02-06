cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT ENABLE_UNIT_TEST)
  include(pico_sdk_import.cmake)

  pico_sdk_init()
else()
  add_definitions(-DUNITTEST)
endif()

project(MaplePad)

add_subdirectory(src)
