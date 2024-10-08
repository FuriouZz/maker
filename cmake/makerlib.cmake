set(
  MAKERLIB_SOURCES
  "${PROJECT_SOURCE_DIR}/src/maker_mutex.c"
  "${PROJECT_SOURCE_DIR}/src/maker_play.c"
  "${PROJECT_SOURCE_DIR}/src/maker_thread.c"
  "${PROJECT_SOURCE_DIR}/src/maker_util.c"
)

add_library(makerlib STATIC "${MAKERLIB_SOURCES}")
target_compile_definitions(makerlib PUBLIC MAKER_DEBUG)

# LIBRARY: requirements
target_link_libraries(makerlib requirements)

# LIBRARY: microui
target_link_libraries(makerlib microui)

# LIBRARY: ffmpeg
target_link_libraries(makerlib ffmpeg)

# LIBRARY: sokol
target_link_libraries(makerlib sokol)
target_compile_definitions(makerlib PUBLIC USE_SOKOL_GFX)
target_compile_definitions(makerlib PUBLIC USE_SOKOL_APP)
target_compile_definitions(makerlib PUBLIC USE_SOKOL_LOG)
target_compile_definitions(makerlib PUBLIC USE_SOKOL_GLUE)
