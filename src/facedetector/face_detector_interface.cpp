#include "face_detector_interface.h"
#include "face_detector.hpp"

// Define the opaque DetectionResult struct that wraps C++ DetectResult
struct DetectionResult {
    face_detector::DetectResult result;
};

extern "C" {

void* create_detector(const char* cascade_path) {
    if (!cascade_path) {
        return nullptr;
    }
    try {
        auto detector = std::make_unique<face_detector::FaceDetector>(cascade_path);
        if (!detector->isLoaded()) {
            return nullptr;
        }
        return detector.release();
    } catch (...) {
        return nullptr;
    }
}

void destroy_detector(void* detector) {
    delete static_cast<face_detector::FaceDetector*>(detector);
}

DetectionResult* detect_faces(void* detector, const char* image_path) {
    if (!detector || !image_path) {
        return nullptr;
    }

    auto* fd = static_cast<face_detector::FaceDetector*>(detector);
    auto cppResult = fd->detect(image_path);
    
    // Wrap C++ result in opaque C handle
    auto* result = new DetectionResult{std::move(cppResult)};
    return result;
}

int get_faces_count(const DetectionResult* result) {
    if (!result) {
        return -1;
    }
    return static_cast<int>(result->result.faces.size());
}

const FaceRect* get_faces_data(const DetectionResult* result) {
    if (!result || result->result.faces.empty()) {
        return nullptr;
    }
    return result->result.faces.data();
}

void free_detection_result(DetectionResult* result) {
    delete result;
}

} // extern "C"
