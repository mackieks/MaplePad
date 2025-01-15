cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-MapleBus STATIC ${SRC})
pico_generate_pio_header(hal-MapleBus ${CMAKE_CURRENT_LIST_DIR}/maple_in.pio)
pico_generate_pio_header(hal-MapleBus ${CMAKE_CURRENT_LIST_DIR}/maple_out.pio)
target_link_libraries(hal-MapleBus
  PUBLIC
    pico_stdlib
    pico_unique_id
    hardware_pio
    hardware_dma
)
target_compile_options(hal-MapleBus PRIVATE
  -Wall
  -Werror
  -Os
)

target_include_directories(hal-MapleBus
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/MapleBus>"
    "${PROJECT_SOURCE_DIR}/inc")
