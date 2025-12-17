cmake_minimum_required(VERSION 3.20)

# Set vcpkg toolchain before project() if VCPKG_ROOT is defined
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(idl_samples VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python for IDL generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Paths
set(IDL_GENERATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../idlgen")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(JAVA_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/java/src/main/java")

# Ensure directories exist
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(MAKE_DIRECTORY ${JAVA_GENERATED_DIR})

# IDL source file
set(IDL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/samples.idl")

# Generated files
set(GENERATED_C_API_H "${GENERATED_DIR}/samples_c_api.h")
set(GENERATED_C_API_CPP "${GENERATED_DIR}/samples_c_api.cpp")
set(GENERATED_CLIENT_HPP "${GENERATED_DIR}/samples_client.hpp")
set(GENERATED_CLIENT_CPP "${GENERATED_DIR}/samples_client.cpp")
set(GENERATED_JNI_H "${GENERATED_DIR}/samples_jni.h")
set(GENERATED_JNI_CPP "${GENERATED_DIR}/samples_jni.cpp")
set(GENERATED_WASM_CPP "${GENERATED_DIR}/samples_wasm_bindings.cpp")

set(GENERATED_FILES
    ${GENERATED_C_API_H}
    ${GENERATED_C_API_CPP}
    ${GENERATED_CLIENT_HPP}
    ${GENERATED_CLIENT_CPP}
    ${GENERATED_JNI_H}
    ${GENERATED_JNI_CPP}
    ${GENERATED_WASM_CPP}
)

# Generator source files (for dependency tracking)
file(GLOB IDL_GENERATOR_SOURCES "${IDL_GENERATOR_DIR}/*.py")

# Custom command to generate bindings from IDL
add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} 
        "${CMAKE_CURRENT_SOURCE_DIR}/../generate_bindings.py"
        "${IDL_FILE}"
        --output-dir "${GENERATED_DIR}"
        --namespace "samples"
        --impl-header "samples.hpp"
        --api-macro "SAMPLES_API"
        --java-package "idl.samples"
        --java-output "${JAVA_GENERATED_DIR}"
    DEPENDS ${IDL_FILE} ${IDL_GENERATOR_SOURCES}
    COMMENT "Generating bindings from samples.idl (C API, JNI, Java)"
    VERBATIM
)

# Custom target for generation
add_custom_target(generate_samples_bindings DEPENDS ${GENERATED_FILES})

# Non-WASM targets
if(NOT EMSCRIPTEN)

# Main library (shared)
add_library(idl_samples SHARED
    samples.cpp
    ${GENERATED_C_API_CPP}
)

target_include_directories(idl_samples
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GENERATED_DIR}
)

target_compile_definitions(idl_samples PRIVATE SAMPLES_EXPORTS)

add_dependencies(idl_samples generate_samples_bindings)

# Static library for testing
add_library(idl_samples_static STATIC
    samples.cpp
    ${GENERATED_C_API_CPP}
)

target_include_directories(idl_samples_static
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GENERATED_DIR}
)

target_compile_definitions(idl_samples_static PRIVATE SAMPLES_EXPORTS)

add_dependencies(idl_samples_static generate_samples_bindings)

# JNI library (optional)
find_package(JNI QUIET)
if(JNI_FOUND)
    add_library(samples_jni SHARED
        ${GENERATED_JNI_CPP}
    )
    
    target_include_directories(samples_jni PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GENERATED_DIR}
    )
    
    target_link_libraries(samples_jni PRIVATE idl_samples)
    
    add_dependencies(samples_jni generate_samples_bindings)
    
    message(STATUS "JNI found - building samples_jni library")
    message(STATUS "Java sources generated to: ${JAVA_GENERATED_DIR}")
endif()

# Tests
option(BUILD_SAMPLES_TESTS "Build IDL samples tests" ON)

if(BUILD_SAMPLES_TESTS AND NOT EMSCRIPTEN)
    find_package(GTest QUIET)
    
    if(GTest_FOUND)
        enable_testing()
        
        add_executable(samples_test
            tests/samples_test.cpp
        )
        
        target_link_libraries(samples_test PRIVATE
            idl_samples_static
            GTest::gtest
        )
        
        include(GoogleTest)
        gtest_discover_tests(samples_test)
        
        message(STATUS "GTest found - building samples tests")
    else()
        message(STATUS "GTest not found - samples tests will not be built")
    endif()
endif()

endif() # NOT EMSCRIPTEN

# WASM build (when using Emscripten)
if(EMSCRIPTEN)
    add_executable(samples_wasm
        samples.cpp
        ${GENERATED_WASM_CPP}
    )
    
    target_include_directories(samples_wasm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GENERATED_DIR}
    )
    
    add_dependencies(samples_wasm generate_samples_bindings)
    
    set_target_properties(samples_wasm PROPERTIES
        SUFFIX ".js"
    )
    
    target_link_options(samples_wasm PRIVATE
        -lembind
        -sMODULARIZE=1
        -sEXPORT_NAME='SamplesModule'
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=node,web,worker
    )
    
    # Copy test file to build directory
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/samples_test.js
        ${CMAKE_CURRENT_BINARY_DIR}/samples_test.js
        COPYONLY
    )
    
    message(STATUS "Building WASM module: samples_wasm.js")
endif()

# Installation (only for non-WASM builds)
if(NOT EMSCRIPTEN)
    install(TARGETS idl_samples
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    
    install(FILES
        samples.hpp
        ${GENERATED_C_API_H}
        ${GENERATED_CLIENT_HPP}
        DESTINATION include/idl_samples
    )
endif()
