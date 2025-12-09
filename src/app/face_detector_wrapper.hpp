#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <functional>

#include "face_detector_interface.h"

namespace app {

class LibraryLoader;

/**
 * @brief RAII wrapper for raw DetectionResult from C API
 */
class DetectionResultData {
public:
    using Deleter = std::function<void(DetectionResult*)>;

    DetectionResultData() = default;
    DetectionResultData(LibraryLoader& loader, DetectionResult* result);
    ~DetectionResultData() = default;
    
    // Non-copyable
    DetectionResultData(const DetectionResultData&) = delete;
    DetectionResultData& operator=(const DetectionResultData&) = delete;
    
    // Movable
    DetectionResultData(DetectionResultData&&) noexcept = default;
    DetectionResultData& operator=(DetectionResultData&&) noexcept = default;
    
    [[nodiscard]] int count() const;
    [[nodiscard]] const FaceRect* data() const;

private:
    LibraryLoader* loader_ = nullptr;
    std::unique_ptr<DetectionResult, Deleter> result_;
};

/**
 * @brief Safe C++ wrapper for face detector loaded via LibraryLoader
 * 
 * Provides RAII management of detector lifetime and type-safe interface.
 * No raw pointers exposed in the public API.
 */
class FaceDetectorWrapper {
public:
    using Deleter = std::function<void(void*)>;

    /**
     * @brief Construct wrapper with library loader and cascade path
     * @param loader Reference to loaded library (must outlive this wrapper)
     * @param cascade_path Path to Haar cascade XML file
     */
    FaceDetectorWrapper(LibraryLoader& loader, const std::string& cascade_path);
    
    ~FaceDetectorWrapper() = default;
    
    // Non-copyable
    FaceDetectorWrapper(const FaceDetectorWrapper&) = delete;
    FaceDetectorWrapper& operator=(const FaceDetectorWrapper&) = delete;
    
    // Movable
    FaceDetectorWrapper(FaceDetectorWrapper&&) noexcept = default;
    FaceDetectorWrapper& operator=(FaceDetectorWrapper&&) noexcept = default;

    /**
     * @brief Check if detector is ready
     */
    [[nodiscard]] bool isReady() const noexcept { return detector_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return isReady(); }

    /**
     * @brief Detect faces in an image file
     * @param image_path Path to image file
     * @return Detection result (wrapper)
     */
    [[nodiscard]] DetectionResultData detect(std::string_view image_path);

private:
    LibraryLoader* loader_ = nullptr;
    std::unique_ptr<void, Deleter> detector_;
};

} // namespace app
