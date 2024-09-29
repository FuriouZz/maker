cmake_minimum_required(VERSION 3.5)

project(
  maker-editor-test VERSION 1.0
  DESCRIPTION "Movie Maker like video editor"
  LANGUAGES C
)

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

#=== EXECUTABLE: decode
add_executable(
  test-decode 
  "decode.c"
  "${CMAKE_SOURCE_DIR}/src/maker_player.c"
  "${CMAKE_SOURCE_DIR}/src/maker_util.c"
)
target_link_libraries(test-decode test)

#=== TESTING:
include(CTest)
enable_testing()
add_test(NAME TestDecode COMMAND test-decode "${CMAKE_SOURCE_DIR}/tests/video.mp4")
