cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-System STATIC ${SRC})
target_link_libraries(hal-System
  PUBLIC
    pico_stdlib
    hardware_flash
    hardware_pwm
)
target_compile_options(hal-System PRIVATE
  -Wall
  -Werror
  -Os
)

target_include_directories(hal-System
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/System>"
    "${PROJECT_SOURCE_DIR}/inc")
