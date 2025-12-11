// X-macro definitions for face_detector library symbols
// Format: SYMBOL(member_name, function_type, c_symbol_name)

SYMBOL(createDetector,      void*(const char*),                         "create_detector")
SYMBOL(destroyDetector,     void(void*),                                "destroy_detector")
SYMBOL(detectFaces,         DetectionResult*(void*, const char*),       "detect_faces")
SYMBOL(getFacesCount,       int(const DetectionResult*),                "get_faces_count")
SYMBOL(getFacesData,        const FaceRect*(const DetectionResult*),    "get_faces_data")
SYMBOL(freeDetectionResult, void(DetectionResult*),                     "free_detection_result")
