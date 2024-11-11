cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

  # Specify the commit you depend on and update it regularly.
set(GOOGLE_TEST_COMMIT_ID "4219e7254cb8c473f57f6065bd13d1520d7b708f")
include(FetchContent)
FetchContent_Declare(
  googletest
  URL "https://github.com/google/googletest/archive/${GOOGLE_TEST_COMMIT_ID}.zip"
)
FetchContent_MakeAvailable(googletest)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_executable(testExe
  ${SRC}
)
target_link_libraries(testExe
  PRIVATE
    gtest_main
    gmock_main
    pthread
    -Wl,--whole-archive
    testHostLib
    -Wl,--no-whole-archive
)

target_include_directories(testExe
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/test>"
    "${PROJECT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_LIST_DIR}/mocks")

enable_testing()
add_test(NAME all_tests COMMAND testExe)
add_custom_target(test
  COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --verbose --output-on-failure
  DEPENDS testExe)
