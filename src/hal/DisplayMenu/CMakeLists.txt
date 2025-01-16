cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-DisplayMenu STATIC ${SRC})
target_link_libraries(hal-DisplayMenu
  PUBLIC
    pico_stdlib
    hardware_flash
    hardware_adc
    hal-Display
)
target_compile_options(hal-DisplayMenu PRIVATE
  -Wall
  -Werror
  -Os
)

target_include_directories(hal-DisplayMenu
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/DisplayMenu>"
    "${PROJECT_SOURCE_DIR}/inc")
