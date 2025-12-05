set(TEST_NAME face_detector_test)

add_executable(${TEST_NAME}
    face_detector_test.cpp
)

target_include_directories(${TEST_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/facedetector
)

target_link_libraries(${TEST_NAME}
    PRIVATE
        face_detector
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${TEST_NAME})

# Copy test data
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy cascade file for tests (only if exists)
if(EXISTS "${CMAKE_BINARY_DIR}/data")
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/data"
            "$<TARGET_FILE_DIR:${TEST_NAME}>/data"
        COMMENT "Copying data files for tests"
    )
endif()
