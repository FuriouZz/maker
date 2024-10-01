set(
  MAKERLIB_SOURCES
  "${PROJECT_SOURCE_DIR}/src/maker_decoder.c"
  "${PROJECT_SOURCE_DIR}/src/maker_util.c"
)

add_library(makerlib STATIC "${MAKERLIB_SOURCES}")
target_link_libraries(makerlib requirements)
target_link_libraries(makerlib microui)
target_link_libraries(makerlib sokol)
target_link_libraries(makerlib ${FFMPEG_LIBRARIES})
target_link_directories(makerlib PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_include_directories(makerlib PUBLIC ${FFMPEG_INCLUDE_DIRS})
