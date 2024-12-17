set(MAKERLIB_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(MAKERLIB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

add_library(makerlib SHARED
  "${MAKERLIB_SOURCE_DIR}/maker_mutex.c"
  "${MAKERLIB_SOURCE_DIR}/maker_play.c"
  "${MAKERLIB_SOURCE_DIR}/maker_thread.c"
  "${MAKERLIB_SOURCE_DIR}/maker_util.c"
  "${MAKERLIB_SOURCE_DIR}/maker_media.c"
  "${MAKERLIB_SOURCE_DIR}/maker_player.c"
  "${MAKERLIB_SOURCE_DIR}/maker_player_ffplay.c"
  "${MAKERLIB_SOURCE_DIR}/maker_pool.c"
)
target_include_directories(makerlib PUBLIC "${MAKERLIB_INCLUDE_DIRS}")
target_compile_definitions(makerlib PUBLIC MAKER_DEBUG)
target_link_libraries(makerlib requirements ffmpeg)

