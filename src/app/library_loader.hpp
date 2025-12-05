#ifndef LIBRARY_LOADER_HPP
#define LIBRARY_LOADER_HPP

#include <string>
#include <memory>
#include <vector>

#include "face_detector.hpp"

namespace app {

/**
 * @brief Dynamic library loader for face_detector
 * 
 * Loads the face_detector shared library at runtime and provides
 * access to its functions through the C API.
 */
class LibraryLoader {
public:
    /**
     * @brief Construct loader and load the library
     * @param library_path Path to the shared library file
     */
    explicit LibraryLoader(const std::string& library_path);
    
    ~LibraryLoader();
    
    // Non-copyable
    LibraryLoader(const LibraryLoader&) = delete;
    LibraryLoader& operator=(const LibraryLoader&) = delete;
    
    // Movable
    LibraryLoader(LibraryLoader&&) noexcept;
    LibraryLoader& operator=(LibraryLoader&&) noexcept;

    /**
     * @brief Check if library was loaded successfully
     */
    bool isLoaded() const;

    /**
     * @brief Create a face detector instance
     * @param cascade_path Path to Haar cascade XML file
     * @return Opaque pointer to detector, or nullptr on failure
     */
    void* createDetector(const std::string& cascade_path);

    /**
     * @brief Destroy a face detector instance
     * @param detector Pointer obtained from createDetector
     */
    void destroyDetector(void* detector);

    /**
     * @brief Check if detector is properly loaded
     * @param detector Pointer to detector
     * @return true if detector is ready
     */
    bool isDetectorLoaded(void* detector);

    /**
     * @brief Detect faces in an image
     * @param detector Pointer to detector
     * @param image_path Path to image file
     * @param out_faces Output buffer for face rectangles
     * @param max_faces Maximum number of faces to return
     * @return Number of faces found, or -1 on error
     */
    int detectFaces(void* detector, const std::string& image_path,
                    face_detector::FaceRect* out_faces, int max_faces);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace app

#endif // LIBRARY_LOADER_HPP
