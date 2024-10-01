cmake_minimum_required(VERSION 3.5)

project(
  maker-editor-test VERSION 1.0
  DESCRIPTION "Movie Maker like video editor"
  LANGUAGES C
)

#=== TESTING:
include(CTest)
enable_testing()

#=== LIBRARY: test
add_library(test INTERFACE)
target_include_directories(test INTERFACE "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(
  test 
  INTERFACE sokol_log
  ${FFMPEG_LIBRARIES}
)
target_link_directories(test INTERFACE ${FFMPEG_LIBRARY_DIRS})
target_include_directories(test INTERFACE ${FFMPEG_INCLUDE_DIRS})

#=== TEST: hello world
add_executable(hello-world "hello-world.c")
target_link_libraries(hello-world test sokol microui)
add_test(NAME HelloWorld COMMAND hello-world)

#=== TEST: decode
add_executable(
  decode 
  "decode.c"
  "${MAKERLIB_SOURCES}"
)
target_link_libraries(decode test)
add_test(NAME Decode COMMAND test-decode "${CMAKE_SOURCE_DIR}/tests/video.mp4")
