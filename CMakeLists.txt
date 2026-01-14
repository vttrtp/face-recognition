cmake_minimum_required(VERSION 3.16)
project(face-recognition VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CopyHelpers)
include(IdlTarget)

# Install directories
include(GNUInstallDirs)

# RPATH settings for installed binaries to find libraries
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Options
option(BUILD_TESTS "Build unit tests" ON)

# Set visibility for shared libraries (not applicable for WASM)
if(NOT EMSCRIPTEN)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

# Find required packages
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs objdetect)
find_package(jsoncpp CONFIG REQUIRED)

# Face detector library (generates all bindings via idl_target)
add_subdirectory(src/facedetector)

# WASM bindings (skipped automatically if not Emscripten)
add_subdirectory(src/wasm)

# App uses dlopen which is not available in WASM
if(NOT EMSCRIPTEN)
    add_subdirectory(src/app)
endif()

# Java JNI bindings (skipped automatically if JNI not found or Emscripten)
if(NOT EMSCRIPTEN)
    include(JavaJNI)
endif()

# Tests
if(BUILD_TESTS AND NOT EMSCRIPTEN)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()
