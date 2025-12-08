#include "library_loader.hpp"

#include <iostream>
#include <utility>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name))
    #define CLOSE_LIBRARY(handle) FreeLibrary(static_cast<HMODULE>(handle))
    #define LIB_ERROR() "Windows error code: " + std::to_string(GetLastError())
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #define LIB_ERROR() (dlerror() ? dlerror() : "Unknown error")
#endif

namespace app {

LibraryLoader::LibraryLoader(const std::string& library_path) {
    handle_ = LOAD_LIBRARY(library_path.c_str());
    if (!handle_) {
        std::cerr << "[LibraryLoader] Failed to load library: " << library_path
                  << "\nError: " << LIB_ERROR() << std::endl;
        return;
    }

    // Load function pointers
    createDetector = reinterpret_cast<CreateDetectorFn>(
        GET_SYMBOL(handle_, "create_detector"));
    destroyDetector = reinterpret_cast<DestroyDetectorFn>(
        GET_SYMBOL(handle_, "destroy_detector"));
    detectFaces = reinterpret_cast<DetectFacesFn>(
        GET_SYMBOL(handle_, "detect_faces"));
    getFacesCount = reinterpret_cast<GetFacesCountFn>(
        GET_SYMBOL(handle_, "get_faces_count"));
    getFacesData = reinterpret_cast<GetFacesDataFn>(
        GET_SYMBOL(handle_, "get_faces_data"));
    freeDetectionResult = reinterpret_cast<FreeDetectionResultFn>(
        GET_SYMBOL(handle_, "free_detection_result"));

    if (!createDetector || !destroyDetector || !detectFaces || 
        !getFacesCount || !getFacesData || !freeDetectionResult) {
        std::cerr << "[LibraryLoader] Failed to load all required symbols" << std::endl;
        CLOSE_LIBRARY(handle_);
        handle_ = nullptr;
    }
}

LibraryLoader::~LibraryLoader() {
    if (handle_) {
        CLOSE_LIBRARY(handle_);
    }
}

LibraryLoader::LibraryLoader(LibraryLoader&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr))
    , createDetector(std::exchange(other.createDetector, nullptr))
    , destroyDetector(std::exchange(other.destroyDetector, nullptr))
    , detectFaces(std::exchange(other.detectFaces, nullptr))
    , getFacesCount(std::exchange(other.getFacesCount, nullptr))
    , getFacesData(std::exchange(other.getFacesData, nullptr))
    , freeDetectionResult(std::exchange(other.freeDetectionResult, nullptr)) {
}

LibraryLoader& LibraryLoader::operator=(LibraryLoader&& other) noexcept {
    if (this != &other) {
        if (handle_) {
            CLOSE_LIBRARY(handle_);
        }
        handle_ = std::exchange(other.handle_, nullptr);
        createDetector = std::exchange(other.createDetector, nullptr);
        destroyDetector = std::exchange(other.destroyDetector, nullptr);
        detectFaces = std::exchange(other.detectFaces, nullptr);
        getFacesCount = std::exchange(other.getFacesCount, nullptr);
        getFacesData = std::exchange(other.getFacesData, nullptr);
        freeDetectionResult = std::exchange(other.freeDetectionResult, nullptr);
    }
    return *this;
}

} // namespace app
