#pragma once

#include <string>
#include <memory>
#include <functional>

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
 * @brief Singleton dynamic library loader for face_detector
 * 
 * Loads the face_detector shared library at runtime and exposes
 * raw function pointers. Use FaceDetectorWrapper for a safe C++ wrapper.
 */
class FaceDetectorLibrary {
public:
    using HandleDeleter = std::function<void(void*)>;

    /**
     * @brief Initialize the singleton with a library path
     * @param library_path Path to the face_detector library
     * @return true if loaded successfully
     */
    static bool initialize(const std::string& library_path);
    
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     * @throws std::runtime_error if not initialized
     */
    static FaceDetectorLibrary& instance();
    
    /**
     * @brief Check if singleton is initialized and loaded
     */
    static bool isInitialized() noexcept;

    ~FaceDetectorLibrary() = default;
    
    // Non-copyable, non-movable (singleton)
    FaceDetectorLibrary(const FaceDetectorLibrary&) = delete;
    FaceDetectorLibrary& operator=(const FaceDetectorLibrary&) = delete;
    FaceDetectorLibrary(FaceDetectorLibrary&&) = delete;
    FaceDetectorLibrary& operator=(FaceDetectorLibrary&&) = delete;

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
    FaceDetectorLibrary() = default;
    bool load(const std::string& library_path);
    
    std::unique_ptr<void, HandleDeleter> handle_;
    static std::unique_ptr<FaceDetectorLibrary> instance_;
};

} // namespace app
