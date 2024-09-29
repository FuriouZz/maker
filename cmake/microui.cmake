add_library(
  microui SHARED
  "${PROJECT_SOURCE_DIR}/vendors/microui/src/microui.h"
  "${PROJECT_SOURCE_DIR}/vendors/microui/src/microui.c"
)
target_include_directories(
  microui INTERFACE
  "${PROJECT_SOURCE_DIR}/vendors/microui/src"
)
