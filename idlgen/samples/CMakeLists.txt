# IDL Generator Samples
# Included from root CMakeLists.txt - not a standalone project

# Paths
set(IDL_SAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(IDL_GENERATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../idlgen")
set(IDL_GENERATOR_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../bin/generate_bindings.py")

# Per-language generated output directories (gitignored)
set(IDL_CPP_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/generated")
set(IDL_JAVA_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/generated")
set(IDL_WASM_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/wasm/generated")

# Ensure directories exist
file(MAKE_DIRECTORY ${IDL_CPP_GENERATED_DIR})
file(MAKE_DIRECTORY ${IDL_JAVA_GENERATED_DIR})
file(MAKE_DIRECTORY ${IDL_WASM_GENERATED_DIR})

# IDL source file
set(IDL_SAMPLES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/samples.idl")

# Generated files
set(IDL_GENERATED_FILES
    ${IDL_CPP_GENERATED_DIR}/samples_c_api.h
    ${IDL_CPP_GENERATED_DIR}/samples_c_api.cpp
    ${IDL_CPP_GENERATED_DIR}/samples_client.hpp
    ${IDL_CPP_GENERATED_DIR}/samples_client.cpp
    ${IDL_CPP_GENERATED_DIR}/samples_jni.h
    ${IDL_CPP_GENERATED_DIR}/samples_jni.cpp
    ${IDL_CPP_GENERATED_DIR}/samples_wasm_bindings.cpp
)

# Generator source files (for dependency tracking)
file(GLOB IDL_GENERATOR_SOURCES "${IDL_GENERATOR_DIR}/*.py")

# Custom command to generate bindings from IDL
add_custom_command(
    OUTPUT ${IDL_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} 
        "${IDL_GENERATOR_BIN}"
        "${IDL_SAMPLES_FILE}"
        --output-dir "${IDL_CPP_GENERATED_DIR}"
        --namespace "samples"
        --impl-header "samples.hpp"
        --api-macro "SAMPLES_API"
        --java-package "idl.samples"
        --java-output "${IDL_JAVA_GENERATED_DIR}"
    DEPENDS ${IDL_SAMPLES_FILE} ${IDL_GENERATOR_SOURCES}
    COMMENT "Generating bindings from samples.idl (C++, Java, WASM)"
    VERBATIM
)

# Custom target for generation
add_custom_target(generate_samples_bindings DEPENDS ${IDL_GENERATED_FILES})

# ══════════════════════════════════════════════════════════════
# NON-WASM TARGETS
# ══════════════════════════════════════════════════════════════
if(NOT EMSCRIPTEN)

# Shared library
add_library(idl_samples SHARED
    ${IDL_SAMPLES_DIR}/samples.cpp
    ${IDL_CPP_GENERATED_DIR}/samples_c_api.cpp
)

target_include_directories(idl_samples
    PUBLIC
        ${IDL_SAMPLES_DIR}
        ${IDL_CPP_GENERATED_DIR}
)

target_compile_definitions(idl_samples PRIVATE SAMPLES_EXPORTS)
add_dependencies(idl_samples generate_samples_bindings)

# Static library for testing
add_library(idl_samples_static STATIC
    ${IDL_SAMPLES_DIR}/samples.cpp
    ${IDL_CPP_GENERATED_DIR}/samples_c_api.cpp
)

target_include_directories(idl_samples_static
    PUBLIC
        ${IDL_SAMPLES_DIR}
        ${IDL_CPP_GENERATED_DIR}
)

target_compile_definitions(idl_samples_static PRIVATE SAMPLES_EXPORTS)
add_dependencies(idl_samples_static generate_samples_bindings)

# JNI library (uses JNI already found by parent)
if(JNI_FOUND)
    add_library(samples_jni SHARED
        ${IDL_CPP_GENERATED_DIR}/samples_jni.cpp
    )
    
    target_include_directories(samples_jni PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${IDL_SAMPLES_DIR}
        ${IDL_CPP_GENERATED_DIR}
    )
    
    target_link_libraries(samples_jni PRIVATE idl_samples)
    add_dependencies(samples_jni generate_samples_bindings)
    
    message(STATUS "IDL Samples: JNI library enabled")
    message(STATUS "IDL Samples: Java sources -> ${IDL_JAVA_GENERATED_DIR}")
endif()

# Tests (uses GTest already found by parent)
if(BUILD_TESTS)
    add_executable(idl_samples_test
        ${IDL_SAMPLES_DIR}/tests/cpp/samples_test.cpp
    )
    
    target_link_libraries(idl_samples_test PRIVATE
        idl_samples_static
        GTest::gtest
    )
    
    include(GoogleTest)
    gtest_discover_tests(idl_samples_test)
    
    message(STATUS "IDL Samples: C++ tests enabled")
endif()

endif() # NOT EMSCRIPTEN

# ══════════════════════════════════════════════════════════════
# WASM TARGETS
# ══════════════════════════════════════════════════════════════
if(EMSCRIPTEN)
    add_executable(samples_wasm
        ${IDL_SAMPLES_DIR}/samples.cpp
        ${IDL_CPP_GENERATED_DIR}/samples_wasm_bindings.cpp
    )
    
    target_include_directories(samples_wasm PRIVATE
        ${IDL_SAMPLES_DIR}
        ${IDL_CPP_GENERATED_DIR}
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
        ${IDL_SAMPLES_DIR}/tests/wasm/samples_test.js
        ${CMAKE_CURRENT_BINARY_DIR}/samples_test.js
        COPYONLY
    )
    
    message(STATUS "IDL Samples: WASM module enabled")
endif()
