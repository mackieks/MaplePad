cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(ENABLE_UNIT_TEST)
    add_subdirectory(test)
else()
    add_subdirectory(hal)
    add_subdirectory(main)
endif()

add_subdirectory(hostLib)
add_subdirectory(clientLib)