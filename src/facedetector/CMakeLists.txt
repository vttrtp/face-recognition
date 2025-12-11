set(LIB_NAME face_detector)

# Use static library for WASM, shared for native
if(EMSCRIPTEN)
    add_library(${LIB_NAME} STATIC
        face_detector.cpp
        face_detector_interface.cpp
    )
else()
    add_library(${LIB_NAME} SHARED
        face_detector.cpp
        face_detector_interface.cpp
    )
    target_compile_definitions(${LIB_NAME} PRIVATE FACE_DETECTOR_EXPORTS)
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        ${OpenCV_LIBS}
)

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
        face_detector_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/face_detector
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/data/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/face_detector
        FILES_MATCHING PATTERN "*.xml"
    )
endif()
