cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB COMMON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/*.c*")

add_executable(client-with-usb-host "${CMAKE_CURRENT_SOURCE_DIR}/client_usb_host.cpp" "${COMMON_SRC}")
pico_add_extra_outputs(client-with-usb-host)
pico_set_binary_type(client-with-usb-host copy_to_ram)
target_link_libraries(client-with-usb-host
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    clientLib
    pico_stdlib
    hardware_adc
)
target_compile_options(client-with-usb-host PRIVATE
  -Wall
  #-Werror
  -O3
)

target_include_directories(client-with-usb-host
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/client>"
    "${PROJECT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/common")
