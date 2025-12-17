#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>

#include "face_detector_client.hpp"

namespace app {

/**
 * @brief Structure to hold processing result for a single image
 */
struct ImageResult {
    std::string original_path;
    std::string result_path;
    face_detector_client::DetectionResult detection;
    bool success;
    std::string error_message;
};

/**
 * @brief Image processor that uses face_detector library
 * 
 * Recursively finds images in a directory, detects faces,
 * creates blurred output images, and saves results to JSON.
 */
class ImageProcessor {
public:
    /**
     * @brief Construct processor with face detector
     * @param detector Shared pointer to face detector
     */
    explicit ImageProcessor(const std::shared_ptr<face_detector_client::FaceDetector>& detector);
    
    ~ImageProcessor() = default;

    /**
     * @brief Check if processor is ready
     */
    [[nodiscard]] bool isReady() const;

    /**
     * @brief Process all images in a directory recursively
     * @param input_dir Root directory to search for images
     * @param output_dir Directory to save result images (same as input_dir if empty)
     * @return Vector of processing results
     */
    std::vector<ImageResult> processDirectory(const std::string& input_dir,
                                               const std::string& output_dir = "");

    /**
     * @brief Save results to JSON file
     * @param results Processing results to save
     * @param json_path Path to output JSON file
     * @return true on success
     */
    bool saveResultsToJson(const std::vector<ImageResult>& results,
                           const std::string& json_path);

private:
    /**
     * @brief Process a single image
     * @param image_path Path to input image
     * @param input_root Root input directory (for calculating relative paths)
     * @param output_root Root output directory (empty = save next to original)
     * @return Processing result
     */
    ImageResult processImage(const std::filesystem::path& image_path,
                             const std::filesystem::path& input_root,
                             const std::filesystem::path& output_root);

    /**
     * @brief Create output image with blurred faces
     * @param input_path Path to original image
     * @param output_path Path to save result
     * @param detection Detected face data
     * @return true on success
     */
    bool createBlurredImage(const std::string& input_path,
                            const std::string& output_path,
                            const face_detector_client::DetectionResult& detection);

    std::shared_ptr<face_detector_client::FaceDetector> m_detector;
};

} // namespace app
