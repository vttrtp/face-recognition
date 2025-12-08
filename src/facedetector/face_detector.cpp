#include "face_detector.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <iostream>

namespace face_detector {

FaceDetector::FaceDetector(const std::string& cascade_path) {
    loaded_ = cascade_.load(cascade_path);
    if (!loaded_) {
        std::cerr << "[FaceDetector] Failed to load cascade from: " 
                  << cascade_path << std::endl;
    }
}

DetectResult FaceDetector::detect(const cv::Mat& image) {
    DetectResult result;
    
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

    auto start = std::chrono::steady_clock::now();

    std::vector<cv::Rect> cvFaces;
    cascade_.detectMultiScale(
        gray,
        cvFaces,
        1.2,    // scale factor - higher = fewer false positives
        5,      // min neighbors - higher = more strict filtering
        0,      // flags
        cv::Size(60, 60)  // min face size - ignore small detections
    );

    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    result.faces.reserve(cvFaces.size());
    for (const auto& face : cvFaces) {
        result.faces.push_back({face.x, face.y, face.width, face.height});
    }

    std::cout << "[FaceDetector] Faces found: " << result.faces.size() 
              << " | Time: " << duration_ms << " ms" << std::endl;

    return result;
}

DetectResult FaceDetector::detect(std::string_view image_path) {
    if (!isLoaded()) {
        std::cerr << "[FaceDetector] Error: Detector not initialized" << std::endl;
        return {};
    }

    cv::Mat image = cv::imread(std::string(image_path));
    if (image.empty()) {
        std::cerr << "[FaceDetector] Failed to load image: " << image_path << std::endl;
        return {};
    }

    std::cout << "[FaceDetector] Processing: " << image_path << std::endl;
    return detect(image);
}

} // namespace face_detector
