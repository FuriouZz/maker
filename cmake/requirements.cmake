add_library(requirements INTERFACE)
target_compile_features(requirements INTERFACE c_std_11)
target_compile_options(requirements INTERFACE -Wall -Wextra -Wpedantic)

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
  target_link_options(requirements INTERFACE LINKER:-dead_strip)
endif()
