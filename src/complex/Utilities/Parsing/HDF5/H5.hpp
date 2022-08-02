#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/Types.hpp"

#include <cstdint>
#include <string>
#include <string_view>

#include "complex/complex_export.hpp"

namespace complex
{
namespace H5
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
 * @brief Returns a Type enum representing the corresponding type matching the
 * specified HDF5 ID.
 * @param typeId
 * @return H5::Type
 */
Type COMPLEX_EXPORT getTypeFromId(IdType typeId);

/**
 * @brief Returns the HDF5 type for the specified H5::Type enum.
 * @param type
 * @return H5::IdType
 */
H5::IdType COMPLEX_EXPORT getIdForType(Type type);

/**
 * @brief Returns a std::string representation from the provided view
 * buffer. If the buffer contains segments divided by '/', only the last
 * segment is returned.
 * @param buffer
 * @return std::string
 */
std::string COMPLEX_EXPORT GetNameFromBuffer(std::string_view buffer);

/**
 * @brief Returns a std::string of a valid path to the object with the given id.
 * Uses H5Iget_name internally.
 * @param id
 * @return std::string
 */
std::string COMPLEX_EXPORT GetPathFromId(IdType id);

/**
 * @brief Returns a std::string of the name of the given object.
 * Equivalent to GetNameFromBuffer(GetPathFromId(id)).
 * @param id
 * @return std::string
 */
std::string COMPLEX_EXPORT GetNameFromId(IdType id);

/**
 * @brief Returns the path to an object's parent
 * @param objectPath The HDF5 path to the object
 * @return  The path to the object's parent
 */
std::string COMPLEX_EXPORT GetParentPath(const std::string& objectPath);

inline constexpr StringLiteral k_DataTypeTag = "DataType";

inline constexpr StringLiteral k_DataStoreTag = "DataStore";
inline constexpr StringLiteral k_TupleShapeTag = "TupleDimensions";
inline constexpr StringLiteral k_ComponentShapeTag = "ComponentDimensions";
inline constexpr StringLiteral k_DataObjectIdTag = "DataObjectId";
inline constexpr StringLiteral k_DataArrayTag = "DataArray";

inline constexpr StringLiteral k_ObjectTypeTag = "ObjectType";
inline constexpr StringLiteral k_DataStructureTag = "DataStructure";

inline constexpr StringLiteral k_VertexListIdTag = "VertexListId";
inline constexpr StringLiteral k_SharedVertexListTag = "SharedVertexList";
inline constexpr StringLiteral k_VertexSizesIdTag = "VertexSizesId";

} // namespace H5
} // namespace complex
