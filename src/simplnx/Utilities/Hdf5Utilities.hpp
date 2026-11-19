#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <vector>

namespace nx::core::HDF5
{
/**
 * @brief Recursively collects the absolute HDF5 paths (e.g. "/Group/SubGroup") of
 * every group contained in the file. The root group itself is not included.
 * Returns an empty vector if the FileIO is invalid.
 * @param fileIO
 * @return std::vector<std::string>
 */
SIMPLNX_EXPORT std::vector<std::string> getGroupPaths(const FileIO& fileIO);

/**
 * @brief Recursively collects the absolute HDF5 paths (e.g. "/Group/Dataset") of
 * every dataset contained in the file.
 * Returns an empty vector if the FileIO is invalid.
 * @param fileIO
 * @return std::vector<std::string>
 */
SIMPLNX_EXPORT std::vector<std::string> getDatasetPaths(const FileIO& fileIO);
} // namespace nx::core::HDF5
