# JNI Library for Java bindings
# Enabled by default - set BUILD_JAVA_JNI=OFF to disable

option(BUILD_JAVA_JNI "Build Java JNI bindings" ON)

if(BUILD_JAVA_JNI)
    find_package(JNI REQUIRED)
    
    if(JNI_FOUND)
        message(STATUS "JNI found: ${JNI_INCLUDE_DIRS}")
        
        add_library(face_detector_jni SHARED
            ${CMAKE_SOURCE_DIR}/src/generated/face_detector_jni.cpp
        )
        
        target_include_directories(face_detector_jni PRIVATE
            ${JNI_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/src/facedetector
            ${CMAKE_SOURCE_DIR}/src/generated
        )
        
        target_link_libraries(face_detector_jni PRIVATE
            face_detector_static
        )
        
        set_target_properties(face_detector_jni PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        )
        
        # Copy JNI library for Java sample
        add_custom_command(TARGET face_detector_jni POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
        )
    else()
        message(WARNING "JNI not found - Java bindings will not be built")
    endif()
endif()
