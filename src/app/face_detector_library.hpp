#pragma once

#include <string>
#include <type_traits>

#include "library_loader.hpp"
#include "face_detector_interface.h"

namespace app {

/**
 * @brief Singleton dynamic library loader for face_detector
 * 
 * Loads the face_detector shared library at runtime and exposes
 * raw function pointers. Use FaceDetectorWrapper for a safe C++ wrapper.
 */
class FaceDetectorLibrary : public SingletonLibrary<FaceDetectorLibrary> {
    friend class SingletonLibrary<FaceDetectorLibrary>;
    
public:
    static constexpr const char* kLogPrefix = "FaceDetectorLibrary";

    ~FaceDetectorLibrary() override = default;

    // Function pointers to library symbols
    #define SYMBOL(name, type, str) std::add_pointer_t<type> name = nullptr;
    #include "face_detector_symbols.inc"
    #undef SYMBOL

private:
    FaceDetectorLibrary() = default;
    bool loadSymbols();
};

} // namespace app
