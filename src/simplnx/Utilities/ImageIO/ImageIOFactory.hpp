#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"

#include <filesystem>
#include <memory>

namespace nx::core
{

/**
 * @brief Creates the appropriate IImageIO backend based on file extension.
 *
 * Supported extensions:
 *   .png, .jpg, .jpeg, .bmp -> StbImageIO
 *   .tif, .tiff -> TiffImageIO
 *
 * @param filePath File path (only extension is examined)
 * @return unique_ptr<IImageIO> on success, or error Result for unsupported extensions
 */
SIMPLNX_EXPORT Result<std::unique_ptr<IImageIO>> CreateImageIO(const std::filesystem::path& filePath);

} // namespace nx::core
