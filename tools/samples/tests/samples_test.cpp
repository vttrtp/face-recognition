#include <gtest/gtest.h>

#include "samples.hpp"
#include "samples_c_api.h"

#include <memory>
#include <vector>

namespace {

// Smart pointer deleters for C API handles
struct CalculatorDeleter {
    void operator()(CalculatorHandle* p) const { Calculator_destroy(p); }
};

struct GeometryDeleter {
    void operator()(GeometryHandle* p) const { Geometry_destroy(p); }
};

struct ShapeProcessorDeleter {
    void operator()(ShapeProcessorHandle* p) const { ShapeProcessor_destroy(p); }
};

struct AsyncProcessorDeleter {
    void operator()(AsyncProcessorHandle* p) const { AsyncProcessor_destroy(p); }
};

using CalculatorPtr = std::unique_ptr<CalculatorHandle, CalculatorDeleter>;
using GeometryPtr = std::unique_ptr<GeometryHandle, GeometryDeleter>;
using ShapeProcessorPtr = std::unique_ptr<ShapeProcessorHandle, ShapeProcessorDeleter>;
using AsyncProcessorPtr = std::unique_ptr<AsyncProcessorHandle, AsyncProcessorDeleter>;

}  // namespace

// ============================================================================
// Calculator Tests
// ============================================================================

TEST(CalculatorTest, BasicArithmetic) {
    samples::Calculator calc;
    
    EXPECT_EQ(calc.add(2, 3), 5);
    EXPECT_EQ(calc.subtract(10, 4), 6);
    EXPECT_EQ(calc.multiply(3, 7), 21);
    EXPECT_DOUBLE_EQ(calc.divide(10.0, 4.0), 2.5);
    EXPECT_DOUBLE_EQ(calc.divide(5.0, 0.0), 0.0);
}

TEST(CalculatorTest, Version) {
    samples::Calculator calc;
    
    EXPECT_EQ(calc.getVersionMajor(), 1);
    EXPECT_EQ(calc.getVersionMinor(), 0);
}

TEST(CalculatorTest, CAPIBasicArithmetic) {
    CalculatorPtr calc(Calculator_create());
    ASSERT_NE(calc, nullptr);
    
    EXPECT_EQ(Calculator_add(calc.get(), 5, 3), 8);
    EXPECT_EQ(Calculator_subtract(calc.get(), 10, 7), 3);
    EXPECT_EQ(Calculator_multiply(calc.get(), 4, 6), 24);
    EXPECT_DOUBLE_EQ(Calculator_divide(calc.get(), 15.0, 3.0), 5.0);
}

// ============================================================================
// Geometry Tests
// ============================================================================

TEST(GeometryTest, CreateLine) {
    samples::Geometry geom;
    
    auto points = geom.createLine(0, 0, 10, 10, 3);
    ASSERT_EQ(points.size(), 3u);
    EXPECT_EQ(points[0].x, 0);
    EXPECT_EQ(points[0].y, 0);
    EXPECT_EQ(points[2].x, 10);
    EXPECT_EQ(points[2].y, 10);
    
    EXPECT_EQ(geom.getLastCount(), 3);
}

TEST(GeometryTest, FindBoundingBoxes) {
    samples::Geometry geom;
    
    auto boxes = geom.findBoundingBoxes(2);
    ASSERT_EQ(boxes.size(), 2u);
    EXPECT_EQ(boxes[0].x, 0);
    EXPECT_EQ(boxes[1].x, 10);
    
    EXPECT_EQ(geom.getLastCount(), 2);
}

TEST(GeometryTest, CAPICreateLine) {
    GeometryPtr geom(Geometry_create());
    ASSERT_NE(geom, nullptr);
    
    struct PointResultDeleter {
        void operator()(Geometry_Point_CResult* p) const { Geometry_Point_CResult_free(p); }
    };
    using PointResultPtr = std::unique_ptr<Geometry_Point_CResult, PointResultDeleter>;
    
    PointResultPtr result(Geometry_createLine(geom.get(), 0, 0, 100, 100, 5));
    ASSERT_NE(result, nullptr);
    
    int count = Geometry_Point_CResult_getCount(result.get());
    EXPECT_EQ(count, 5);
    
    const Point* data = Geometry_Point_CResult_getData(result.get());
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data[0].x, 0);
    EXPECT_EQ(data[4].x, 100);
}

// ============================================================================
// ShapeProcessor Tests
// ============================================================================

TEST(ShapeProcessorTest, CalculateArea) {
    samples::ShapeProcessor processor;
    
    BoundingBox box = {10, 20, 100, 50, 0.9};
    int area = processor.calculateArea(box);
    EXPECT_EQ(area, 5000);
}

