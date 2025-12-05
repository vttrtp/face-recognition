#include <gtest/gtest.h>

#include "face_detector.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>

namespace fs = std::filesystem;

// Test fixture for FaceDetector tests
class FaceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Find cascade file
        cascade_path = findCascadeFile();
        ASSERT_FALSE(cascade_path.empty()) << "Haar cascade file not found";
        
        // Set up test data directory
        test_data_dir = findTestDataDir();
    }

    std::string findCascadeFile() {
        // Get executable directory
        fs::path exe_path = fs::canonical("/proc/self/exe");
        fs::path exe_dir = exe_path.parent_path();
        
        std::vector<fs::path> search_paths = {
            exe_dir / "data/haarcascade_frontalface_default.xml",
            "data/haarcascade_frontalface_default.xml",
            "../data/haarcascade_frontalface_default.xml",
            "../../data/haarcascade_frontalface_default.xml",
            "build/data/haarcascade_frontalface_default.xml",
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
        };

        for (const auto& path : search_paths) {
            if (fs::exists(path)) {
                return fs::canonical(path).string();
            }
        }
        return "";
    }

    fs::path findTestDataDir() {
        std::vector<fs::path> search_paths = {
            "data",
            "../data",
            "tests/data"
        };

        for (const auto& path : search_paths) {
            if (fs::exists(path)) {
                return fs::canonical(path);
            }
        }
        return fs::current_path() / "data";
    }

    // Create a test image with a simple pattern (not a real face)
    cv::Mat createTestImage(int width = 640, int height = 480) {
        cv::Mat image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
        return image;
    }

    // Create and save a temporary test image
    std::string createTempImage(const cv::Mat& image, const std::string& name) {
        fs::path temp_path = fs::temp_directory_path() / name;
        cv::imwrite(temp_path.string(), image);
        temp_files.push_back(temp_path);
        return temp_path.string();
    }

    void TearDown() override {
        // Clean up temporary files
        for (const auto& path : temp_files) {
            if (fs::exists(path)) {
                fs::remove(path);
            }
        }
    }

    std::string cascade_path;
    fs::path test_data_dir;
    std::vector<fs::path> temp_files;
};

// Test: Constructor with valid cascade file
TEST_F(FaceDetectorTest, ConstructorValidCascade) {
    face_detector::FaceDetector detector(cascade_path);
    EXPECT_TRUE(detector.isLoaded());
}

// Test: Constructor with invalid cascade file
TEST_F(FaceDetectorTest, ConstructorInvalidCascade) {
    face_detector::FaceDetector detector("non_existent_cascade.xml");
    EXPECT_FALSE(detector.isLoaded());
}

// Test: Detect on empty image
TEST_F(FaceDetectorTest, DetectEmptyMat) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat empty_image;
    auto faces = detector.detect(empty_image);
    EXPECT_TRUE(faces.empty());
}

// Test: Detect on valid image with no faces
TEST_F(FaceDetectorTest, DetectNoFaces) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat blank_image = createTestImage();
    auto faces = detector.detect(blank_image);
    EXPECT_TRUE(faces.empty());
}

// Test: Detect from file path - invalid file
TEST_F(FaceDetectorTest, DetectFromInvalidPath) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    auto result = detector.detect("non_existent_image.jpg");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Test: Detect from file path - valid file
TEST_F(FaceDetectorTest, DetectFromValidPath) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat test_image = createTestImage();
    std::string image_path = createTempImage(test_image, "test_image.jpg");

    auto result = detector.detect(image_path);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.file_path, image_path);
}

// Test: Grayscale image processing
TEST_F(FaceDetectorTest, DetectGrayscaleImage) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat color_image = createTestImage();
    cv::Mat gray_image;
    cv::cvtColor(color_image, gray_image, cv::COLOR_BGR2GRAY);

    auto faces = detector.detect(gray_image);
    // Should not crash, faces may or may not be empty
    SUCCEED();
}

// Test: FaceRect structure
TEST_F(FaceDetectorTest, FaceRectStructure) {
    face_detector::FaceRect rect{10, 20, 100, 150};
    
    EXPECT_EQ(rect.x, 10);
    EXPECT_EQ(rect.y, 20);
    EXPECT_EQ(rect.width, 100);
    EXPECT_EQ(rect.height, 150);
}

// Test: DetectionResult structure
TEST_F(FaceDetectorTest, DetectionResultStructure) {
    face_detector::DetectionResult result;
    result.file_path = "test.jpg";
    result.success = true;
    result.faces.push_back({0, 0, 50, 50});
    result.faces.push_back({100, 100, 50, 50});

    EXPECT_EQ(result.file_path, "test.jpg");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.faces.size(), 2);
}

// Test: C API - create and destroy detector
TEST_F(FaceDetectorTest, CAPICreateDestroy) {
    void* detector = face_detector::create_detector(cascade_path.c_str());
    ASSERT_NE(detector, nullptr);
    EXPECT_TRUE(face_detector::is_detector_loaded(detector));
    
    face_detector::destroy_detector(detector);
}

// Test: C API - create with null path
TEST_F(FaceDetectorTest, CAPICreateNullPath) {
    void* detector = face_detector::create_detector(nullptr);
    EXPECT_EQ(detector, nullptr);
}

// Test: C API - detect faces
TEST_F(FaceDetectorTest, CAPIDetectFaces) {
    void* detector = face_detector::create_detector(cascade_path.c_str());
    ASSERT_NE(detector, nullptr);

    cv::Mat test_image = createTestImage();
    std::string image_path = createTempImage(test_image, "capi_test.jpg");

    face_detector::FaceRect faces[10];
    int count = face_detector::detect_faces(detector, image_path.c_str(), faces, 10);
    
    EXPECT_GE(count, 0);  // Should return >= 0 for valid image

    face_detector::destroy_detector(detector);
}

// Test: C API - detect with null detector
TEST_F(FaceDetectorTest, CAPIDetectNullDetector) {
    face_detector::FaceRect faces[10];
    int count = face_detector::detect_faces(nullptr, "test.jpg", faces, 10);
    EXPECT_EQ(count, -1);
}

// Test: C API - detect with null path
TEST_F(FaceDetectorTest, CAPIDetectNullPath) {
    void* detector = face_detector::create_detector(cascade_path.c_str());
    ASSERT_NE(detector, nullptr);

    face_detector::FaceRect faces[10];
    int count = face_detector::detect_faces(detector, nullptr, faces, 10);
    EXPECT_EQ(count, -1);

    face_detector::destroy_detector(detector);
}

// Test: C API - is_detector_loaded with null
TEST_F(FaceDetectorTest, CAPIIsLoadedNull) {
    EXPECT_FALSE(face_detector::is_detector_loaded(nullptr));
}

// Test: Move semantics
TEST_F(FaceDetectorTest, MoveSemantics) {
    face_detector::FaceDetector detector1(cascade_path);
    ASSERT_TRUE(detector1.isLoaded());

    face_detector::FaceDetector detector2(std::move(detector1));
    EXPECT_TRUE(detector2.isLoaded());
}

// Test: Different image formats
TEST_F(FaceDetectorTest, DifferentImageFormats) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat test_image = createTestImage();

    // Test JPG
    std::string jpg_path = createTempImage(test_image, "test.jpg");
    auto jpg_result = detector.detect(jpg_path);
    EXPECT_TRUE(jpg_result.success);

    // Test PNG
    std::string png_path = createTempImage(test_image, "test.png");
    auto png_result = detector.detect(png_path);
    EXPECT_TRUE(png_result.success);
}
