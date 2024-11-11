cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-Usb-Host STATIC ${SRC})
target_link_libraries(hal-Usb-Host
  PUBLIC
    pico_stdlib
    tinyusb_board
    tinyusb_host
)
target_compile_options(hal-Usb-Host PRIVATE
  -Wall
  #-Werror
  -O3
)

# This is a workaround for a bug in pico-sdk under usb host
target_compile_definitions(tinyusb_host INTERFACE NDEBUG=1)

target_include_directories(hal-Usb-Host
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../Common"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/Usb>"
    "${PROJECT_SOURCE_DIR}/inc")
