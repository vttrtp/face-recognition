#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace app {

/**
 * @brief Find files with specified extensions recursively
 * @param dir Directory to search
 * @param extensions Vector of file extensions to match (e.g., {".jpg", ".png"})
 * @return Vector of matching file paths
 */
std::vector<std::filesystem::path> findFiles(
    const std::filesystem::path& dir,
    const std::vector<std::string>& extensions);

} // namespace app
