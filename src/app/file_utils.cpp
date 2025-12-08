#include "file_utils.hpp"

#include <algorithm>
#include <iostream>

namespace app {

namespace fs = std::filesystem;

std::vector<fs::path> findFiles(
    const fs::path& dir,
    const std::vector<std::string>& extensions) {
    
    std::vector<fs::path> files;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                files.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[findFiles] Filesystem error: " << e.what() << std::endl;
    }

    return files;
}

} // namespace app
