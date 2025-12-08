#pragma once

#include <string>

#include "face_detector_interface.h"

namespace app {

// Function pointer types matching the C API
using CreateDetectorFn = void* (*)(const char*);
using DestroyDetectorFn = void (*)(void*);
using DetectFacesFn = DetectionResult* (*)(void*, const char*);
using GetFacesCountFn = int (*)(const DetectionResult*);
using GetFacesDataFn = const FaceRect* (*)(const DetectionResult*);
using FreeDetectionResultFn = void (*)(DetectionResult*);

/**
 * @brief Minimal dynamic library loader for face_detector
 * 
 * Loads the face_detector shared library at runtime and exposes
 * raw function pointers. Use FaceDetectorWrapper for a safe C++ wrapper.
 */
class LibraryLoader {
public:
    explicit LibraryLoader(const std::string& library_path);
    ~LibraryLoader();
    
    // Non-copyable
    LibraryLoader(const LibraryLoader&) = delete;
    LibraryLoader& operator=(const LibraryLoader&) = delete;
    
    // Movable
    LibraryLoader(LibraryLoader&&) noexcept;
    LibraryLoader& operator=(LibraryLoader&&) noexcept;

    [[nodiscard]] bool isLoaded() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return isLoaded(); }

    // Raw function pointers - use FaceDetectorWrapper for safe wrapper
    CreateDetectorFn createDetector = nullptr;
    DestroyDetectorFn destroyDetector = nullptr;
    DetectFacesFn detectFaces = nullptr;
    GetFacesCountFn getFacesCount = nullptr;
    GetFacesDataFn getFacesData = nullptr;
    FreeDetectionResultFn freeDetectionResult = nullptr;

private:
    void* handle_ = nullptr;
};

} // namespace app
