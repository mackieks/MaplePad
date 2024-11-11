cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*" "${CMAKE_CURRENT_SOURCE_DIR}/../Common/*.c*")

add_library(hal-Usb-Client-Hid STATIC ${SRC})
target_link_libraries(hal-Usb-Client-Hid
  PUBLIC
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
    tinyusb_device_base
)
target_compile_options(hal-Usb-Client-Hid PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(hal-Usb-Client-Hid
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../Common"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/Usb>"
    "${PROJECT_SOURCE_DIR}/inc")
