cmake_minimum_required(VERSION 3.5)

project(
  maker-editor VERSION 1.0
  DESCRIPTION "Movie Maker like video editor"
  LANGUAGES C
)

# Add ./cmake directory into module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#=== BUILT-IN_MODULES: ExternalProject
include(ExternalProject)

#=== USER_MODULES: microui sokol ffmpeg
include(microui)
include(sokol)
include(ffmpeg)
include(requirements)
include(makerlib)

#=== EXECUTABLE: maker
add_executable(maker src/main.c)
target_link_libraries(maker makerlib)

#=== MISC: hack
# this hack removes the xxx-CMakeForceLinker.cxx dummy file
set_target_properties(maker PROPERTIES LINKER_LANGUAGE C)

#=== ADD_SUBDIRECTORIES: tests
add_subdirectory(tests)
