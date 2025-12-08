#ifndef FACE_DETECTOR_INTERFACE_H
#define FACE_DETECTOR_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #ifdef FACE_DETECTOR_EXPORTS
        #define FACE_DETECTOR_API __declspec(dllexport)
    #else
        #define FACE_DETECTOR_API __declspec(dllimport)
    #endif
#else
    #define FACE_DETECTOR_API __attribute__((visibility("default")))
#endif

/**
 * @brief Rectangle representing detected face bounds
 */
typedef struct FaceRect {
    int x;
    int y;
    int width;
    int height;
} FaceRect;

/**
 * @brief Opaque handle to detection result
 */
typedef struct DetectionResult DetectionResult;

/**
 * @brief Create a face detector instance
 * @param cascade_path Path to Haar cascade XML file
 * @return Opaque pointer to detector, or NULL on failure
 */
FACE_DETECTOR_API void* create_detector(const char* cascade_path);

/**
 * @brief Destroy a face detector instance
 * @param detector Pointer obtained from create_detector
 */
FACE_DETECTOR_API void destroy_detector(void* detector);

/**
 * @brief Detect faces in an image file
 * @param detector Pointer to detector
 * @param image_path Path to image file
 * @return Opaque pointer to detection result, or NULL on error.
 *         Must be freed with free_detection_result when no longer needed.
 */
FACE_DETECTOR_API DetectionResult* detect_faces(void* detector, const char* image_path);

/**
 * @brief Get the number of faces in detection result
 * @param result Pointer to detection result
 * @return Number of faces, or -1 if result is NULL
 */
FACE_DETECTOR_API int get_faces_count(const DetectionResult* result);

/**
 * @brief Get pointer to face rectangles array
 * @param result Pointer to detection result
 * @return Pointer to internal FaceRect array, or NULL if result is NULL/empty.
 *         The pointer is valid as long as the DetectionResult is not freed.
 */
FACE_DETECTOR_API const FaceRect* get_faces_data(const DetectionResult* result);

/**
 * @brief Free detection result
 * @param result Pointer to detection result returned by detect_faces
 */
FACE_DETECTOR_API void free_detection_result(DetectionResult* result);

#ifdef __cplusplus
}
#endif

#endif // FACE_DETECTOR_INTERFACE_H
