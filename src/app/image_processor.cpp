#include "image_processor.hpp"
#include "face_detector_wrapper.hpp"
#include "file_utils.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <json/json.h>

#include <fstream>
#include <iostream>
#include <algorithm>

namespace app {

namespace fs = std::filesystem;

// Supported image extensions
static const std::vector<std::string> IMAGE_EXTENSIONS = {
    ".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".tif"
};

ImageProcessor::ImageProcessor(FaceDetectorWrapper& detector)
    : m_detector(detector) {
}

bool ImageProcessor::isReady() const {
    return m_detector.isReady();
}

bool ImageProcessor::createBlurredImage(const std::string& input_path,
                                         const std::string& output_path,
                                         const DetectionResultData& detection) {
    cv::Mat image = cv::imread(input_path);
    if (image.empty()) {
        return false;
    }

    // Resize to half
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(), 0.5, 0.5, cv::INTER_AREA);

    // Blur face regions (scaled to half size)
    const FaceRect* faces = detection.data();
    for (int i = 0; i < detection.count(); ++i) {
        const auto& face = faces[i];
        // Scale face coordinates to resized image
        cv::Rect roi(
            face.x / 2,
            face.y / 2,
            face.width / 2,
            face.height / 2
        );

        // Clamp ROI to image bounds
        roi.x = std::max(0, roi.x);
        roi.y = std::max(0, roi.y);
        roi.width = std::min(roi.width, resized.cols - roi.x);
        roi.height = std::min(roi.height, resized.rows - roi.y);

        if (roi.width > 0 && roi.height > 0) {
            cv::Mat face_region = resized(roi);
            cv::GaussianBlur(face_region, face_region, cv::Size(99, 99), 30);
        }
    }

    // Create output directory if needed
    fs::path output_dir = fs::path(output_path).parent_path();
    if (!output_dir.empty() && !fs::exists(output_dir)) {
        fs::create_directories(output_dir);
    }

    return cv::imwrite(output_path, resized);
}

ImageResult ImageProcessor::processImage(const fs::path& image_path,
                                          const fs::path& input_root,
                                          const fs::path& output_root) {
    ImageResult result;
    result.original_path = image_path.string();
    result.success = false;

    result.detection = m_detector.detect(image_path.string());

    // Determine output directory
    fs::path output_dir;
    if (output_root.empty()) {
        // No output specified: save in same directory as the image
        output_dir = image_path.parent_path();
    } else {
        // Output specified: preserve relative path structure
        fs::path relative = fs::relative(image_path.parent_path(), input_root);
        output_dir = output_root / relative;
    }

    // Generate output filename: original_name_result.jpg
    std::string stem = image_path.stem().string();
    std::string output_filename = stem + "_result.jpg";
    fs::path output_path = output_dir / output_filename;

    result.result_path = output_path.string();

    if (!createBlurredImage(image_path.string(), output_path.string(), result.detection)) {
        result.error_message = "Failed to create output image";
        return result;
    }

    result.success = true;
    return result;
}

std::vector<ImageResult> ImageProcessor::processDirectory(const std::string& input_dir,
                                                           const std::string& output_dir) {
    std::vector<ImageResult> results;

    fs::path input_root(input_dir);
    fs::path output_root = output_dir.empty() ? fs::path() : fs::path(output_dir);

    if (!fs::exists(input_root) || !fs::is_directory(input_root)) {
        std::cerr << "[ImageProcessor] Invalid input directory: " << input_dir << std::endl;
        return results;
    }

    auto images = findFiles(input_root, IMAGE_EXTENSIONS);
    std::cout << "[ImageProcessor] Found " << images.size() << " images to process" << std::endl;

    results.reserve(images.size());
    for (const auto& image : images) {
        auto result = processImage(image, input_root, output_root);
        results.push_back(std::move(result));
    }

    return results;
}

bool ImageProcessor::saveResultsToJson(const std::vector<ImageResult>& results,
                                        const std::string& json_path) {
    Json::Value root(Json::arrayValue);

    for (const auto& result : results) {
        Json::Value item;
        item["original_file"] = result.original_path;
        item["result_file"] = result.result_path;
        item["success"] = result.success;
        
        if (!result.error_message.empty()) {
            item["error"] = result.error_message;
        }

        Json::Value faces_array(Json::arrayValue);
        const FaceRect* faces = result.detection.data();
        for (int i = 0; i < result.detection.count(); ++i) {
            const auto& face = faces[i];
            Json::Value face_obj;
            face_obj["x"] = face.x;
            face_obj["y"] = face.y;
            face_obj["width"] = face.width;
            face_obj["height"] = face.height;
            faces_array.append(face_obj);
        }
        item["faces"] = faces_array;
        item["face_count"] = result.detection.count();

        root.append(item);
    }

    std::ofstream file(json_path);
    if (!file.is_open()) {
        std::cerr << "[ImageProcessor] Failed to open JSON file for writing: " 
                  << json_path << std::endl;
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);

    std::cout << "[ImageProcessor] Results saved to: " << json_path << std::endl;
    return true;
}

} // namespace app
