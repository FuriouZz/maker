set(MICROUI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/vendors/microui/src")
set(MICROUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/vendors/microui/src")

add_library(microui SHARED "${MICROUI_SOURCE_DIR}/microui.c")
target_include_directories(microui INTERFACE "${MICROUI_INCLUDE_DIRS}")
