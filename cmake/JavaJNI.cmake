# JNI Library for Java bindings using idl_target

option(BUILD_JAVA_JNI "Build Java JNI bindings" ON)

if(BUILD_JAVA_JNI)
    idl_target(
        NAME face_detector_jni
        TYPE JNI
        DEPENDENCIES face_detector
    )

    if(TARGET face_detector_jni)
        set_target_properties(face_detector_jni PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        )
    endif()
endif()
