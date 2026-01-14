# WASM Face Detector - using idl_target

idl_target(
    NAME face_detector_wasm
    TYPE WASM
    DEPENDENCIES face_detector
    WASM_EXPORT_NAME FaceDetector
    LINK_LIBRARIES ${OpenCV_LIBS}
    LINK_OPTIONS
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','getValue']
        -sEXPORTED_FUNCTIONS=['_malloc','_free']
)

# Only proceed if target was created (WASM only built with Emscripten)
if(TARGET face_detector_wasm)
    set_target_properties(face_detector_wasm PROPERTIES SUFFIX ".js")

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
    add_custom_command(TARGET face_detector_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:face_detector_wasm>/face_detector_wasm.js
            ${CMAKE_BINARY_DIR}/web/face_detector_wasm.js
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:face_detector_wasm>/face_detector_wasm.wasm
            ${CMAKE_BINARY_DIR}/web/face_detector_wasm.wasm
        COMMENT "Copying WASM files to web directory"
    )
endif()
