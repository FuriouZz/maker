set(MAKERLIB_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(MAKERLIB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

add_library(makerlib SHARED
  "${MAKERLIB_SOURCE_DIR}/decoder.c"
  "${MAKERLIB_SOURCE_DIR}/format.c"
  "${MAKERLIB_SOURCE_DIR}/ffmpeg_decoder.c"
  "${MAKERLIB_SOURCE_DIR}/ffmpeg_frame_queue.c"
  "${MAKERLIB_SOURCE_DIR}/ffmpeg_packet_queue.c"
  "${MAKERLIB_SOURCE_DIR}/image_data.c"
  "${MAKERLIB_SOURCE_DIR}/log.c"
  "${MAKERLIB_SOURCE_DIR}/media.c"
  "${MAKERLIB_SOURCE_DIR}/mutex.c"
  "${MAKERLIB_SOURCE_DIR}/thread.c"
  "${MAKERLIB_SOURCE_DIR}/track.c"
  "${MAKERLIB_SOURCE_DIR}/utils_image_data.c"
  "${MAKERLIB_SOURCE_DIR}/utils_mem.c"
  "${MAKERLIB_SOURCE_DIR}/utils_str.c"
)
target_include_directories(makerlib PUBLIC "${MAKERLIB_INCLUDE_DIRS}")
target_compile_definitions(makerlib PUBLIC MAKER_DEBUG)
target_link_libraries(makerlib requirements ffmpeg)

