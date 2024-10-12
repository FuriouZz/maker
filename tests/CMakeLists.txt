cmake_minimum_required(VERSION 3.5)

set(MAKER_TEST_EXECUTABLES)

include(CTest)
enable_testing()

macro(add_maker_test_executable TARGET)
  cmake_parse_arguments(AST "USE_SOKOL_APP" "" "LIBRARIES;SOURCES" ${ARGN})
  if(AST_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "Unknown argument(s): ${AST_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT AST_SOURCES)
    message(FATAL_ERROR "add_maker_test_executable needs at least one source")
  endif()

  list(APPEND MAKER_TEST_EXECUTABLES ${TARGET})

  # Append makerlib
  list(APPEND AST_LIBRARIES makerlib)
  
  add_executable(${TARGET} ${AST_SOURCES})
  target_link_libraries(${TARGET} PRIVATE ${AST_LIBRARIES})

  if ("sokol" IN_LIST AST_LIBRARIES)
    if(AST_USE_SOKOL_APP)
    target_compile_definitions(${TARGET} PUBLIC USE_SOKOL_APP)
    target_compile_definitions(${TARGET} PUBLIC USE_SOKOL_GLUE)
    endif()
    target_compile_definitions(${TARGET} PUBLIC USE_SOKOL_LOG)
    target_compile_definitions(${TARGET} PUBLIC USE_SOKOL_GFX)
  endif()
endmacro()

add_maker_test_executable(test_hello USE_SOKOL_APP LIBRARIES sokol microui SOURCES test_hello.c)
add_maker_test_executable(test_decode LIBRARIES sokol SOURCES test_decode.c)
add_maker_test_executable(test_decode_video USE_SOKOL_APP LIBRARIES sokol SOURCES test_decode_video.c)
add_maker_test_executable(test_thread SOURCES test_thread.c)

function(add_maker_test TEST TARGET)
  add_test(
    NAME ${TARGET}
    COMMAND ${TARGET}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

foreach(TARGET ${MAKER_TEST_EXECUTABLES})
  add_maker_test(${TARGET} ${TARGET})
endforeach()

# #=== LIBRARY: test
# add_library(test INTERFACE)
# target_include_directories(test INTERFACE "${PROJECT_SOURCE_DIR}/src")

# #=== TEST: hello world
# add_executable(test_hello "hello.c")
# target_link_libraries(test_hello
#   test
#   sokol
#   microui
#   ffmpeg
# )
# target_compile_definitions(test_hello PUBLIC 
#   USE_SOKOL_LOG 
#   USE_SOKOL_GFX 
#   USE_SOKOL_APP 
#   USE_SOKOL_GLUE
# )
# add_test(NAME hello COMMAND test_hello)

# #=== TEST: decode
# add_executable(test_decode 
#   "decode.c" "${MAKERLIB_SOURCES}" "maker_play_ext.h"
# )
# target_link_libraries(test_decode test sokol microui ffmpeg)
# target_compile_definitions(test_decode PUBLIC USE_SOKOL_LOG)
# add_test(
#   NAME test_decode
#   COMMAND decode
#   "${CMAKE_SOURCE_DIR}/tests/video.mp4"
#   "${CMAKE_SOURCE_DIR}/tmp/output.pgm"
#   "${CMAKE_SOURCE_DIR}/tmp/output.ppm"
# )

# #=== TEST: decode_video
# add_executable(test_decode_video 
#   "decode_video.c" "${MAKERLIB_SOURCES}" "maker_play_ext.h"
# )
# target_link_libraries(test_decode_video test sokol microui ffmpeg)
# target_compile_definitions(test_decode_video PUBLIC
#   USE_SOKOL_LOG
#   USE_SOKOL_GFX
#   USE_SOKOL_APP
#   USE_SOKOL_GLUE
# )
# add_test(
#   NAME test_decode_video 
#   COMMAND decode_video 
#   "${CMAKE_SOURCE_DIR}/tests/video.mp4"
# )

# #=== TEST: thread
# add_executable(test_thread 
#   "thread.c" "${MAKERLIB_SOURCES}" "maker_play_ext.h"
# )
# target_link_libraries(test_thread test ffmpeg)
# add_test(NAME thread COMMAND test_thread)

