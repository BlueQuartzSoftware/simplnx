#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Common/StringLiteral.hpp"

#define H5_USE_110_API
#include <H5Opublic.h>
#include <hdf5.h>

#ifdef H5Support_USE_MUTEX
#include <mutex>
#define H5SUPPORT_MUTEX_LOCK()                                                                                                                                                                         \
  std::mutex mutex;                                                                                                                                                                                    \
  std::lock_guard<std::mutex> lock(mutex);
#else
#define H5SUPPORT_MUTEX_LOCK()
#endif

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

Type COMPLEX_EXPORT getTypeFromId(IdType typeId);
H5::IdType COMPLEX_EXPORT getIdForType(Type type);

inline const std::string DataTypeTag = "DataType";
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
