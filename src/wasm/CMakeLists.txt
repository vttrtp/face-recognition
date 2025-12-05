if(EMSCRIPTEN)
    set(WASM_TARGET face_detector_wasm)
    
    add_executable(${WASM_TARGET}
        wasm_bindings.cpp
    )
    
    target_link_libraries(${WASM_TARGET}
        PRIVATE
            face_detector
            ${OpenCV_LIBS}
    )
    
    target_include_directories(${WASM_TARGET}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/facedetector
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
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
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
    
    # Download cascade file if not present
    set(CASCADE_FILE_NAME "haarcascade_frontalface_default.xml")
    set(CASCADE_DEST "${CMAKE_BINARY_DIR}/web/${CASCADE_FILE_NAME}")
    if(NOT EXISTS ${CASCADE_DEST})
        set(CASCADE_URL "https://raw.githubusercontent.com/opencv/opencv/4.x/data/haarcascades/${CASCADE_FILE_NAME}")
        message(STATUS "Downloading cascade file for web demo...")
        file(DOWNLOAD ${CASCADE_URL} ${CASCADE_DEST}
            STATUS DOWNLOAD_STATUS
            SHOW_PROGRESS
        )
    endif()
    
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
