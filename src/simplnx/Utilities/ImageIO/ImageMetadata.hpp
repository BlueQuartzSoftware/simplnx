#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"

#include <optional>

namespace nx::core
{

/**
 * @struct ImageMetadata
 * @brief Holds metadata extracted from an image file without loading pixel data.
 *
 * The origin and spacing fields are std::optional to distinguish between
 * "the file contained this value" and "the file had no such metadata."
 * This allows callers to decide whether to use file values or user-provided overrides.
 */
struct SIMPLNX_EXPORT ImageMetadata
{
  usize width = 0;                      ///< X dimension in pixels
  usize height = 0;                     ///< Y dimension in pixels
  usize num_components = 0;             ///< 1=grayscale, 3=RGB, 4=RGBA
  DataType data_type = DataType::uint8; ///< Pixel data type (uint8, uint16, or float32)
  usize num_pages = 1;                  ///< Number of pages (>1 for multi-page TIFF)
  std::optional<FloatVec3Type> origin;  ///< Image origin if stored in file
  std::optional<FloatVec3Type> spacing; ///< Image spacing/resolution if stored in file
};

} // namespace nx::core
