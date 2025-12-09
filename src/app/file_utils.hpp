#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace app {

/**
 * @brief Find a file in a list of search paths
 * @param filename The filename to search for
 * @param search_paths Vector of directories to search in
 * @return Canonical path to the file if found, empty string otherwise
 */
std::string findInPaths(
    const std::string& filename,
    const std::vector<std::filesystem::path>& search_paths);

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
