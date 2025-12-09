#include "face_detector_wrapper.hpp"
#include "library_loader.hpp"

#include <utility>

namespace app {

// DetectionResultData implementation

DetectionResultData::DetectionResultData(LibraryLoader& loader, DetectionResult* result)
    : loader_(&loader)
    , result_(result, [&loader](DetectionResult* r) {
        if (r) {
            loader.freeDetectionResult(r);
        }
    }) {
}

int DetectionResultData::count() const {
    if (!loader_ || !result_) {
        return 0;
    }
    int c = loader_->getFacesCount(result_.get());
    return c > 0 ? c : 0;
}

const FaceRect* DetectionResultData::data() const {
    if (!loader_ || !result_) {
        return nullptr;
    }
    return loader_->getFacesData(result_.get());
}

// FaceDetectorWrapper implementation

FaceDetectorWrapper::FaceDetectorWrapper(LibraryLoader& loader, const std::string& cascade_path)
    : loader_(&loader)
    , detector_(nullptr, [](void*){}) {
    if (loader_->isLoaded()) {
        auto* det = loader_->createDetector(cascade_path.c_str());
        detector_ = std::unique_ptr<void, Deleter>(det, [&loader](void* d) {
            if (d) {
                loader.destroyDetector(d);
            }
        });
    }
}

DetectionResultData FaceDetectorWrapper::detect(std::string_view image_path) {
    if (!loader_ || !detector_) {
        return {};
    }
    
    DetectionResult* result = loader_->detectFaces(detector_.get(), std::string(image_path).c_str());
    return DetectionResultData(*loader_, result);
}

} // namespace app
