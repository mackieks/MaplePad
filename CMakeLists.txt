cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT ENABLE_UNIT_TEST)
  # Have pico_sdk_import.cmake use the local SDK if it was pulled down; otherwise, use path at
  # PICO_SDK_PATH environment variable as specified within pico_sdk_import.cmake
  if (EXISTS "${CMAKE_SOURCE_DIR}/ext/pico-sdk/CMakeLists.txt")
    set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/ext/pico-sdk/" CACHE INTERNAL PICO_SDK_PATH)
    message("Using local pico SDK at ('${PICO_SDK_PATH}')")
  endif ()

  if (NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH not defined; either execute:\ngit submodule update --recursive --init\nor set PICO_SDK_PATH in environment")
  endif ()

  include(pico_sdk_import.cmake)
  #include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

  pico_sdk_init()
else()
  add_definitions(-DUNITTEST)
endif()

project(MaplePad)

add_subdirectory(src)
