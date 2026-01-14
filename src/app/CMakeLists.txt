# Face Recognition App - using idl_target with dynamic loading

idl_target(
    NAME face_recognition_app
    TYPE EXECUTABLE
    SOURCES
        main.cpp
        image_processor.cpp
        file_utils.cpp
    DYNAMIC_DEPENDENCIES face_detector
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        JsonCpp::JsonCpp
)

# Copy data and library to output directory
idl_copy_data_to_target(face_recognition_app ${CMAKE_BINARY_DIR}/data)
idl_copy_library_to_target(face_recognition_app face_detector)

# Install target
install(TARGETS face_recognition_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
