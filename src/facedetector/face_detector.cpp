#include "face_detector.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <iostream>

namespace face_detector {

class FaceDetector::Impl {
public:
    cv::CascadeClassifier cascade;
    bool loaded = false;

    explicit Impl(const std::string& cascade_path) {
        loaded = cascade.load(cascade_path);
        if (!loaded) {
            std::cerr << "[FaceDetector] Failed to load cascade from: " 
                      << cascade_path << std::endl;
        }
    }
};

FaceDetector::FaceDetector(const std::string& cascade_path)
    : pImpl(std::make_unique<Impl>(cascade_path)) {
}

FaceDetector::~FaceDetector() = default;

FaceDetector::FaceDetector(FaceDetector&&) noexcept = default;
FaceDetector& FaceDetector::operator=(FaceDetector&&) noexcept = default;

bool FaceDetector::isLoaded() const {
    return pImpl && pImpl->loaded;
}

std::vector<FaceRect> FaceDetector::detect(const cv::Mat& image) {
    std::vector<FaceRect> result;
    
    if (!isLoaded() || image.empty()) {
        return result;
    }

    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else if (image.channels() == 4) {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = image;
    }
    
    // Enhance contrast for better detection
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    pImpl->cascade.detectMultiScale(
        gray,
        faces,
        1.2,    // scale factor - higher = fewer false positives
        5,      // min neighbors - higher = more strict filtering
        0,      // flags
        cv::Size(60, 60)  // min face size - ignore small detections
    );

    result.reserve(faces.size());
    for (const auto& face : faces) {
        result.push_back({face.x, face.y, face.width, face.height});
    }

    return result;
}

DetectionResult FaceDetector::detect(const std::string& image_path) {
    DetectionResult result;
    result.file_path = image_path;
    result.success = false;

    if (!isLoaded()) {
        result.error_message = "Detector not initialized";
        std::cout << "[FaceDetector] Error: " << result.error_message << std::endl;
        return result;
    }

    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        result.error_message = "Failed to load image: " + image_path;
        std::cout << "[FaceDetector] " << result.error_message << std::endl;
        return result;
    }

    result.faces = detect(image);
    result.success = true;

    // Print notification about processing completion
    std::cout << "[FaceDetector] Processed: " << image_path 
              << " | Faces found: " << result.faces.size() << std::endl;

    return result;
}

// C-style interface implementation for dynamic loading
extern "C" {

void* create_detector(const char* cascade_path) {
    if (!cascade_path) {
        return nullptr;
    }
    try {
        return new FaceDetector(cascade_path);
    } catch (...) {
        return nullptr;
    }
}

void destroy_detector(void* detector) {
    delete static_cast<FaceDetector*>(detector);
}

int detect_faces(void* detector, const char* image_path, 
                 FaceRect* out_faces, int max_faces) {
    if (!detector || !image_path || !out_faces || max_faces <= 0) {
        return -1;
    }

    auto* fd = static_cast<FaceDetector*>(detector);
    auto result = fd->detect(image_path);
    
    if (!result.success) {
        return -1;
    }

    int count = std::min(static_cast<int>(result.faces.size()), max_faces);
    for (int i = 0; i < count; ++i) {
        out_faces[i] = result.faces[i];
    }

    return static_cast<int>(result.faces.size());
}

bool is_detector_loaded(void* detector) {
    if (!detector) {
        return false;
    }
    return static_cast<FaceDetector*>(detector)->isLoaded();
}

} // extern "C"

} // namespace face_detector
