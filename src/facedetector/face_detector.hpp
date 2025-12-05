#ifndef FACE_DETECTOR_HPP
#define FACE_DETECTOR_HPP

#include <memory>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

#ifdef _WIN32
    #ifdef FACE_DETECTOR_EXPORTS
        #define FACE_DETECTOR_API __declspec(dllexport)
    #else
        #define FACE_DETECTOR_API __declspec(dllimport)
    #endif
#else
    #define FACE_DETECTOR_API __attribute__((visibility("default")))
#endif

namespace face_detector {

/**
 * @brief Rectangle representing detected face bounds
 */
struct FaceRect {
    int x;
    int y;
    int width;
    int height;
};

/**
 * @brief Result of face detection on a single image
 */
struct DetectionResult {
    std::string file_path;
    std::vector<FaceRect> faces;
    bool success;
    std::string error_message;
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
    
    ~FaceDetector();
    
    // Non-copyable
    FaceDetector(const FaceDetector&) = delete;
    FaceDetector& operator=(const FaceDetector&) = delete;
    
    // Movable
    FaceDetector(FaceDetector&&) noexcept;
    FaceDetector& operator=(FaceDetector&&) noexcept;

    /**
     * @brief Check if detector is properly initialized
     * @return true if ready to detect faces
     */
    bool isLoaded() const;

    /**
     * @brief Detect faces in an image file
     * @param image_path Path to the image file
     * @return DetectionResult containing face rectangles and status
     */
    DetectionResult detect(const std::string& image_path);

    /**
     * @brief Detect faces in an OpenCV Mat image
     * @param image Image to process
     * @return Vector of face rectangles
     */
    std::vector<FaceRect> detect(const cv::Mat& image);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief C-style interface for dynamic loading
 */
extern "C" {
    FACE_DETECTOR_API void* create_detector(const char* cascade_path);
    FACE_DETECTOR_API void destroy_detector(void* detector);
    FACE_DETECTOR_API int detect_faces(void* detector, const char* image_path, 
                                        FaceRect* out_faces, int max_faces);
    FACE_DETECTOR_API bool is_detector_loaded(void* detector);
}

} // namespace face_detector

#endif // FACE_DETECTOR_HPP
