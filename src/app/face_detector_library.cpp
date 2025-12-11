#include "face_detector_library.hpp"

#include <iostream>

namespace app {

bool FaceDetectorLibrary::loadSymbols() {
    // Load symbols using X-macro
    #define SYMBOL(name, type, str) \
        name = reinterpret_cast<std::add_pointer_t<type>>(getSymbol(str));
    #include "face_detector_symbols.inc"
    #undef SYMBOL

    // Verify all symbols loaded
    bool all_loaded = true
    #define SYMBOL(name, type, str) && name
    #include "face_detector_symbols.inc"
    #undef SYMBOL
    ;

    if (!all_loaded) {
        std::cerr << "[" << kLogPrefix << "] Failed to load all required symbols" << std::endl;
    }
    return all_loaded;
}

} // namespace app
