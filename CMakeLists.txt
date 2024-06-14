cmake_minimum_required(VERSION 3.5)
project(pacman)
set(CMAKE_C_STANDARD 11)

# Linux -pthread shenanigans
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

# Manually specify Python include directories and libraries if needed
include_directories("/Library/Frameworks/Python.framework/Versions/3.9/include/python3.9")
link_directories("/Library/Frameworks/Python.framework/Versions/3.9/lib")


# Specify the exact path to the Python 3.9 interpreter
set(Python3_EXECUTABLE
        "/Library/Frameworks/Python.framework/Versions/3.9/bin/python3.9"
        # "/usr/local/Cellar/python@3.9/3.9.19/Frameworks/Python.framework/Versions/3.9/bin/python3.9"
)

# Find Python 3.9 library and include directories
find_package(
        Python3 3.9
        COMPONENTS Interpreter Development
        REQUIRED)

# === LIBRARY: sokol add headers to the the file list because they are useful to
# have in IDEs
set(SOKOL_HEADERS sokol/sokol_gfx.h sokol/sokol_app.h sokol/sokol_audio.h
        sokol/sokol_glue.h sokol/getdirection.h)
add_library(sokol STATIC sokol/sokol.c ${SOKOL_HEADERS})
add_library(getdirection STATIC sokol/getdirection.c sokol/getdirection.h)
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  # compile sokol.c as Objective-C
  target_compile_options(sokol PRIVATE -x objective-c)
  target_link_libraries(
          sokol
          "-framework QuartzCore"
          "-framework Cocoa"
          "-framework MetalKit"
          "-framework Metal"
          "-framework OpenGL"
          "-framework AudioToolbox")
else()
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(
            sokol
            INTERFACE X11
            Xi
            Xcursor
            GL
            asound
            dl
            m)
    target_link_libraries(sokol PUBLIC Threads::Threads)
  endif()
endif()
target_include_directories(sokol INTERFACE sokol)

# === EXECUTABLE: pacman
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  add_executable(pacman WIN32 pacman.c)
else()
  add_executable(pacman pacman.c)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} Python3::Python sokol getdirection)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS} sokol)

if(CMAKE_SYSTEM_NAME STREQUAL Emscripten)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  target_link_options(pacman PUBLIC --shell-file ../sokol/shell.html)
  target_link_options(
          pacman
          PUBLIC
          -sUSE_WEBGL2=1
          -sNO_FILESYSTEM=1
          -sASSERTIONS=0
          -sMALLOC=emmalloc
          --closure=1)
endif()
if(MSVC)
  target_compile_options(pacman PUBLIC /W3)
else()
  target_compile_options(pacman PUBLIC -Wall -Wextra -Wsign-compare)
endif()

# explicitly strip dead code
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL
        Emscripten)
  target_link_options(pacman PRIVATE LINKER:-dead_strip)
endif()
