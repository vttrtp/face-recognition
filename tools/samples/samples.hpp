#pragma once

#include "samples_c_api.h"

#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace samples {

// Callback type aliases for C++ usage
using ProgressCallback = std::function<void(int current, int total)>;
using FilterCallback = std::function<bool(int value)>;
using TransformCallback = std::function<int(int value)>;

/**
 * @brief Simple calculator for testing numeric types
 */
class SAMPLES_API Calculator {
public:
    Calculator() = default;

    [[nodiscard]] int add(int a, int b) const { return a + b; }
    [[nodiscard]] int subtract(int a, int b) const { return a - b; }
    [[nodiscard]] int multiply(int a, int b) const { return a * b; }
    [[nodiscard]] double divide(double a, double b) const { return b != 0.0 ? a / b : 0.0; }

    [[nodiscard]] int getTotal() const { return total_; }

    [[nodiscard]] int getVersionMajor() const { return 1; }
    [[nodiscard]] int getVersionMinor() const { return 0; }

private:
    int total_ = 0;
};

/**
 * @brief Geometry helper for testing vector returns
 */
class SAMPLES_API Geometry {
public:
    Geometry() = default;

    [[nodiscard]] std::vector<Point> createLine(int x1, int y1, int x2, int y2, int numPoints) {
        std::vector<Point> points;
        if (numPoints <= 0) {
            lastCount_ = 0;
            return points;
        }
        
        points.reserve(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            double t = (numPoints == 1) ? 0.0 : static_cast<double>(i) / (numPoints - 1);
            Point p;
            p.x = static_cast<int>(x1 + t * (x2 - x1));
            p.y = static_cast<int>(y1 + t * (y2 - y1));
            points.push_back(p);
        }
        
        lastCount_ = static_cast<int>(points.size());
        return points;
    }

    [[nodiscard]] std::vector<BoundingBox> findBoundingBoxes(int count) {
        std::vector<BoundingBox> boxes;
        if (count <= 0) {
            lastCount_ = 0;
            return boxes;
        }
        
        boxes.reserve(count);
        for (int i = 0; i < count; ++i) {
            BoundingBox box;
            box.x = i * 10;
            box.y = i * 10;
            box.width = 50 + i;
            box.height = 50 + i;
            box.confidence = 0.9 - (i * 0.1);
            boxes.push_back(box);
        }
        
        lastCount_ = static_cast<int>(boxes.size());
        return boxes;
    }

    [[nodiscard]] int getLastCount() const { return lastCount_; }

private:
    int lastCount_ = 0;
};

/**
 * @brief Shape processor for testing struct parameters
 */
class SAMPLES_API ShapeProcessor {
public:
    ShapeProcessor() = default;

    [[nodiscard]] int calculateArea(BoundingBox box) const {
        return box.width * box.height;
    }

    [[nodiscard]] double calculateDiagonal(const BoundingBox& box) const {
        return std::sqrt(static_cast<double>(box.width * box.width + box.height * box.height));
    }

    [[nodiscard]] Point translate(Point p, int dx, int dy) const {
        return {p.x + dx, p.y + dy};
    }

    [[nodiscard]] int distanceFromOrigin(const Point& p) const {
        return static_cast<int>(std::sqrt(static_cast<double>(p.x * p.x + p.y * p.y)));
    }

    [[nodiscard]] bool boxContainsPoint(const BoundingBox& box, const Point& point) const {
        return point.x >= box.x && point.x < box.x + box.width &&
               point.y >= box.y && point.y < box.y + box.height;
    }

    [[nodiscard]] BoundingBox createBox(int x, int y, int width, int height) const {
        return {x, y, width, height, 1.0};
    }
};

/**
 * @brief Async processor for testing callback support
 */
class SAMPLES_API AsyncProcessor {
public:
    AsyncProcessor() = default;

    [[nodiscard]] int processWithProgress(int count, ProgressCallback onProgress) {
        for (int i = 0; i < count; ++i) {
            onProgress(i, count);
        }
        return count;
    }

    [[nodiscard]] int countFiltered(int start, int end, FilterCallback filter) {
        int count = 0;
        for (int i = start; i <= end; ++i) {
            if (filter(i)) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] int sumTransformed(int start, int end, TransformCallback transform) {
        int sum = 0;
        for (int i = start; i <= end; ++i) {
            sum += transform(i);
        }
        return sum;
    }
};

} // namespace samples
