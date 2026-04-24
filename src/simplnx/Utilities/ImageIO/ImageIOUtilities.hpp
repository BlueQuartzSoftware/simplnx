#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace nx::core
{

/**
 * @brief Formats an integer as a fill-padded string with the requested total digit count.
 * Used by the image-stack writers to build per-slice filenames such as "slice_007.tif".
 */
template <typename T>
std::string CreateIndexString(T index, usize totalDigits, std::string_view fillCharacter)
{
  std::string formatString = fmt::format("{{:{}>{}}}", fillCharacter, totalDigits);
  return fmt::format(fmt::runtime(formatString), index);
}

/**
 * @brief Returns the size in bytes of a single scalar element for the DataTypes that
 * the image-IO backends (stb, libtiff) actually support: uint8, uint16, float32.
 * Returns 0 for any other DataType so callers can detect unsupported types.
 */
SIMPLNX_EXPORT usize BytesPerImageElement(DataType type);

/**
 * @brief Converts the index from the image-reader filters' "Output Data Type"
 * ChoicesParameter into a concrete DataType. Choices are (0=uint8, 1=uint16, 2=uint32).
 * Out-of-range inputs are coerced to uint8.
 */
SIMPLNX_EXPORT DataType ChoiceToImageDataType(usize choice);

/**
 * @brief Reverse of ChoiceToImageDataType — maps a DataType back to its index in the
 * "Output Data Type" ChoicesParameter. Any unsupported DataType is mapped to 0 (uint8).
 */
SIMPLNX_EXPORT usize ImageDataTypeToChoice(DataType type);

} // namespace nx::core
