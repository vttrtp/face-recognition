# Face Detector Library - using idl_target for automatic binding generation

idl_target(
    NAME face_detector
    TYPE LIBRARY
    SOURCES face_detector.cpp
    IDL_FILES
        ${CMAKE_SOURCE_DIR}/src/common/common_types.idl
        face_detector.idl
    NAMESPACE face_detector
    IMPL_HEADER face_detector.hpp
    JAVA
    JAVA_PACKAGE face.detector
    JAVA_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/src/java/src/main/java
    LINK_LIBRARIES ${OpenCV_LIBS}
)

# Download OpenCV cascade classifier data
download_cascade_file()

# Install targets (not for WASM)
if(NOT EMSCRIPTEN)
    install(TARGETS face_detector
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES
        face_detector.hpp
        ${face_detector_GENERATED_DIR}/face_detector_c_api.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/face_detector
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/data/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/face_detector
        FILES_MATCHING PATTERN "*.xml"
    )
endif()
