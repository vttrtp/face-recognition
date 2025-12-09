#include "face_detector_library.hpp"

#include <iostream>
#include <stdexcept>

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

std::unique_ptr<FaceDetectorLibrary> FaceDetectorLibrary::instance_{};

bool FaceDetectorLibrary::initialize(const std::string& library_path) {
    if (instance_) {
        return instance_->isLoaded();
    }
    instance_.reset(new FaceDetectorLibrary());
    return instance_->load(library_path);
}

FaceDetectorLibrary& FaceDetectorLibrary::instance() {
    if (!instance_) {
        throw std::runtime_error("FaceDetectorLibrary not initialized. Call initialize() first.");
    }
    return *instance_;
}

bool FaceDetectorLibrary::isInitialized() noexcept {
    return instance_ != nullptr && instance_->isLoaded();
}

bool FaceDetectorLibrary::load(const std::string& library_path) {
    void* raw_handle = LOAD_LIBRARY(library_path.c_str());
    if (!raw_handle) {
        std::cerr << "[FaceDetectorLibrary] Failed to load library: " << library_path
                  << "\nError: " << LIB_ERROR() << std::endl;
        return false;
    }
    
    handle_ = {raw_handle, [](void* h) { CLOSE_LIBRARY(h); }};

    // Load function pointers
    createDetector = reinterpret_cast<CreateDetectorFn>(
        GET_SYMBOL(handle_.get(), "create_detector"));
    destroyDetector = reinterpret_cast<DestroyDetectorFn>(
        GET_SYMBOL(handle_.get(), "destroy_detector"));
    detectFaces = reinterpret_cast<DetectFacesFn>(
        GET_SYMBOL(handle_.get(), "detect_faces"));
    getFacesCount = reinterpret_cast<GetFacesCountFn>(
        GET_SYMBOL(handle_.get(), "get_faces_count"));
    getFacesData = reinterpret_cast<GetFacesDataFn>(
        GET_SYMBOL(handle_.get(), "get_faces_data"));
    freeDetectionResult = reinterpret_cast<FreeDetectionResultFn>(
        GET_SYMBOL(handle_.get(), "free_detection_result"));

    if (!createDetector || !destroyDetector || !detectFaces || 
        !getFacesCount || !getFacesData || !freeDetectionResult) {
        std::cerr << "[FaceDetectorLibrary] Failed to load all required symbols" << std::endl;
        handle_.reset();
        return false;
    }
    return true;
}

} // namespace app
