#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace nx::core::HDF5
{
using IdType = int64;
using ErrorType = int32;
using SizeType = unsigned long long;

enum class Type
{
  int8,
  int16,
  int32,
  int64,
  uint8,
  uint16,
  uint32,
  uint64,
  float32,
  float64,
  string,
  unknown = 255
};

/**
 * @brief converts an H5Support enum Type to  enum type.
 * @param typeEnum
 * @return DataType
 */
std::optional<DataType> SIMPLNX_EXPORT toCommonType(Type typeEnum);

/**
 * @brief Returns a Type enum representing the corresponding type matching the
 * specified HDF5 ID.
 * @param typeId
 * @return Type
 */
Type SIMPLNX_EXPORT getTypeFromId(IdType typeId);

/**
 * @brief Returns the HDF5 type for the specified Type enum.
 * @param type
 * @return IdType
 */
IdType SIMPLNX_EXPORT getIdForType(Type type);

/**
 * @brief Returns a std::string representation from the provided view
 * buffer. If the buffer contains segments divided by '/', only the last
 * segment is returned.
 * @param buffer
 * @return std::string
 */
std::string SIMPLNX_EXPORT GetNameFromBuffer(std::string_view buffer);

/**
 * @brief Returns a std::string of a valid path to the object with the given id.
 * Uses H5Iget_name internally.
 * @param id
 * @return std::string
 */
std::string SIMPLNX_EXPORT GetPathFromId(IdType id);

/**
 * @brief Returns a std::string of the name of the given object.
 * Equivalent to GetNameFromBuffer(GetPathFromId(id)).
 * @param id
 * @return std::string
 */
std::string SIMPLNX_EXPORT GetNameFromId(IdType id);

/**
 * @brief Returns the path to an object's parent
 * @param objectPath The HDF5 path to the object
 * @return  The path to the object's parent
 */
std::string SIMPLNX_EXPORT GetParentPath(const std::string& objectPath);

} // namespace nx::core::HDF5
