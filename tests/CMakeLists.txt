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

#=== TEST: hello world
add_executable(test_hello "test_hello.c")
target_link_libraries(test_hello
  test
  sokol
  microui
  ffmpeg
)
target_compile_definitions(test_hello PUBLIC 
  USE_SOKOL_LOG 
  USE_SOKOL_GFX 
  USE_SOKOL_APP 
  USE_SOKOL_GLUE
)
add_test(NAME test_hello COMMAND test_hello)

#=== TEST: decode
add_executable(test_decode "test_decode.c" "maker_play_ext.h" "${MAKERLIB_SOURCES}")
target_link_libraries(test_decode test sokol microui ffmpeg)
target_compile_definitions(test_decode PUBLIC USE_SOKOL_LOG)
add_test(
  NAME test_decode
  COMMAND test_decode
  "${CMAKE_SOURCE_DIR}/tests/video.mp4"
  "${CMAKE_SOURCE_DIR}/tmp/output.pgm"
  "${CMAKE_SOURCE_DIR}/tmp/output.ppm"
)

#=== TEST: decode_video
add_executable(test_decode_video "test_decode_video.c" "maker_play_ext.h"  "${MAKERLIB_SOURCES}")
target_link_libraries(test_decode_video test sokol microui ffmpeg)
target_compile_definitions(test_decode_video PUBLIC
  USE_SOKOL_LOG
  USE_SOKOL_GFX
  USE_SOKOL_APP
  USE_SOKOL_GLUE
)
add_test(
  NAME test_decode_video 
  COMMAND test_decode_video 
  "${CMAKE_SOURCE_DIR}/tests/video.mp4"
)

