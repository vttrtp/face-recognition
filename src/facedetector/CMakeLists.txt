set(LIB_NAME face_detector)

# Use static library for WASM, shared for native
if(EMSCRIPTEN)
    add_library(${LIB_NAME} STATIC
        face_detector.cpp
        face_detector_interface.cpp
    )
else()
    add_library(${LIB_NAME} SHARED
        face_detector.cpp
        face_detector_interface.cpp
    )
    target_compile_definitions(${LIB_NAME} PRIVATE FACE_DETECTOR_EXPORTS)
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        ${OpenCV_LIBS}
)

# Find and copy OpenCV cascade classifier data (not needed for WASM)
if(NOT EMSCRIPTEN)
    set(CASCADE_FILE_NAME "haarcascade_frontalface_default.xml")
    set(CASCADE_FILE_DEST "${CMAKE_BINARY_DIR}/data/${CASCADE_FILE_NAME}")
    
    find_file(CASCADE_FILE 
        NAMES ${CASCADE_FILE_NAME}
        PATHS 
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/share/opencv4/haarcascades"
            "${OpenCV_DIR}/../../../share/opencv4/haarcascades"
            "${OpenCV_DIR}/../../share/opencv4/haarcascades"
            "${OpenCV_DIR}/../share/opencv4/haarcascades"
            "/usr/share/opencv4/haarcascades"
            "/usr/local/share/opencv4/haarcascades"
    )

    if(CASCADE_FILE)
        message(STATUS "Found cascade file: ${CASCADE_FILE}")
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
        configure_file(${CASCADE_FILE} ${CASCADE_FILE_DEST} COPYONLY)
    else()
        # Download from OpenCV GitHub repository
        set(CASCADE_URL "https://raw.githubusercontent.com/opencv/opencv/4.x/data/haarcascades/${CASCADE_FILE_NAME}")
        message(STATUS "Downloading cascade file from ${CASCADE_URL}")
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
        file(DOWNLOAD ${CASCADE_URL} ${CASCADE_FILE_DEST}
            STATUS DOWNLOAD_STATUS
            SHOW_PROGRESS
        )
        list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
        if(STATUS_CODE EQUAL 0)
            message(STATUS "Successfully downloaded cascade file")
        else()
            message(WARNING "Failed to download cascade file. You may need to provide it manually.")
        endif()
    endif()

    # Install targets
    install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES
        face_detector.hpp
        face_detector_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/face_detector
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/data/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/face_detector
        FILES_MATCHING PATTERN "*.xml"
    )
endif()
