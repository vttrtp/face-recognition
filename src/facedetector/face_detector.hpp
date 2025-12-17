#pragma once

#include "face_detector_c_api.h"

#include <string>
#include <string_view>
#include <vector>
#include <opencv2/objdetect.hpp>

namespace face_detector {

/**
 * @brief Result of face detection - owns the data via vector
 */
struct DetectResult {
    std::vector<FaceRect> faces;
    
    [[nodiscard]] bool empty() const noexcept { return faces.empty(); }
    [[nodiscard]] explicit operator bool() const noexcept { return !faces.empty(); }
};

/**
 * @brief Face detector class using OpenCV Haar cascades
 */
class FACE_DETECTOR_API FaceDetector {
public:
    /**
     * @brief Construct a new Face Detector
     * @param cascade_path Path to Haar cascade XML file
     */
    explicit FaceDetector(const std::string& cascade_path);
    
    // Non-copyable (CascadeClassifier is not copyable)
    FaceDetector(const FaceDetector&) = delete;
    FaceDetector& operator=(const FaceDetector&) = delete;
    
    // Movable
    FaceDetector(FaceDetector&&) noexcept = default;
    FaceDetector& operator=(FaceDetector&&) noexcept = default;

    /**
     * @brief Check if detector is properly initialized
     * @return true if ready to detect faces
     */
    [[nodiscard]] bool isLoaded() const noexcept { return loaded_; }

    /**
     * @brief Detect faces in an OpenCV Mat image
     * @param image Image to process
     * @return DetectResult with vector of faces
     */
    [[nodiscard]] DetectResult detect(const cv::Mat& image);

    /**
     * @brief Detect faces in an image file
     * @param image_path Path to the image file
     * @return DetectResult with vector of faces
     */
    [[nodiscard]] DetectResult detect(std::string_view image_path);

    /**
     * @brief Detect faces in an image file (IDL-compatible name)
     * @param image_path Path to the image file
     * @return vector of faces
     */
    [[nodiscard]] std::vector<FaceRect> detectFromFile(const std::string& image_path) {
        return detect(image_path).faces;
    }

    /**
     * @brief Detect faces from raw RGBA image data (IDL-compatible)
     * @param imageData Pointer to RGBA pixel data
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @return vector of faces
     */
    [[nodiscard]] std::vector<FaceRect> detectFromImageData(const uint8_t* imageData, int width, int height);

private:
    cv::CascadeClassifier cascade_;
    bool loaded_ = false;
};

} // namespace face_detector
