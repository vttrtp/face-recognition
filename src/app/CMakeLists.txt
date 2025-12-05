set(APP_NAME face_recognition_app)

add_executable(${APP_NAME}
    main.cpp
    image_processor.cpp
    library_loader.cpp
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

# Copy cascade file next to executable (only if found)
if(EXISTS "${CMAKE_BINARY_DIR}/data")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${APP_NAME}>/data"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/data"
            "$<TARGET_FILE_DIR:${APP_NAME}>/data"
        COMMENT "Copying data files to output directory"
    )
endif()

# Copy the library next to executable for convenience
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:face_detector>"
        "$<TARGET_FILE_DIR:${APP_NAME}>"
    COMMENT "Copying face_detector library to output directory"
)

add_dependencies(${APP_NAME} face_detector)
