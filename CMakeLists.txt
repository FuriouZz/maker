cmake_minimum_required(VERSION 3.5)

project(
  maker-editor VERSION 1.0
  DESCRIPTION "Movie Maker like video editor"
  LANGUAGES C
)

set(MAKER_APP OFF CACHE BOOL "Generate for maker app")
set(MAKER_TESTS OFF CACHE BOOL "Genereate for maker tests")

# Add ./cmake directory into module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#=== BUILT-IN_MODULES: ExternalProject
include(ExternalProject)
include(CMakeParseArguments)

#=== USER_MODULES: microui sokol ffmpeg
include(requirements)
include(microui)
include(sokol)
include(ffmpeg)
include(makerlib)

#=== EXECUTABLE: maker
if(MAKER_APP)
add_executable(maker src/main.c)
target_link_libraries(maker makerlib)
endif()

#=== ADD_SUBDIRECTORIES: tests
if(MAKER_TESTS)
add_subdirectory(tests)
endif()
