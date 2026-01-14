# IdlTarget.cmake - Unified CMake function for IDL-based targets
#
# Supports all target types with minimal boilerplate:
#
# Usage:
#   idl_target(
#       NAME my_target
#       TYPE LIBRARY|EXECUTABLE|WASM|JNI
#       [SOURCES src1.cpp src2.cpp]
#       [IDL_FILES api.idl]
#       [DEPENDENCIES lib1 lib2]           # Link directly
#       [DYNAMIC_DEPENDENCIES idl_target]  # Load dynamically at runtime
#       [NAMESPACE my_namespace]
#       [IMPL_HEADER impl.hpp]
#       [JAVA]                             # Enable Java/JNI bindings generation for LIBRARY type
#       [JAVA_PACKAGE com.example]
#       [JAVA_OUTPUT_DIR path/to/java]
#       [LINK_LIBRARIES lib1 lib2]
#       [LINK_OPTIONS -opt1 -opt2]
#       [INCLUDE_DIRS dir1 dir2]
#       [STATIC]
#       [SHARED]
#       [WASM_EXPORT_NAME name]
#       [WASM_THREADS]
#   )
#
# TYPE descriptions:
#   LIBRARY    - Creates shared/static library with C API from IDL
#   EXECUTABLE - Creates executable (if IDL dependency, uses client wrapper for dynamic loading)
#   WASM       - Creates WASM executable from dependency library
#   JNI        - Creates JNI shared library + Java classes from dependency library

include_guard(GLOBAL)

# Internal: Get the generated output directory for a target
function(_idl_get_generated_dir TARGET_NAME OUTPUT_VAR)
    set(${OUTPUT_VAR} "${CMAKE_BINARY_DIR}/generated/${TARGET_NAME}" PARENT_SCOPE)
endfunction()

# Internal: Get the list of generated files for IDL files
# IDL_FILES - List of IDL file paths
# NAMESPACE - Namespace for combined files (WASM, JNI)
# OUTPUT_DIR - Output directory
# GENERATE_JAVA - Whether to generate Java/JNI bindings
# OUTPUT_VAR - Variable to store the result
function(_idl_get_generated_files IDL_FILES NAMESPACE OUTPUT_DIR GENERATE_JAVA OUTPUT_VAR)
    set(FILES "")

    # Per-file outputs (C API and client for each IDL file)
    foreach(IDL_FILE ${IDL_FILES})
        get_filename_component(IDL_NAME "${IDL_FILE}" NAME_WE)
        string(REPLACE "-" "_" FILE_NS "${IDL_NAME}")

        list(APPEND FILES
            "${OUTPUT_DIR}/${FILE_NS}_c_api.h"
            "${OUTPUT_DIR}/${FILE_NS}_c_api.cpp"
            "${OUTPUT_DIR}/${FILE_NS}_client.hpp"
            "${OUTPUT_DIR}/${FILE_NS}_client.cpp"
        )
    endforeach()

    # Combined outputs (WASM bindings - always generated)
    list(APPEND FILES "${OUTPUT_DIR}/${NAMESPACE}_wasm_bindings.cpp")

    # Combined outputs (JNI - only if Java is enabled)
    if(GENERATE_JAVA)
        list(APPEND FILES
            "${OUTPUT_DIR}/${NAMESPACE}_jni.h"
            "${OUTPUT_DIR}/${NAMESPACE}_jni.cpp"
        )
    endif()

    set(${OUTPUT_VAR} ${FILES} PARENT_SCOPE)
endfunction()

