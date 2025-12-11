set(APP_NAME face_recognition_app)

add_executable(${APP_NAME}
    main.cpp
    image_processor.cpp
    library_loader.cpp
    face_detector_library.cpp
    face_detector_wrapper.cpp
    file_utils.cpp
)

target_include_directories(${APP_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/facedetector
)

target_link_libraries(${APP_NAME}
    PRIVATE
        ${OpenCV_LIBS}
        JsonCpp::JsonCpp
        ${CMAKE_DL_LIBS}
)

# Copy data and library to output directory
copy_data_to_target(${APP_NAME})
copy_library_to_target(${APP_NAME} face_detector)

add_dependencies(${APP_NAME} face_detector)

# Install target
install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