TEST(ShapeProcessorTest, CalculateDiagonal) {
    samples::ShapeProcessor processor;
    
    BoundingBox box = {0, 0, 3, 4, 0.9};
    double diagonal = processor.calculateDiagonal(box);
    EXPECT_DOUBLE_EQ(diagonal, 5.0);
}

TEST(ShapeProcessorTest, Translate) {
    samples::ShapeProcessor processor;
    
    Point p = {10, 20};
    Point result = processor.translate(p, 5, -3);
    EXPECT_EQ(result.x, 15);
    EXPECT_EQ(result.y, 17);
}

TEST(ShapeProcessorTest, CreateBox) {
    samples::ShapeProcessor processor;
    
    BoundingBox box = processor.createBox(10, 20, 100, 200);
    EXPECT_EQ(box.x, 10);
    EXPECT_EQ(box.y, 20);
    EXPECT_EQ(box.width, 100);
    EXPECT_EQ(box.height, 200);
    EXPECT_DOUBLE_EQ(box.confidence, 1.0);
}

TEST(ShapeProcessorTest, BoxContainsPoint) {
    samples::ShapeProcessor processor;
    
    BoundingBox box = {10, 10, 100, 100, 0.9};
    Point inside = {50, 50};
    Point outside = {5, 5};
    Point edge = {10, 10};
    
    EXPECT_TRUE(processor.boxContainsPoint(box, inside));
    EXPECT_FALSE(processor.boxContainsPoint(box, outside));
    EXPECT_TRUE(processor.boxContainsPoint(box, edge));
}

TEST(ShapeProcessorTest, CAPIStructParameters) {
    ShapeProcessorPtr processor(ShapeProcessor_create());
    ASSERT_NE(processor, nullptr);
    
    BoundingBox box = {0, 0, 10, 20, 0.5};
    int area = ShapeProcessor_calculateArea(processor.get(), box);
    EXPECT_EQ(area, 200);
    
    Point p = {5, 10};
    Point translated = ShapeProcessor_translate(processor.get(), p, 3, 4);
    EXPECT_EQ(translated.x, 8);
    EXPECT_EQ(translated.y, 14);
    
    BoundingBox created = ShapeProcessor_createBox(processor.get(), 1, 2, 3, 4);
    EXPECT_EQ(created.x, 1);
    EXPECT_EQ(created.y, 2);
    EXPECT_EQ(created.width, 3);
    EXPECT_EQ(created.height, 4);
    
    BoundingBox testBox = {0, 0, 100, 100, 1.0};
    Point testPoint = {50, 50};
    EXPECT_TRUE(ShapeProcessor_boxContainsPoint(processor.get(), testBox, testPoint));
}

// ============================================================================
// AsyncProcessor Tests (Callbacks)
// ============================================================================

TEST(AsyncProcessorTest, ProcessWithProgress) {
    samples::AsyncProcessor processor;
    
    std::vector<std::pair<int, int>> progressCalls;
    int result = processor.processWithProgress(5, [&](int current, int total) {
        progressCalls.push_back({current, total});
    });
    
    EXPECT_EQ(result, 5);
    ASSERT_EQ(progressCalls.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(progressCalls[i].first, i);
        EXPECT_EQ(progressCalls[i].second, 5);
    }
}

TEST(AsyncProcessorTest, CountFiltered) {
    samples::AsyncProcessor processor;
    
    int evenCount = processor.countFiltered(1, 10, [](int value) {
        return value % 2 == 0;
    });
    EXPECT_EQ(evenCount, 5);
}

TEST(AsyncProcessorTest, SumTransformed) {
    samples::AsyncProcessor processor;
    
    int sumSquares = processor.sumTransformed(1, 5, [](int value) {
        return value * value;
    });
    EXPECT_EQ(sumSquares, 55);
}

TEST(AsyncProcessorTest, CAPICallbacks) {
    AsyncProcessorPtr processor(AsyncProcessor_create());
    ASSERT_NE(processor, nullptr);
    
    static std::vector<std::pair<int, int>> s_progressCalls;
    s_progressCalls.clear();
    
    int result = AsyncProcessor_processWithProgress(processor.get(), 3, [](int current, int total) {
        s_progressCalls.push_back({current, total});
    });
    
    EXPECT_EQ(result, 3);
    ASSERT_EQ(s_progressCalls.size(), 3u);
    
    int countGtFive = AsyncProcessor_countFiltered(processor.get(), 1, 10, [](int value) -> int {
        return value > 5 ? 1 : 0;
    });
    EXPECT_EQ(countGtFive, 5);
    
    int sumDoubled = AsyncProcessor_sumTransformed(processor.get(), 1, 3, [](int value) -> int {
        return value * 2;
    });
    EXPECT_EQ(sumDoubled, 12);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
