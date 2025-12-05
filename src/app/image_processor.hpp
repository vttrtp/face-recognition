#ifndef IMAGE_PROCESSOR_HPP
#define IMAGE_PROCESSOR_HPP

#include <string>
#include <vector>
#include <filesystem>

#include "face_detector.hpp"

namespace app {

class LibraryLoader;

/**
 * @brief Structure to hold processing result for a single image
 */
struct ImageResult {
    std::string original_path;
    std::string result_path;
    std::vector<face_detector::FaceRect> faces;
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
     * @brief Construct processor with library loader and cascade path
     * @param loader Reference to loaded library
     * @param cascade_path Path to Haar cascade file
     */
    ImageProcessor(LibraryLoader& loader, const std::string& cascade_path);
    
    ~ImageProcessor();

    /**
     * @brief Check if processor is ready
     */
    bool isReady() const;

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
     * @brief Find all image files recursively
     * @param dir Directory to search
     * @return Vector of image file paths
     */
    std::vector<std::filesystem::path> findImages(const std::filesystem::path& dir);

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
                            const std::vector<face_detector::FaceRect>& faces);

    LibraryLoader& m_loader;
    void* m_detector;
    bool m_ready;
};

} // namespace app

#endif // IMAGE_PROCESSOR_HPP
