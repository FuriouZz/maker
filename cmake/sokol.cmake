set(SOKOL_DIR "${PROJECT_SOURCE_DIR}/vendors/sokol")

option(USE_SOKOL_APP "Use sokol_app.h" ON)
option(USE_SOKOL_LOG "Use sokol_log.h" ON)
option(USE_SOKOL_GFX "Use sokol_log.h" ON)
option(USE_SOKOL_GLUE "Use sokol_glue.h" ON)

file(
  GENERATE 
  OUTPUT "${SOKOL_DIR}/sokol.c"
  CONTENT "\
  #define SOKOL_IMPL\n\
  #define SOKOL_DEBUG\n\
  #if defined(__MINGW32__)\n\
  #define SOKOL_GLCORE\n\
  #elif defined(_WIN32)\n\
  #define SOKOL_D3D11\n\
  #elif defined(__EMSCRIPTEN__)\n\
  #define SOKOL_GLES3\n\
  #elif defined(__APPLE__)\n\
  #define SOKOL_METAL\n\
  #endif\n\

  #if defined(USE_SOKOL_GFX)\n\
  #include \"sokol_gfx.h\"\n\
  #endif\n\

  #if defined(USE_SOKOL_APP)\n\
  #include \"sokol_app.h\"\n\
  #endif\n\

  #if defined(USE_SOKOL_LOG)\n\
  #include \"sokol_log.h\"\n\
  #endif\n\

  #if defined(USE_SOKOL_GLUE)\n\
  #include \"sokol_glue.h\"\n\
  #endif\n"
)

#=== LIBRARY: sokol
add_library(sokol INTERFACE)
target_sources(sokol PUBLIC "${SOKOL_DIR}/sokol.c")

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  # compile sokol.c as Objective-C
  target_compile_options(sokol INTERFACE -x objective-c)
  target_link_libraries(
    sokol INTERFACE
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework CoreAudio"
    "-framework AudioToolbox"
  )
  target_link_libraries(
    sokol INTERFACE
    "-framework MetalKit"
    "-framework Metal"
  )
else()
  if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL asound dl m)
    target_link_libraries(sokol INTERFACE Threads::Threads)
  endif()
endif()
target_include_directories(sokol INTERFACE "${SOKOL_DIR}")
