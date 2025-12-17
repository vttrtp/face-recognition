if(EMSCRIPTEN)
    set(WASM_TARGET face_detector_wasm)
    
    add_executable(${WASM_TARGET}
        ${CMAKE_SOURCE_DIR}/src/generated/face_detector_wasm_bindings.cpp
    )
    
    target_link_libraries(${WASM_TARGET}
        PRIVATE
            face_detector
            ${OpenCV_LIBS}
    )
    
    target_include_directories(${WASM_TARGET}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/facedetector
            ${CMAKE_SOURCE_DIR}/src/generated
    )
    
    # Emscripten specific flags
    set_target_properties(${WASM_TARGET} PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "\
            -s WASM=1 \
            -s MODULARIZE=1 \
            -s EXPORT_NAME='createFaceDetectorModule' \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s MAXIMUM_MEMORY=512MB \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS'] \
            -s FORCE_FILESYSTEM=1 \
            --bind \
            -s FETCH=1 \
        "
    )
    
    # Create web directory and copy files
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    
    # Copy HTML file
    configure_file(
        ${CMAKE_SOURCE_DIR}/src/web/index.html
        ${CMAKE_BINARY_DIR}/web/index.html
        COPYONLY
    )
    
    # Copy test image for demo
    configure_file(
        ${CMAKE_SOURCE_DIR}/tests/data/test_image.jpg
        ${CMAKE_BINARY_DIR}/web/test_image.jpg
        COPYONLY
    )
    
    # Copy cascade file from build/data (downloaded by facedetector)
    configure_file(
        ${CMAKE_BINARY_DIR}/data/haarcascade_frontalface_default.xml
        ${CMAKE_BINARY_DIR}/web/haarcascade_frontalface_default.xml
        COPYONLY
    )
    
    # Copy output files to web directory after build
    add_custom_command(TARGET ${WASM_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:${WASM_TARGET}>/${WASM_TARGET}.js
            ${CMAKE_BINARY_DIR}/web/${WASM_TARGET}.js
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:${WASM_TARGET}>/${WASM_TARGET}.wasm
            ${CMAKE_BINARY_DIR}/web/${WASM_TARGET}.wasm
        COMMENT "Copying WASM files to web directory"
    )
endif()
