#include "face_detector_wrapper.hpp"
#include "face_detector_library.hpp"

namespace app {

// DetectionResultData implementation

DetectionResultData::DetectionResultData(DetectionResult* result)
    : result_(result, [](DetectionResult* r) {
        if (r) {
            FaceDetectorLibrary::instance().freeDetectionResult(r);
        }
    }) {
}

int DetectionResultData::count() const {
    if (!result_) {
        return 0;
    }
    int c = FaceDetectorLibrary::instance().getFacesCount(result_.get());
    return c > 0 ? c : 0;
}

const FaceRect* DetectionResultData::data() const {
    if (!result_) {
        return nullptr;
    }
    return FaceDetectorLibrary::instance().getFacesData(result_.get());
}

// FaceDetectorWrapper implementation

FaceDetectorWrapper::FaceDetectorWrapper(const std::string& cascade_path)
    : detector_(nullptr, [](void*){}) {
    auto& lib = FaceDetectorLibrary::instance();
    if (lib.isLoaded()) {
        auto* det = lib.createDetector(cascade_path.c_str());
        detector_ = std::unique_ptr<void, Deleter>(det, [](void* d) {
            if (d) {
                FaceDetectorLibrary::instance().destroyDetector(d);
            }
        });
    }
}

DetectionResultData FaceDetectorWrapper::detect(std::string_view image_path) {
    if (!detector_) {
        return {};
    }
    
    auto& lib = FaceDetectorLibrary::instance();
    DetectionResult* result = lib.detectFaces(detector_.get(), std::string(image_path).c_str());
    return DetectionResultData(result);
}

} // namespace app
