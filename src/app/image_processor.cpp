#include "image_processor.hpp"
#include "library_loader.hpp"

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

ImageProcessor::ImageProcessor(LibraryLoader& loader, const std::string& cascade_path)
    : m_loader(loader)
    , m_detector(nullptr)
    , m_ready(false) {
    
    if (!m_loader.isLoaded()) {
        std::cerr << "[ImageProcessor] Library not loaded" << std::endl;
        return;
    }

    m_detector = m_loader.createDetector(cascade_path);
    if (!m_detector) {
        std::cerr << "[ImageProcessor] Failed to create detector" << std::endl;
        return;
    }

    m_ready = m_loader.isDetectorLoaded(m_detector);
    if (!m_ready) {
        std::cerr << "[ImageProcessor] Detector not properly initialized" << std::endl;
    }
}

ImageProcessor::~ImageProcessor() {
    if (m_detector) {
        m_loader.destroyDetector(m_detector);
    }
}

bool ImageProcessor::isReady() const {
    return m_ready;
}

std::vector<fs::path> ImageProcessor::findImages(const fs::path& dir) {
    std::vector<fs::path> images;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (std::find(IMAGE_EXTENSIONS.begin(), IMAGE_EXTENSIONS.end(), ext) 
                != IMAGE_EXTENSIONS.end()) {
                images.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ImageProcessor] Filesystem error: " << e.what() << std::endl;
    }

    return images;
}

bool ImageProcessor::createBlurredImage(const std::string& input_path,
                                         const std::string& output_path,
                                         const std::vector<face_detector::FaceRect>& faces) {
    cv::Mat image = cv::imread(input_path);
    if (image.empty()) {
        return false;
    }

    // Resize to half
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(), 0.5, 0.5, cv::INTER_AREA);

    // Blur face regions (scaled to half size)
    for (const auto& face : faces) {
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
                                          const fs::path& output_dir) {
    ImageResult result;
    result.original_path = image_path.string();
    result.success = false;

    // Maximum faces to detect per image
    constexpr int MAX_FACES = 100;
    std::vector<face_detector::FaceRect> faces(MAX_FACES);

    int face_count = m_loader.detectFaces(
        m_detector,
        image_path.string(),
        faces.data(),
        MAX_FACES
    );

    if (face_count < 0) {
        result.error_message = "Detection failed";
        return result;
    }

    faces.resize(face_count);
    result.faces = faces;

    // Generate output filename: original_name_result.jpg
    std::string stem = image_path.stem().string();
    std::string output_filename = stem + "_result.jpg";
    fs::path output_path = output_dir / output_filename;

    // Handle filename collisions
    int counter = 1;
    while (fs::exists(output_path)) {
        output_filename = stem + "_result_" + std::to_string(counter++) + ".jpg";
        output_path = output_dir / output_filename;
    }

    result.result_path = output_path.string();

    if (!createBlurredImage(image_path.string(), output_path.string(), faces)) {
        result.error_message = "Failed to create output image";
        return result;
    }

    result.success = true;
    return result;
}

std::vector<ImageResult> ImageProcessor::processDirectory(const std::string& input_dir,
                                                           const std::string& output_dir) {
    std::vector<ImageResult> results;

    fs::path input_path(input_dir);
    fs::path output_path = output_dir.empty() ? input_path : fs::path(output_dir);

    if (!fs::exists(input_path) || !fs::is_directory(input_path)) {
        std::cerr << "[ImageProcessor] Invalid input directory: " << input_dir << std::endl;
        return results;
    }

    auto images = findImages(input_path);
    std::cout << "[ImageProcessor] Found " << images.size() << " images to process" << std::endl;

    results.reserve(images.size());
    for (const auto& image : images) {
        auto result = processImage(image, output_path);
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
        for (const auto& face : result.faces) {
            Json::Value face_obj;
            face_obj["x"] = face.x;
            face_obj["y"] = face.y;
            face_obj["width"] = face.width;
            face_obj["height"] = face.height;
            faces_array.append(face_obj);
        }
        item["faces"] = faces_array;
        item["face_count"] = static_cast<int>(result.faces.size());

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
