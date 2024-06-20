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

include_directories(${PROJECT_SOURCE_DIR}/matrix/lib)
include_directories(${PROJECT_SOURCE_DIR}/matrix/include)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE RGB_SRC_FILES ${PROJECT_SOURCE_DIR}/matrix/lib/*.cc)
file(GLOB_RECURSE RGB_HEADER_FILES ${PROJECT_SOURCE_DIR}/matrix/include/*.h)

add_executable(${PROJECT_NAME} speechrecog.py ${RGB_SRC_FILES} /
                               ${RGB_HEADER_FILES} ${SRC_FILES} ${HEADER_FILES})

# Link the Python libraries to your project
target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Python)
