#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "face_detector_interface.h"

namespace app {

class FaceDetectorWrapper;

/**
 * @brief Structure to hold processing result for a single image
 */
struct ImageResult {
    std::string original_path;
    std::string result_path;
    std::vector<FaceRect> faces;
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
     * @brief Construct processor with face detector wrapper
     * @param detector Reference to face detector wrapper (must outlive this processor)
     */
    explicit ImageProcessor(FaceDetectorWrapper& detector);
    
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
     * @param output_dir Directory for output image
     * @return Processing result
     */
    ImageResult processImage(const std::filesystem::path& image_path,
                             const std::filesystem::path& output_dir);

    /**
     * @brief Create output image with blurred faces
     * @param input_path Path to original image
     * @param output_path Path to save result
     * @param faces Detected face rectangles
     * @return true on success
     */
    bool createBlurredImage(const std::string& input_path,
                            const std::string& output_path,
                            const std::vector<FaceRect>& faces);

    FaceDetectorWrapper& m_detector;
};

} // namespace app
