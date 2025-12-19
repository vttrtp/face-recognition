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
    
    # Base linker flags (common to both single-threaded and multithreaded)
    set(WASM_LINK_FLAGS "\
        -sWASM=1 \
        -sMODULARIZE=1 \
        -sEXPORT_NAME='createFaceDetectorModule' \
        -sALLOW_MEMORY_GROWTH=1 \
        -sMAXIMUM_MEMORY=512MB \
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','getValue'] \
        -sEXPORTED_FUNCTIONS=['_malloc','_free'] \
        -sFORCE_FILESYSTEM=1 \
        --bind \
        -sFETCH=1 \
    ")
    
    # Conditionally add pthread support
    if(WASM_THREADS)
        message(STATUS "Building WASM with pthread support")
        target_compile_options(${WASM_TARGET}
            PRIVATE
                -pthread
                -matomics
                -mbulk-memory
        )
        set(WASM_LINK_FLAGS "${WASM_LINK_FLAGS} \
            -pthread \
            -sUSE_PTHREADS=1 \
            -sENVIRONMENT='web,worker' \
            -sPTHREAD_POOL_SIZE_STRICT=0 \
        ")
    else()
        message(STATUS "Building WASM single-threaded (no pthreads)")
    endif()

    # Emscripten specific flags
    set_target_properties(${WASM_TARGET} PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "${WASM_LINK_FLAGS}"
    )
    
    # Create web directory and copy files
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    
    # Copy HTML file
    configure_file(
        ${CMAKE_SOURCE_DIR}/src/web/index.html
        ${CMAKE_BINARY_DIR}/web/index.html
        COPYONLY
    )
    
    # Copy service worker for GitHub Pages COOP/COEP headers
    configure_file(
        ${CMAKE_SOURCE_DIR}/src/web/coi-serviceworker.js
        ${CMAKE_BINARY_DIR}/web/coi-serviceworker.js
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
