cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-Display STATIC ${SRC})
target_link_libraries(hal-Display
  PUBLIC
    pico_stdlib
    hardware_flash
    hardware_dma
    hardware_spi
)
target_compile_options(hal-Display PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(hal-Display
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/Display>"
    "${PROJECT_SOURCE_DIR}/inc")
