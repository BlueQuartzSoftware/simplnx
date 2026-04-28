#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>

#include <stdexcept>
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
 * @brief Converts the index from the image-reader filters' "Output Data Type"
 * ChoicesParameter into a concrete DataType. Choices are (0=uint8, 1=uint16, 2=uint32).
 * Throws std::runtime_error for any other value rather than silently coercing to uint8.
 */
inline DataType ChoiceToImageDataType(usize choice)
{
  switch(choice)
  {
  case 0:
    return DataType::uint8;
  case 1:
    return DataType::uint16;
  case 2:
    return DataType::uint32;
  default:
    throw std::runtime_error(fmt::format("nx::core::ChoiceToImageDataType: invalid choice {}", choice));
  }
}

/**
 * @brief Reverse of ChoiceToImageDataType — maps a DataType back to its index in the
 * "Output Data Type" ChoicesParameter. Throws std::runtime_error for any DataType
 * outside the supported set (uint8/uint16/uint32).
 */
inline usize ImageDataTypeToChoice(DataType type)
{
  switch(type)
  {
  case DataType::uint8:
    return 0;
  case DataType::uint16:
    return 1;
  case DataType::uint32:
    return 2;
  default:
    throw std::runtime_error(fmt::format("nx::core::ImageDataTypeToChoice: unsupported DataType {}", static_cast<int>(type)));
  }
}

} // namespace nx::core
