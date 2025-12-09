#include "library_loader.hpp"
#include "face_detector_wrapper.hpp"
#include "image_processor.hpp"
#include "file_utils.hpp"

#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <input_directory> [options]\n"
              << "\n"
              << "Arguments:\n"
              << "  input_directory    Path to directory containing images (recursive)\n"
              << "\n"
              << "Options:\n"
              << "  --library <path>   Path to face_detector library (default: auto-detect)\n"
              << "  --cascade <path>   Path to Haar cascade XML file (default: data/haarcascade_frontalface_default.xml)\n"
              << "  --output <path>    Output directory for result images (default: input_directory)\n"
              << "  --help             Show this help message\n"
              << std::endl;
}

std::string findLibrary(const fs::path& exe_dir) {
    // Platform-specific library names
#ifdef _WIN32
    const std::string lib_name = "face_detector.dll";
#elif __APPLE__
    const std::string lib_name = "libface_detector.dylib";
#else
    const std::string lib_name = "libface_detector.so";
#endif

    // Search locations
    std::vector<fs::path> search_paths = {
        exe_dir,
        exe_dir / "lib",
        exe_dir / ".."/  "lib",
        fs::current_path(),
        fs::current_path() / "lib"
    };

    auto result = app::findInPaths(lib_name, search_paths);
    return result.empty() ? lib_name : result;  // Fall back to system library path
}

std::string findCascade(const fs::path& exe_dir) {
    const std::string cascade_name = "haarcascade_frontalface_default.xml";

    std::vector<fs::path> search_paths = {
        exe_dir / "data",
        exe_dir,
        exe_dir / ".." / "data",
        fs::current_path() / "data",
        fs::current_path(),
        "/usr/share/opencv4/haarcascades",
        "/usr/local/share/opencv4/haarcascades"
    };

    return app::findInPaths(cascade_name, search_paths);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Get executable directory
    fs::path exe_path = fs::canonical(argv[0]);
    fs::path exe_dir = exe_path.parent_path();

    // Parse arguments
    std::string input_dir;
    std::string library_path;
    std::string cascade_path;
    std::string output_dir;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--library" && i + 1 < argc) {
            library_path = argv[++i];
        } else if (arg == "--cascade" && i + 1 < argc) {
            cascade_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg[0] != '-') {
            input_dir = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    if (input_dir.empty()) {
        std::cerr << "Error: input directory is required" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Validate input directory
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Error: invalid input directory: " << input_dir << std::endl;
        return 1;
    }

    // Auto-detect library path if not specified
    if (library_path.empty()) {
        library_path = findLibrary(exe_dir);
    }

    // Auto-detect cascade path if not specified
    if (cascade_path.empty()) {
        cascade_path = findCascade(exe_dir);
        if (cascade_path.empty()) {
            std::cerr << "Error: Haar cascade file not found. "
                      << "Please specify with --cascade option" << std::endl;
            return 1;
        }
    }

    std::cout << "=== Face Recognition Application ===" << std::endl;
    std::cout << "Input directory: " << input_dir << std::endl;
    std::cout << "Library: " << library_path << std::endl;
    std::cout << "Cascade: " << cascade_path << std::endl;
    std::cout << "Output directory: " << (output_dir.empty() ? input_dir : output_dir) << std::endl;
    std::cout << "====================================" << std::endl;

    // Load the library dynamically
    app::LibraryLoader loader(library_path);
    if (!loader.isLoaded()) {
        std::cerr << "Error: failed to load face_detector library" << std::endl;
        return 1;
    }

    // Create face detector wrapper
    app::FaceDetectorWrapper detector(loader, cascade_path);
    if (!detector.isReady()) {
        std::cerr << "Error: failed to initialize face detector" << std::endl;
        return 1;
    }

    // Create image processor
    app::ImageProcessor processor(detector);
    if (!processor.isReady()) {
        std::cerr << "Error: processor not ready" << std::endl;
        return 1;
    }

    // Process all images
    auto results = processor.processDirectory(input_dir, output_dir);

    // Generate JSON output path (save to output directory)
    fs::path output_path = output_dir.empty() ? fs::path(input_dir) : fs::path(output_dir);
    fs::path json_path = output_path / "result.json";

    // Save results to JSON
    if (!processor.saveResultsToJson(results, json_path.string())) {
        std::cerr << "Error: failed to save results to JSON" << std::endl;
        return 1;
    }

    // Print summary
    int total_faces = 0;
    int successful = 0;
    for (const auto& result : results) {
        if (result.success) {
            successful++;
            total_faces += result.detection.count();
        }
    }

    std::cout << "\n=== Processing Complete ===" << std::endl;
    std::cout << "Images processed: " << successful << "/" << results.size() << std::endl;
    std::cout << "Total faces found: " << total_faces << std::endl;
    std::cout << "Results saved to: " << json_path << std::endl;

    return 0;
}
