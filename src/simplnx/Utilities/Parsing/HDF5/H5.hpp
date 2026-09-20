#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Tpublic.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace nx::core::HDF5
{
using IdType = int64;
using ErrorType = int32;
using SizeType = uint64;

/**
 * @enum Type
 * @brief Identifies numeric and string types in the HDF5 utility layer.
 */
enum class Type
{
  int8,         ///< Selects signed 8-bit integers.
  int16,        ///< Selects signed 16-bit integers.
  int32,        ///< Selects signed 32-bit integers.
  int64,        ///< Selects signed 64-bit integers.
  uint8,        ///< Selects unsigned 8-bit integers.
  uint16,       ///< Selects unsigned 16-bit integers.
  uint32,       ///< Selects unsigned 32-bit integers.
  uint64,       ///< Selects unsigned 64-bit integers.
  float32,      ///< Selects 32-bit floating-point values.
  float64,      ///< Selects 64-bit floating-point values.
  string,       ///< Selects string values.
  unknown = 255 ///< Identifies an unsupported or invalid type.
};

/**
 * @brief Converts an HDF5 utility type to a common numeric DataType.
 * @param typeEnum Specifies the HDF5 utility type.
 * @return Matching numeric type, or std::nullopt for string and unknown.
 */
std::optional<DataType> SIMPLNX_EXPORT toCommonType(Type typeEnum);

// Reserved HighFive conversion overload.
// std::optional<nx::core::DataType> SIMPLNX_EXPORT toCommonType(HighFive::DataType typeEnum);

/**
 * @brief Identifies a native numeric HDF5 type.
 * @param typeId Supplies a valid HDF5 datatype identifier.
 * @return Matching numeric Type, or unknown for invalid and unsupported types.
 * @pre The caller does not hold Support::ApiLock().
 *
 * The function gets the non-recursive process-wide HDF5 API lock. It does not
 * classify HDF5 string types.
 */
Type SIMPLNX_EXPORT getTypeFromId(IdType typeId);

/**
 * @brief Gets the native HDF5 identifier for a numeric utility type.
 * @param type Specifies the numeric utility type.
 * @return Native HDF5 datatype identifier.
 * @throws std::runtime_error If type is string, unknown, or otherwise unsupported.
 */
IdType SIMPLNX_EXPORT getIdForType(Type type);

/**
 * @brief Gets the final name segment from an HDF5 path buffer.
 * @param buffer Supplies a nonempty HDF5 path or name.
 * @return Final segment, or "/" for the root path.
 * @throws std::runtime_error If buffer is empty or a non-root path ends with a slash.
 */
std::string SIMPLNX_EXPORT GetNameFromBuffer(std::string_view buffer);

/**
 * @brief Gets the complete HDF5 path for an object identifier.
 * @param id Supplies a valid named HDF5 object identifier.
 * @return Complete object path from H5Iget_name().
 * @throws std::runtime_error If HDF5 cannot get the path.
 * @pre The caller does not hold Support::ApiLock().
 *
 * The function gets the non-recursive process-wide HDF5 API lock.
 */
std::string SIMPLNX_EXPORT GetPathFromId(IdType id);

/**
 * @brief Gets the final name segment for an HDF5 object identifier.
 * @param id Supplies a valid named HDF5 object identifier.
 * @return Final path segment.
 * @throws std::runtime_error If the object path or name is invalid.
 * @pre The caller does not hold Support::ApiLock().
 */
std::string SIMPLNX_EXPORT GetNameFromId(IdType id);

/**
 * @brief Removes trailing slash characters from a path.
 * @param objectPath Supplies the HDF5 path.
 * @return Path without trailing slashes, or an empty string for empty and root-only input.
 *
 * Despite its historical name, this function does not remove the final name segment.
 */
std::string SIMPLNX_EXPORT GetParentPath(const std::string& objectPath);

} // namespace nx::core::HDF5
