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

# Copy test data and cascade files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
copy_data_to_target(${TEST_NAME})
