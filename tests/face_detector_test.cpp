#include <gtest/gtest.h>

#include "face_detector.hpp"
#include "face_detector_c_api.h"

#include <opencv2/imgcodecs.hpp>

#include <filesystem>
#include <memory>

namespace {

struct DetectorDeleter {
    void operator()(FaceDetectorHandle* p) const { FaceDetector_destroy(p); }
};

struct ResultDeleter {
    void operator()(FaceDetectorResult* p) const { FaceDetector_freeResult(p); }
};

using DetectorPtr = std::unique_ptr<FaceDetectorHandle, DetectorDeleter>;
using ResultPtr = std::unique_ptr<FaceDetectorResult, ResultDeleter>;

}  // namespace

namespace fs = std::filesystem;

// Test fixture for FaceDetector tests
class FaceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        cascade_path = findCascadeFile();
        ASSERT_FALSE(cascade_path.empty()) << "Haar cascade file not found";
        
        test_image_path = findTestImage();
    }

    std::string findCascadeFile() {
        fs::path exe_path = fs::canonical("/proc/self/exe");
        fs::path exe_dir = exe_path.parent_path();
        
        std::vector<fs::path> search_paths = {
            exe_dir / "data" / "haarcascade_frontalface_default.xml"
        };

        for (const auto& path : search_paths) {
            if (fs::exists(path)) {
                return fs::canonical(path).string();
            }
        }
        return "";
    }

    std::string findTestImage() {
        fs::path exe_path = fs::canonical("/proc/self/exe");
        fs::path exe_dir = exe_path.parent_path();
        
        std::vector<fs::path> search_paths = {
            exe_dir / "data" / "test_image.jpg"
        };

        for (const auto& path : search_paths) {
            if (fs::exists(path)) {
                return fs::canonical(path).string();
            }
        }
        return "";
    }

    cv::Mat createBlankImage(int width = 640, int height = 480) {
        return cv::Mat(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    }

    std::string cascade_path;
    std::string test_image_path;
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

// Test: Detect on empty image returns empty result
TEST_F(FaceDetectorTest, DetectEmptyImage) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat empty_image;
    auto result = detector.detect(empty_image);
    EXPECT_TRUE(result.empty());
}

// Test: Detect on blank image with no faces
TEST_F(FaceDetectorTest, DetectNoFaces) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    cv::Mat blank_image = createBlankImage();
    auto result = detector.detect(blank_image);
    EXPECT_TRUE(result.empty());
}

// Test: Detect from non-existent file path
TEST_F(FaceDetectorTest, DetectFromInvalidPath) {
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    auto result = detector.detect("non_existent_image.jpg");
    EXPECT_TRUE(result.empty());
}

// Test: Detect faces in real test image
TEST_F(FaceDetectorTest, DetectFromRealImage) {
    ASSERT_FALSE(test_image_path.empty()) << "Test image not found";
    
    face_detector::FaceDetector detector(cascade_path);
    ASSERT_TRUE(detector.isLoaded());

    auto result = detector.detect(test_image_path);
    EXPECT_FALSE(result.empty()) << "Expected to detect faces in test image";
    EXPECT_EQ(result.faces.size(), 97) << "Unexpected number of faces detected";
    
    // Verify face rectangles have valid dimensions
    for (const auto& face : result.faces) {
        EXPECT_GT(face.width, 0);
        EXPECT_GT(face.height, 0);
    }
}

// Test: C API - create and destroy detector
TEST_F(FaceDetectorTest, CAPICreateDestroy) {
    DetectorPtr detector(FaceDetector_create(cascade_path.c_str()));
    ASSERT_NE(detector, nullptr);
}

// Test: C API - null/invalid inputs return nullptr
TEST_F(FaceDetectorTest, CAPIErrorHandling) {
    EXPECT_EQ(FaceDetector_create(nullptr), nullptr);
    
    // Creating detector with invalid cascade is allowed, but it will not be loaded
    DetectorPtr invalid_detector(FaceDetector_create("non_existent.xml"));
    ASSERT_NE(invalid_detector, nullptr);
    EXPECT_FALSE(FaceDetector_isLoaded(invalid_detector.get()));
    
    EXPECT_EQ(FaceDetector_detectFromFile(nullptr, "test.jpg"), nullptr);
    
    DetectorPtr detector(FaceDetector_create(cascade_path.c_str()));
    ASSERT_NE(detector, nullptr);
    EXPECT_EQ(FaceDetector_detectFromFile(detector.get(), nullptr), nullptr);
}

// Test: C API - detect faces in real image
TEST_F(FaceDetectorTest, CAPIDetectFaces) {
    ASSERT_FALSE(test_image_path.empty()) << "Test image not found";
    
    DetectorPtr detector(FaceDetector_create(cascade_path.c_str()));
    ASSERT_NE(detector, nullptr);

    ResultPtr result(FaceDetector_detectFromFile(detector.get(), test_image_path.c_str()));
    ASSERT_NE(result, nullptr);
    
    int count = FaceDetector_getResultCount(result.get());
    EXPECT_GT(count, 0) << "Expected to detect faces in test image";
    
    const FaceRect* faces = FaceDetector_getResultData(result.get());
    ASSERT_NE(faces, nullptr);
    EXPECT_GT(faces[0].width, 0);
    EXPECT_GT(faces[0].height, 0);
}
