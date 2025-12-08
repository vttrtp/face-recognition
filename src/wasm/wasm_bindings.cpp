#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <face_detector.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <fstream>
#include <cstdio>
#include <iostream>

using namespace emscripten;

/**
 * WASM wrapper for FaceDetector library.
 * Uses the core face_detector library but adapted for browser environment.
 * Returns rectangle data for JavaScript to draw.
 */
class WasmFaceDetector {
public:
    WasmFaceDetector() : detector_(nullptr) {}
    
    ~WasmFaceDetector() {
        // Clean up temporary file if it exists
        if (!temp_cascade_path_.empty()) {
            std::remove(temp_cascade_path_.c_str());
        }
    }
    
    /**
     * Load cascade from XML string data (fetched via JavaScript)
     * Creates a temporary file and initializes the FaceDetector library
     */
    bool loadCascade(const std::string& cascade_data) {
        // Write cascade data to a temporary file for FaceDetector to load
        temp_cascade_path_ = "/tmp/cascade.xml";
        
        std::ofstream ofs(temp_cascade_path_);
        if (!ofs.is_open()) {
            return false;
        }
        ofs << cascade_data;
        ofs.close();
        
        // Initialize the FaceDetector library with the cascade file
        detector_ = std::make_unique<face_detector::FaceDetector>(temp_cascade_path_);
        return detector_->isLoaded();
    }
    
    bool isLoaded() const {
        return detector_ && detector_->isLoaded();
    }
    
    /**
     * Detect faces in image data from canvas.
     * Returns array of rectangle objects {x, y, width, height} for JS to draw.
     */
    val detectFaces(val imageData, int width, int height) {
        val result = val::array();
        
        if (!isLoaded()) {
            return result;
        }
        
        // Convert JavaScript Uint8ClampedArray to cv::Mat (RGBA format from canvas)
        std::vector<uint8_t> data = vecFromJSArray<uint8_t>(imageData);
        cv::Mat rgba(height, width, CV_8UC4, data.data());
        
        // Convert RGBA to BGR for OpenCV processing
        cv::Mat bgr;
        cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
        
        // Use the FaceDetector library's detect method
        auto detectResult = detector_->detect(bgr);

        // Convert FaceRect results to JavaScript array of objects
        for (const auto& face : detectResult.faces) {
            val faceObj = val::object();
            faceObj.set("x", face.x);
            faceObj.set("y", face.y);
            faceObj.set("width", face.width);
            faceObj.set("height", face.height);
            result.call<void>("push", faceObj);
        }
        std::cout << "Detected " << detectResult.faces.size() << " faces." << std::endl;
        return result;
    }
    
private:
    std::unique_ptr<face_detector::FaceDetector> detector_;
    std::string temp_cascade_path_;
};

EMSCRIPTEN_BINDINGS(face_detector) {
    class_<WasmFaceDetector>("FaceDetector")
        .constructor<>()
        .function("loadCascade", &WasmFaceDetector::loadCascade)
        .function("isLoaded", &WasmFaceDetector::isLoaded)
        .function("detectFaces", &WasmFaceDetector::detectFaces);
}
