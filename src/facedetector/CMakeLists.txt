set(LIB_NAME face_detector)

# Core library sources
set(LIB_SOURCES
    face_detector.cpp
    ${CMAKE_SOURCE_DIR}/src/generated/face_detector_c_api.cpp
)

# Use static library for WASM, shared for native
if(EMSCRIPTEN)
    add_library(${LIB_NAME} STATIC ${LIB_SOURCES})
else()
    add_library(${LIB_NAME} SHARED ${LIB_SOURCES})
    target_compile_definitions(${LIB_NAME} PRIVATE FACE_DETECTOR_EXPORTS)
    
    # Also build static library for JNI linking (needs PIC for shared lib)
    add_library(${LIB_NAME}_static STATIC ${LIB_SOURCES})
    set_target_properties(${LIB_NAME}_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(${LIB_NAME}_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/generated>
    )
    target_link_libraries(${LIB_NAME}_static PUBLIC ${OpenCV_LIBS})
    add_dependencies(${LIB_NAME}_static generate_bindings)
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/generated>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        ${OpenCV_LIBS}
)

# Ensure bindings are generated before building
add_dependencies(${LIB_NAME} generate_bindings)

# Download OpenCV cascade classifier data
download_cascade_file()

# Install targets (not for WASM)
if(NOT EMSCRIPTEN)
    install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES
        face_detector.hpp
        ${CMAKE_SOURCE_DIR}/src/generated/face_detector_c_api.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/face_detector
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/data/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/face_detector
        FILES_MATCHING PATTERN "*.xml"
    )
endif()
