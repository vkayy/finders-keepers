cmake_minimum_required(VERSION 3.10)

# Set the project name and version""
project(finders_keepers VERSION 0.1)

# Specify the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Specify the exact path to the Python 3.9 interpreter
set(Python3_EXECUTABLE
    # "/Library/Frameworks/Python.framework/Versions/3.9/bin/python3.9"
    # "/usr/local/Cellar/python@3.9/3.9.19/Frameworks/Python.framework/Versions/3.9/bin/python3.9"
    "usr/bin/python3")

# Find Python libraries (assuming you have Python installed)
find_package(
  Python3
  COMPONENTS Interpreter Development
  REQUIRED)

# Add the executable

set(SOURCES
    finders_keepers.c
    getspeech.c
    speechrecog.py
    matrix/lib/led-matrix.cc
    matrix/lib/led-matrix-c.cc
    matrix/lib/graphics.cc
    # matrix/include/canvas.h matrix/include/content-streamer.h
    # matrix/include/graphics.h matrix/include/led-matrix-c.h
    # matrix/include/led-matrix.h matrix/include/thread.h
    # matrix/include/threaded-canvas-manipulator.h
)

# Add the executable

# set(SOURCES finders_keepers.c getspeech.c speechrecog.py
# matrix/lib/led-matrix.cc matrix/lib/graphics.cc matrix/include/canvas.h
# matrix/include/content-streamer.h matrix/include/graphics.h
# matrix/include/led-matrix-c.h matrix/include/led-matrix.h
# matrix/include/thread.h matrix/include/threaded-canvas-manipulator.h)
#
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE matrix/include)
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

# Link the Python libraries to your project
target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Python)
