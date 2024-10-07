find_program(MAKE_EXE NAMES gmake nmake make)

set(FFMPEG_OPTIONS "")

#=== OPTIONS: INSTALLATION
list(APPEND FFMPEG_OPTIONS "--prefix=<INSTALL_DIR>") # set installation prefix

#=== OPTIONS: DISABLE
list(APPEND FFMPEG_OPTIONS "--disable-static") # do not build static libraries
list(APPEND FFMPEG_OPTIONS "--disable-doc")    # do not build documentation
list(APPEND FFMPEG_OPTIONS "--disable-doc")    # do not build command line programs

#=== OPTIONS: CONFIGURATION
list(APPEND FFMPEG_OPTIONS "--enable-shared") # build shared libraries

#=== OPTIONS: TOOLCHAINS
list(APPEND FFMPEG_OPTIONS "--enable-pic")           # build position independent code
list(APPEND FFMPEG_OPTIONS "--enable-cross-compile") # assume a cross-compiler is used


list(APPEND FFMPEG_OPTIONS "--enable-swscale")
# list(APPEND FFMPEG_OPTIONS "--enable-libx264")

ExternalProject_Add(
  ffmpeglibs
  URL "${PROJECT_SOURCE_DIR}/vendors/ffmpeg"
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./configure ${FFMPEG_OPTIONS}
  BUILD_COMMAND ${MAKE_EXE}
  INSTALL_COMMAND ${MAKE_EXE} install
  LOG_CONFIGURE 1
  LOG_BUILD 1
  LOG_INSTALL 1
)
ExternalProject_Get_property(ffmpeglibs INSTALL_DIR)

set(FFMPEG_FOUND true)
set(FFMPEG_LIBRARY_DIRS "${INSTALL_DIR}/lib")
set(FFMPEG_INCLUDE_DIRS "${INSTALL_DIR}/include")

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE avutil swresample avcodec avformat swscale avfilter avdevice)
target_link_directories(ffmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
target_include_directories(ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
