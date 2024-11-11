cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(clientLib STATIC ${SRC})

target_link_libraries(clientLib
  PRIVATE
    # TODO: move this to HAL
    hardware_flash
)

if(NOT ENABLE_UNIT_TEST)
  target_compile_options(clientLib PRIVATE
    -Wall
    -Werror
    -O3
  )
endif()

target_include_directories(clientLib
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/core>"
    "${PROJECT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/peripherals"
    "${CMAKE_CURRENT_SOURCE_DIR}/parsers")
