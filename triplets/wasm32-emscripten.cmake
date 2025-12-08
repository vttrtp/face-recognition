set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)

# Set the Emscripten toolchain
if(DEFINED ENV{EMSDK})
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
endif()

# Disable features not available in WASM for OpenCV
if(PORT MATCHES "opencv")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
        -DWITH_V4L=OFF
        -DWITH_FFMPEG=OFF
        -DWITH_GSTREAMER=OFF
        -DWITH_GTK=OFF
        -DWITH_1394=OFF
        -DENABLE_CONFIG_VERIFICATION=OFF
    )
endif()
