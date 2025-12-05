#include "library_loader.hpp"

#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define LIB_HANDLE HMODULE
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) GetProcAddress(handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary(handle)
    #define LIB_ERROR() "Windows error code: " + std::to_string(GetLastError())
#else
    #include <dlfcn.h>
    #define LIB_HANDLE void*
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #define LIB_ERROR() (dlerror() ? dlerror() : "Unknown error")
#endif

namespace app {

// Function pointer types matching the C API
using CreateDetectorFn = void* (*)(const char*);
using DestroyDetectorFn = void (*)(void*);
using DetectFacesFn = int (*)(void*, const char*, face_detector::FaceRect*, int);
using IsDetectorLoadedFn = bool (*)(void*);

class LibraryLoader::Impl {
public:
    LIB_HANDLE handle = nullptr;
    
    CreateDetectorFn createDetector = nullptr;
    DestroyDetectorFn destroyDetector = nullptr;
    DetectFacesFn detectFaces = nullptr;
    IsDetectorLoadedFn isDetectorLoaded = nullptr;

    explicit Impl(const std::string& library_path) {
        handle = LOAD_LIBRARY(library_path.c_str());
        if (!handle) {
            std::cerr << "[LibraryLoader] Failed to load library: " << library_path
                      << "\nError: " << LIB_ERROR() << std::endl;
            return;
        }

        // Load function pointers
        createDetector = reinterpret_cast<CreateDetectorFn>(
            GET_SYMBOL(handle, "create_detector"));
        destroyDetector = reinterpret_cast<DestroyDetectorFn>(
            GET_SYMBOL(handle, "destroy_detector"));
        detectFaces = reinterpret_cast<DetectFacesFn>(
            GET_SYMBOL(handle, "detect_faces"));
        isDetectorLoaded = reinterpret_cast<IsDetectorLoadedFn>(
            GET_SYMBOL(handle, "is_detector_loaded"));

        if (!createDetector || !destroyDetector || !detectFaces || !isDetectorLoaded) {
            std::cerr << "[LibraryLoader] Failed to load all required symbols" << std::endl;
            CLOSE_LIBRARY(handle);
            handle = nullptr;
        }
    }

    ~Impl() {
        if (handle) {
            CLOSE_LIBRARY(handle);
        }
    }
};

LibraryLoader::LibraryLoader(const std::string& library_path)
    : pImpl(std::make_unique<Impl>(library_path)) {
}

LibraryLoader::~LibraryLoader() = default;

LibraryLoader::LibraryLoader(LibraryLoader&&) noexcept = default;
LibraryLoader& LibraryLoader::operator=(LibraryLoader&&) noexcept = default;

bool LibraryLoader::isLoaded() const {
    return pImpl && pImpl->handle != nullptr;
}

void* LibraryLoader::createDetector(const std::string& cascade_path) {
    if (!isLoaded()) {
        return nullptr;
    }
    return pImpl->createDetector(cascade_path.c_str());
}

void LibraryLoader::destroyDetector(void* detector) {
    if (isLoaded() && detector) {
        pImpl->destroyDetector(detector);
    }
}

bool LibraryLoader::isDetectorLoaded(void* detector) {
    if (!isLoaded() || !detector) {
        return false;
    }
    return pImpl->isDetectorLoaded(detector);
}

int LibraryLoader::detectFaces(void* detector, const std::string& image_path,
                                face_detector::FaceRect* out_faces, int max_faces) {
    if (!isLoaded()) {
        return -1;
    }
    return pImpl->detectFaces(detector, image_path.c_str(), out_faces, max_faces);
}

} // namespace app
