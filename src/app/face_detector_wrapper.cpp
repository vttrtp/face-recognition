#include "face_detector_wrapper.hpp"
#include "library_loader.hpp"

#include <utility>

namespace app {

// DetectionResultData implementation

DetectionResultData::DetectionResultData(LibraryLoader& loader, DetectionResult* result)
    : loader_(&loader)
    , result_(result) {
}

DetectionResultData::~DetectionResultData() {
    if (loader_ && result_) {
        loader_->freeDetectionResult(result_);
    }
}

DetectionResultData::DetectionResultData(DetectionResultData&& other) noexcept
    : loader_(std::exchange(other.loader_, nullptr))
    , result_(std::exchange(other.result_, nullptr)) {
}

DetectionResultData& DetectionResultData::operator=(DetectionResultData&& other) noexcept {
    if (this != &other) {
        if (loader_ && result_) {
            loader_->freeDetectionResult(result_);
        }
        loader_ = std::exchange(other.loader_, nullptr);
        result_ = std::exchange(other.result_, nullptr);
    }
    return *this;
}

int DetectionResultData::count() const {
    if (!loader_ || !result_) {
        return 0;
    }
    int c = loader_->getFacesCount(result_);
    return c > 0 ? c : 0;
}

const FaceRect* DetectionResultData::data() const {
    if (!loader_ || !result_) {
        return nullptr;
    }
    return loader_->getFacesData(result_);
}

std::vector<FaceRect> DetectionResultData::toVector() const {
    int c = count();
    const FaceRect* ptr = data();
    if (c <= 0 || !ptr) {
        return {};
    }
    return {ptr, ptr + c};
}

// FaceDetectorWrapper implementation

FaceDetectorWrapper::FaceDetectorWrapper(LibraryLoader& loader, const std::string& cascade_path)
    : loader_(&loader) {
    if (loader_->isLoaded()) {
        detector_ = loader_->createDetector(cascade_path.c_str());
    }
}

FaceDetectorWrapper::~FaceDetectorWrapper() {
    if (loader_ && detector_) {
        loader_->destroyDetector(detector_);
    }
}

FaceDetectorWrapper::FaceDetectorWrapper(FaceDetectorWrapper&& other) noexcept
    : loader_(other.loader_)
    , detector_(other.detector_) {
    other.loader_ = nullptr;
    other.detector_ = nullptr;
}

FaceDetectorWrapper& FaceDetectorWrapper::operator=(FaceDetectorWrapper&& other) noexcept {
    if (this != &other) {
        if (loader_ && detector_) {
            loader_->destroyDetector(detector_);
        }
        loader_ = other.loader_;
        detector_ = other.detector_;
        other.loader_ = nullptr;
        other.detector_ = nullptr;
    }
    return *this;
}

DetectionResultData FaceDetectorWrapper::detect(std::string_view image_path) {
    if (!loader_ || !detector_) {
        return {};
    }
    
    DetectionResult* result = loader_->detectFaces(detector_, std::string(image_path).c_str());
    return DetectionResultData(*loader_, result);
}

} // namespace app