# Main function: Define a target with IDL generation support
function(idl_target)
    set(OPTIONS JAVA STATIC SHARED WASM_THREADS)
    set(ONE_VALUE_ARGS NAME TYPE NAMESPACE IMPL_HEADER JAVA_PACKAGE JAVA_OUTPUT_DIR WASM_EXPORT_NAME)
    set(MULTI_VALUE_ARGS SOURCES IDL_FILES DEPENDENCIES DYNAMIC_DEPENDENCIES LINK_LIBRARIES LINK_OPTIONS INCLUDE_DIRS)
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_NAME)
        message(FATAL_ERROR "idl_target: NAME is required")
    endif()

    if(NOT ARG_TYPE)
        message(FATAL_ERROR "idl_target: TYPE is required")
    endif()

    # Validate TYPE
    set(VALID_TYPES LIBRARY EXECUTABLE WASM JNI)
    if(NOT ARG_TYPE IN_LIST VALID_TYPES)
        message(FATAL_ERROR "idl_target: TYPE must be one of: ${VALID_TYPES}, got '${ARG_TYPE}'")
    endif()

    # Platform checks
    if(ARG_TYPE STREQUAL "WASM" AND NOT EMSCRIPTEN)
        message(STATUS "Skipping WASM target ${ARG_NAME} (not building with Emscripten)")
        return()
    endif()

    if(ARG_TYPE STREQUAL "JNI" AND EMSCRIPTEN)
        message(STATUS "Skipping JNI target ${ARG_NAME} (building with Emscripten)")
        return()
    endif()

    if(ARG_TYPE STREQUAL "JNI")
        find_package(JNI QUIET)
        if(NOT JNI_FOUND)
            message(STATUS "Skipping JNI target ${ARG_NAME} (JNI not found)")
            return()
        endif()
    endif()

    # Set generated directory
    _idl_get_generated_dir(${ARG_NAME} GENERATED_DIR)
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    # Collect all sources
    set(ALL_SOURCES ${ARG_SOURCES})
    set(GENERATED_FILES "")

    # Get dependency information (for IDL imports)
    set(DEP_IDL_FILES "")
    set(DEP_INCLUDE_DIRS "")
    set(DEP_NAMESPACES "")
    set(DEP_GENERATED_DIRS "")

    # Collect IDL info from regular DEPENDENCIES (for import resolution)
    if(ARG_DEPENDENCIES)
        foreach(DEP ${ARG_DEPENDENCIES})
            if(TARGET ${DEP})
                get_target_property(DEP_IDLS ${DEP} IDL_FILES)
                if(DEP_IDLS)
                    list(APPEND DEP_IDL_FILES ${DEP_IDLS})
                endif()

                get_target_property(DEP_GEN_DIR ${DEP} IDL_GENERATED_DIR)
                if(DEP_GEN_DIR)
                    list(APPEND DEP_INCLUDE_DIRS ${DEP_GEN_DIR})
                    list(APPEND DEP_GENERATED_DIRS ${DEP_GEN_DIR})
                endif()

                get_target_property(DEP_NS ${DEP} IDL_NAMESPACE)
                if(DEP_NS)
                    list(APPEND DEP_NAMESPACES ${DEP_NS})
                endif()
            endif()
        endforeach()
    endif()

    # Collect IDL info from DYNAMIC_DEPENDENCIES (for client wrapper generation)
    set(DYN_DEP_IDL_FILES "")
    set(DYN_DEP_INCLUDE_DIRS "")
    set(DYN_DEP_NAMESPACES "")
    set(DYN_DEP_GENERATED_DIRS "")

    if(ARG_DYNAMIC_DEPENDENCIES)
        foreach(DEP ${ARG_DYNAMIC_DEPENDENCIES})
            if(TARGET ${DEP})
                get_target_property(DEP_IDLS ${DEP} IDL_FILES)
                if(DEP_IDLS)
                    list(APPEND DYN_DEP_IDL_FILES ${DEP_IDLS})
                    list(APPEND DEP_IDL_FILES ${DEP_IDLS})  # Also add to regular for imports
                endif()

                get_target_property(DEP_GEN_DIR ${DEP} IDL_GENERATED_DIR)
                if(DEP_GEN_DIR)
                    list(APPEND DYN_DEP_INCLUDE_DIRS ${DEP_GEN_DIR})
                    list(APPEND DYN_DEP_GENERATED_DIRS ${DEP_GEN_DIR})
                    list(APPEND DEP_INCLUDE_DIRS ${DEP_GEN_DIR})  # Also add to regular
                endif()

                get_target_property(DEP_NS ${DEP} IDL_NAMESPACE)
                if(DEP_NS)
                    list(APPEND DYN_DEP_NAMESPACES ${DEP_NS})
                    list(APPEND DEP_NAMESPACES ${DEP_NS})  # Also add to regular
                endif()
            endif()
        endforeach()
    endif()

    # Determine namespace
    if(ARG_NAMESPACE)
        set(NS "${ARG_NAMESPACE}")
    elseif(ARG_IDL_FILES)
        list(GET ARG_IDL_FILES 0 FIRST_IDL)
        get_filename_component(IDL_NAME "${FIRST_IDL}" NAME_WE)
        string(REPLACE "-" "_" NS "${IDL_NAME}")
    elseif(DEP_NAMESPACES)
        list(GET DEP_NAMESPACES 0 NS)
    else()
        string(REPLACE "-" "_" NS "${ARG_NAME}")
    endif()

    # Process IDL files (for LIBRARY type)
    if(ARG_IDL_FILES)
        set(IDL_GENERATOR "${CMAKE_SOURCE_DIR}/idlgen/bin/generate_bindings.py")

        # Convert all IDL files to absolute paths
        set(ABSOLUTE_IDL_FILES "")
        foreach(IDL_FILE ${ARG_IDL_FILES})
            if(NOT IS_ABSOLUTE "${IDL_FILE}")
                set(IDL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${IDL_FILE}")
            endif()
            list(APPEND ABSOLUTE_IDL_FILES ${IDL_FILE})
        endforeach()

        # Determine implementation header from first IDL file
        list(GET ABSOLUTE_IDL_FILES 0 FIRST_IDL_FILE)
        get_filename_component(FIRST_IDL_NAME "${FIRST_IDL_FILE}" NAME_WE)
        if(ARG_IMPL_HEADER)
            set(IMPL_HEADER "${ARG_IMPL_HEADER}")
        else()
            set(IMPL_HEADER "${FIRST_IDL_NAME}.hpp")
        endif()

        # Get generated files list
        _idl_get_generated_files("${ABSOLUTE_IDL_FILES}" "${NS}" "${GENERATED_DIR}" "${ARG_JAVA}" GENERATED_FILES)

        # Build generator command with all IDL files
        set(GEN_CMD
            ${Python3_EXECUTABLE} ${IDL_GENERATOR}
            ${ABSOLUTE_IDL_FILES}
            --output-dir ${GENERATED_DIR}
            --namespace ${NS}
            --impl-header ${IMPL_HEADER}
        )

        # Add Java options
        if(ARG_JAVA)
            list(APPEND GEN_CMD --java)
            if(ARG_JAVA_PACKAGE)
                list(APPEND GEN_CMD --java-package ${ARG_JAVA_PACKAGE})
            endif()
            if(ARG_JAVA_OUTPUT_DIR)
                list(APPEND GEN_CMD --java-output-dir ${ARG_JAVA_OUTPUT_DIR})
            endif()
        endif()

        # Get generator source files for dependency tracking
        file(GLOB IDL_GENERATOR_SOURCES
            "${CMAKE_SOURCE_DIR}/idlgen/idlgen/*.py"
            "${CMAKE_SOURCE_DIR}/idlgen/bin/*.py"
        )

        # Create custom command for generation
        add_custom_command(
            OUTPUT ${GENERATED_FILES}
            COMMAND ${GEN_CMD}
            DEPENDS ${ABSOLUTE_IDL_FILES} ${IDL_GENERATOR} ${IDL_GENERATOR_SOURCES}
            COMMENT "Generating bindings from IDL for ${ARG_NAME}"
            VERBATIM
        )

        # Create umbrella generation target
        add_custom_target(${ARG_NAME}_generate ALL DEPENDS ${GENERATED_FILES})
    endif()

    # Handle different target types
    if(ARG_TYPE STREQUAL "LIBRARY")
        _idl_create_library_target()
    elseif(ARG_TYPE STREQUAL "EXECUTABLE")
        _idl_create_executable_target()
    elseif(ARG_TYPE STREQUAL "WASM")
        _idl_create_wasm_target()
    elseif(ARG_TYPE STREQUAL "JNI")
        _idl_create_jni_target()
    endif()

    # Store IDL metadata on target for dependencies to use
    if(TARGET ${ARG_NAME})
        set_target_properties(${ARG_NAME} PROPERTIES
            IDL_FILES "${ARG_IDL_FILES}"
            IDL_GENERATED_DIR "${GENERATED_DIR}"
            IDL_NAMESPACE "${NS}"
            IDL_DEPENDENCIES "${ARG_DEPENDENCIES}"
        )

        # Export variables for convenience
        set(${ARG_NAME}_GENERATED_DIR ${GENERATED_DIR} PARENT_SCOPE)
        set(${ARG_NAME}_GENERATED_FILES ${GENERATED_FILES} PARENT_SCOPE)
        set(${ARG_NAME}_NAMESPACE ${NS} PARENT_SCOPE)
    endif()

endfunction()

# Internal: Create LIBRARY target
macro(_idl_create_library_target)
    # Determine library type
    if(ARG_STATIC)
        set(LIB_TYPE STATIC)
    elseif(ARG_SHARED)
        set(LIB_TYPE SHARED)
    elseif(EMSCRIPTEN)
        set(LIB_TYPE STATIC)
    else()
        set(LIB_TYPE SHARED)
    endif()

    # Add C API sources to the build
    foreach(GEN_FILE ${GENERATED_FILES})
        if(GEN_FILE MATCHES "_c_api\\.cpp$")
            list(APPEND ALL_SOURCES ${GEN_FILE})
        endif()
    endforeach()

    # Create the library target
    add_library(${ARG_NAME} ${LIB_TYPE} ${ALL_SOURCES})

    # Add export macro for shared libraries
    if(LIB_TYPE STREQUAL "SHARED")
        string(TOUPPER "${NS}" EXPORT_MACRO_PREFIX)
        target_compile_definitions(${ARG_NAME} PRIVATE ${EXPORT_MACRO_PREFIX}_EXPORTS)
    endif()

    # Add include directories
    target_include_directories(${ARG_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${GENERATED_DIR}>
    )

    # Add dependency include directories
    foreach(DEP_INC ${DEP_INCLUDE_DIRS})
        target_include_directories(${ARG_NAME} PUBLIC $<BUILD_INTERFACE:${DEP_INC}>)
    endforeach()

    # Add user-specified include directories
    if(ARG_INCLUDE_DIRS)
        foreach(INC_DIR ${ARG_INCLUDE_DIRS})
            target_include_directories(${ARG_NAME} PUBLIC $<BUILD_INTERFACE:${INC_DIR}>)
        endforeach()
    endif()

    # Add link libraries
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${ARG_NAME} PUBLIC ${ARG_LINK_LIBRARIES})
    endif()

    # Add dependencies
    if(ARG_DEPENDENCIES)
        target_link_libraries(${ARG_NAME} PUBLIC ${ARG_DEPENDENCIES})
        foreach(DEP ${ARG_DEPENDENCIES})
            if(TARGET ${DEP}_generate)
                add_dependencies(${ARG_NAME} ${DEP}_generate)
            endif()
        endforeach()
    endif()

    # Ensure generation happens before compilation
    if(TARGET ${ARG_NAME}_generate)
        add_dependencies(${ARG_NAME} ${ARG_NAME}_generate)
    endif()

    # Create additional static library for JNI if building shared native library
    if(LIB_TYPE STREQUAL "SHARED" AND NOT EMSCRIPTEN)
        add_library(${ARG_NAME}_static STATIC ${ALL_SOURCES})
        set_target_properties(${ARG_NAME}_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(${ARG_NAME}_static
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<BUILD_INTERFACE:${GENERATED_DIR}>
        )
        foreach(DEP_INC ${DEP_INCLUDE_DIRS})
            target_include_directories(${ARG_NAME}_static PUBLIC $<BUILD_INTERFACE:${DEP_INC}>)
        endforeach()
        if(ARG_INCLUDE_DIRS)
            foreach(INC_DIR ${ARG_INCLUDE_DIRS})
                target_include_directories(${ARG_NAME}_static PUBLIC $<BUILD_INTERFACE:${INC_DIR}>)
            endforeach()
        endif()
        if(ARG_LINK_LIBRARIES)
            target_link_libraries(${ARG_NAME}_static PUBLIC ${ARG_LINK_LIBRARIES})
        endif()
        if(ARG_DEPENDENCIES)
            target_link_libraries(${ARG_NAME}_static PUBLIC ${ARG_DEPENDENCIES})
        endif()
        if(TARGET ${ARG_NAME}_generate)
            add_dependencies(${ARG_NAME}_static ${ARG_NAME}_generate)
        endif()
    endif()
endmacro()

# Internal: Create EXECUTABLE target
# DEPENDENCIES are linked directly
# DYNAMIC_DEPENDENCIES use client wrapper for runtime loading
macro(_idl_create_executable_target)
    # Add client wrapper sources from DYNAMIC_DEPENDENCIES
    # We need to add one client.cpp per IDL file (per-file generation)
    set(CLIENT_SOURCES "")
    if(ARG_DYNAMIC_DEPENDENCIES)
        foreach(DEP ${ARG_DYNAMIC_DEPENDENCIES})
            get_target_property(DEP_GEN_DIR ${DEP} IDL_GENERATED_DIR)
            get_target_property(DEP_IDLS ${DEP} IDL_FILES)
            if(DEP_GEN_DIR AND DEP_IDLS)
                foreach(IDL_FILE ${DEP_IDLS})
                    get_filename_component(IDL_NAME "${IDL_FILE}" NAME_WE)
                    string(REPLACE "-" "_" FILE_NS "${IDL_NAME}")
                    set(CLIENT_SRC "${DEP_GEN_DIR}/${FILE_NS}_client.cpp")
                    list(APPEND CLIENT_SOURCES "${CLIENT_SRC}")
                    list(APPEND ALL_SOURCES "${CLIENT_SRC}")
                endforeach()
            endif()
        endforeach()
    endif()

    # Mark client sources as generated (they come from dependency targets)
    set_source_files_properties(${CLIENT_SOURCES} PROPERTIES GENERATED TRUE)

    add_executable(${ARG_NAME} ${ALL_SOURCES})

    # Add include directories
    target_include_directories(${ARG_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${GENERATED_DIR}
    )

    # Add dependency include directories (both regular and dynamic)
    foreach(DEP_INC ${DEP_INCLUDE_DIRS})
        target_include_directories(${ARG_NAME} PRIVATE ${DEP_INC})
    endforeach()

    # Add user-specified include directories
    if(ARG_INCLUDE_DIRS)
        target_include_directories(${ARG_NAME} PRIVATE ${ARG_INCLUDE_DIRS})
    endif()

    # Add link libraries
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # Handle regular DEPENDENCIES - link directly
    if(ARG_DEPENDENCIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_DEPENDENCIES})
        foreach(DEP ${ARG_DEPENDENCIES})
            if(TARGET ${DEP}_generate)
                add_dependencies(${ARG_NAME} ${DEP}_generate)
            endif()
        endforeach()
    endif()

    # Handle DYNAMIC_DEPENDENCIES - use dynamic loading
    if(ARG_DYNAMIC_DEPENDENCIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${CMAKE_DL_LIBS})
        foreach(DEP ${ARG_DYNAMIC_DEPENDENCIES})
            if(TARGET ${DEP}_generate)
                add_dependencies(${ARG_NAME} ${DEP}_generate)
            endif()
            # Add include path from dependency source
            get_target_property(DEP_SOURCE_DIR ${DEP} SOURCE_DIR)
            if(DEP_SOURCE_DIR)
                target_include_directories(${ARG_NAME} PRIVATE ${DEP_SOURCE_DIR})
            endif()
        endforeach()
    endif()

    # Ensure generation happens before compilation
    if(TARGET ${ARG_NAME}_generate)
        add_dependencies(${ARG_NAME} ${ARG_NAME}_generate)
    endif()
endmacro()

# Internal: Create WASM target
macro(_idl_create_wasm_target)
    # Get WASM binding sources from dependencies (one combined _wasm_bindings.cpp per dependency namespace)
    set(WASM_SOURCES "")
    if(ARG_DEPENDENCIES)
        foreach(DEP ${ARG_DEPENDENCIES})
            get_target_property(DEP_GEN_DIR ${DEP} IDL_GENERATED_DIR)
            get_target_property(DEP_NS ${DEP} IDL_NAMESPACE)
            if(DEP_GEN_DIR AND DEP_NS)
                set(WASM_SRC "${DEP_GEN_DIR}/${DEP_NS}_wasm_bindings.cpp")
                list(APPEND WASM_SOURCES "${WASM_SRC}")
                list(APPEND ALL_SOURCES "${WASM_SRC}")
            endif()
        endforeach()
    endif()

    # Mark WASM sources as generated (they come from dependency targets)
    set_source_files_properties(${WASM_SOURCES} PROPERTIES GENERATED TRUE)

    add_executable(${ARG_NAME} ${ALL_SOURCES})

    # Link to dependency libraries
    if(ARG_DEPENDENCIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_DEPENDENCIES})
        foreach(DEP ${ARG_DEPENDENCIES})
            if(TARGET ${DEP}_generate)
                add_dependencies(${ARG_NAME} ${DEP}_generate)
            endif()
        endforeach()
    endif()

    # Add link libraries
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # Determine export name
    if(ARG_WASM_EXPORT_NAME)
        set(WASM_EXPORT "${ARG_WASM_EXPORT_NAME}")
    elseif(DEP_NAMESPACES)
        list(GET DEP_NAMESPACES 0 WASM_EXPORT)
    else()
        set(WASM_EXPORT "${ARG_NAME}")
    endif()

    # Add Emscripten-specific link options
    target_link_options(${ARG_NAME} PRIVATE
        -lembind
        -sWASM=1
        -sMODULARIZE=1
        -sEXPORT_NAME='create${WASM_EXPORT}Module'
        -sALLOW_MEMORY_GROWTH=1
        -sMAXIMUM_MEMORY=512MB
        -sFORCE_FILESYSTEM=1
        -sFETCH=1
    )

    # Add pthread support if requested
    if(ARG_WASM_THREADS)
        target_link_options(${ARG_NAME} PRIVATE
            -pthread
            -sUSE_PTHREADS=1
            -sPTHREAD_POOL_SIZE=4
        )
        target_compile_options(${ARG_NAME} PRIVATE -pthread)
    endif()

    # Add user link options
    if(ARG_LINK_OPTIONS)
        target_link_options(${ARG_NAME} PRIVATE ${ARG_LINK_OPTIONS})
    endif()
endmacro()

# Internal: Create JNI target (JNI library + Java classes)
macro(_idl_create_jni_target)
    # Get JNI sources from dependencies (one combined _jni.cpp per dependency namespace)
    set(JNI_SOURCES "")
    if(ARG_DEPENDENCIES)
        foreach(DEP ${ARG_DEPENDENCIES})
            get_target_property(DEP_GEN_DIR ${DEP} IDL_GENERATED_DIR)
            get_target_property(DEP_NS ${DEP} IDL_NAMESPACE)
            if(DEP_GEN_DIR AND DEP_NS)
                set(JNI_SRC "${DEP_GEN_DIR}/${DEP_NS}_jni.cpp")
                list(APPEND JNI_SOURCES "${JNI_SRC}")
                list(APPEND ALL_SOURCES "${JNI_SRC}")
            endif()
        endforeach()
    endif()

    # Mark JNI sources as generated (they come from dependency targets)
    set_source_files_properties(${JNI_SOURCES} PROPERTIES GENERATED TRUE)

    add_library(${ARG_NAME} SHARED ${ALL_SOURCES})

    # Link to static version of dependency libraries (for PIC)
    if(ARG_DEPENDENCIES)
        foreach(DEP ${ARG_DEPENDENCIES})
            if(TARGET ${DEP}_static)
                target_link_libraries(${ARG_NAME} PRIVATE ${DEP}_static)
            elseif(TARGET ${DEP})
                target_link_libraries(${ARG_NAME} PRIVATE ${DEP})
            endif()
            if(TARGET ${DEP}_generate)
                add_dependencies(${ARG_NAME} ${DEP}_generate)
            endif()
            # Add include path from dependency
            get_target_property(DEP_SOURCE_DIR ${DEP} SOURCE_DIR)
            if(DEP_SOURCE_DIR)
                target_include_directories(${ARG_NAME} PRIVATE ${DEP_SOURCE_DIR})
            endif()
        endforeach()
    endif()

    # Add link libraries
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # Add JNI include directories
    target_include_directories(${ARG_NAME} PRIVATE ${JNI_INCLUDE_DIRS})

    # Add dependency include directories
    foreach(DEP_INC ${DEP_INCLUDE_DIRS})
        target_include_directories(${ARG_NAME} PRIVATE ${DEP_INC})
    endforeach()
endmacro()

# Helper: Copy library to target directory after build
function(idl_copy_library_to_target TARGET_NAME LIB_TARGET)
    if(NOT TARGET ${TARGET_NAME} OR NOT TARGET ${LIB_TARGET})
        return()
    endif()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIB_TARGET}>
            $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMENT "Copying ${LIB_TARGET} to ${TARGET_NAME} directory"
    )
endfunction()

# Helper: Copy data directory to target
function(idl_copy_data_to_target TARGET_NAME DATA_DIR)
    if(NOT TARGET ${TARGET_NAME})
        return()
    endif()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${DATA_DIR}
            $<TARGET_FILE_DIR:${TARGET_NAME}>/data
        COMMENT "Copying data to ${TARGET_NAME} directory"
    )
endfunction()
