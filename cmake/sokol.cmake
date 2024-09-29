option(USE_SOKOL_APP "Use sokol_app.h" ON)
option(USE_SOKOL_LOG "Use sokol_log.h" ON)
option(USE_SOKOL_GFX "Use sokol_log.h" ON)
option(USE_SOKOL_GLUE "Use sokol_glue.h" ON)

set(SOKOL_C "#define SOKOL_IMPL\n#define SOKOL_DEBUG\n")

set(SOKOL_C "${SOKOL_C}\n#if defined(__MINGW32__)")
set(SOKOL_C "${SOKOL_C}\n#define SOKOL_GLCORE")
set(SOKOL_C "${SOKOL_C}\n#elif defined(_WIN32)")
set(SOKOL_C "${SOKOL_C}\n#define SOKOL_D3D11")
set(SOKOL_C "${SOKOL_C}\n#elif defined(__EMSCRIPTEN__)")
set(SOKOL_C "${SOKOL_C}\n#define SOKOL_GLES3")
set(SOKOL_C "${SOKOL_C}\n#elif defined(__APPLE__)")
set(SOKOL_C "${SOKOL_C}\n#define SOKOL_METAL")
set(SOKOL_C "${SOKOL_C}\n#else")
set(SOKOL_C "${SOKOL_C}\n#define SOKOL_GLCORE")
set(SOKOL_C "${SOKOL_C}\n#endif\n")

if(USE_SOKOL_GFX)
set(SOKOL_C "${SOKOL_C}\n#include \"sokol_gfx.h\"")
endif()

if(USE_SOKOL_APP)
set(SOKOL_C "${SOKOL_C}\n#include \"sokol_app.h\"")
endif()

if(USE_SOKOL_LOG)
set(SOKOL_C "${SOKOL_C}\n#include \"sokol_log.h\"")
endif()


if(USE_SOKOL_GLUE)
set(SOKOL_C "${SOKOL_C}\n#include \"sokol_glue.h\"")
endif()

set(SOKOL_DIR "${PROJECT_SOURCE_DIR}/vendors/sokol")

#=== LIBRARY: sokol
add_library(sokol STATIC "${SOKOL_DIR}/sokol.c")

file(
  GENERATE 
  OUTPUT "${SOKOL_DIR}/sokol.c"
  CONTENT "${SOKOL_C}"
)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  # compile sokol.c as Objective-C
  target_compile_options(sokol PRIVATE -x objective-c)
  target_link_libraries(
    sokol PUBLIC
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework CoreAudio"
    "-framework AudioToolbox"
  )
  target_link_libraries(
    sokol PUBLIC
    "-framework MetalKit"
    "-framework Metal"
  )
else()
  if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL asound dl m)
    target_link_libraries(sokol PUBLIC Threads::Threads)
  endif()
endif()
target_include_directories(sokol INTERFACE "${SOKOL_DIR}")

#=== LIBRARY: sokol-log
add_library(sokol_log STATIC "${SOKOL_DIR}/sokol_log.c")
file(
  GENERATE 
  OUTPUT "${SOKOL_DIR}/sokol_log.c"
  CONTENT "#define SOKOL_IMPL\n#include \"sokol_log.h\""
)
target_include_directories(sokol_log INTERFACE "${SOKOL_DIR}")

