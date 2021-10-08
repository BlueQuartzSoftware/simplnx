#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Common/StringLiteral.hpp"

#include <cstdint>
#include <string>

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
 * @brief Returns a std::string representation from the provided const char*
 * buffer. If the buffer contains segments divided by '/', only the last
 * segment is returned.
 * @param buffer
 * @return std::string
 */
std::string COMPLEX_EXPORT GetNameFromBuffer(const char* buffer);

static constexpr StringLiteral k_DataTypeTag = "DataType";

static constexpr StringLiteral k_DataStoreTag = "DataStore";
static constexpr StringLiteral k_TupleShapeTag = "TupleShape";
static constexpr StringLiteral k_ComponentShapeTag = "ComponentShape";
static constexpr StringLiteral k_DataObjectIdTag = "DataObjectId";
static constexpr StringLiteral k_DataArrayTag = "DataArray";

static constexpr StringLiteral k_ObjectTypeTag = "ObjectType";
static constexpr StringLiteral k_DataStructureTag = "DataStructure";

static constexpr StringLiteral k_VertexListIdTag = "VertexListId";
static constexpr StringLiteral k_SharedVertexListTag = "SharedVertexList";
static constexpr StringLiteral k_VertexSizesIdTag = "VertexSizesId";

} // namespace H5
} // namespace complex
