cmake_minimum_required(VERSION 4.0)

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


add_maker_test_executable(test_media SOURCES test_media.c)
add_maker_test_executable(test_decode SOURCES test_decode.c)
# add_maker_test_executable(test_hello USE_SOKOL_APP LIBRARIES sokol microui SOURCES test_hello.c)
# add_maker_test_executable(test_decode LIBRARIES sokol SOURCES test_decode.c)
# add_maker_test_executable(test_decode2 LIBRARIES sokol SOURCES test_decode2.c)
# add_maker_test_executable(test_decode_video USE_SOKOL_APP LIBRARIES sokol SOURCES test_decode_video.c)
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
