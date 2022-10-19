cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(maplepad C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(maplepad)

target_compile_definitions(maplepad PRIVATE PICO_HW)

pico_add_extra_outputs(maplepad)

pico_generate_pio_header(maplepad ${CMAKE_CURRENT_LIST_DIR}/src/maple.pio)

target_sources(maplepad PRIVATE src/maple.c src/state_machine.c src/format.c src/ssd1331.c src/ssd1306.c src/font.c src/menu.c src/draw.c)


target_link_libraries(maplepad PRIVATE
        pico_stdlib
        pico_multicore
        pico_time
        hardware_adc
        hardware_pio
        hardware_dma
        hardware_pwm
        hardware_timer
        hardware_i2c
        hardware_spi
        hardware_flash
        )



