#include "CreateNeighborListAction.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
{
CreateNeighborListAction::CreateNeighborListAction(DataType type, usize tupleCount, const DataPath& path)
: IDataCreationAction(path)
, m_Type(type)
, m_TupleCount(tupleCount)
{
}

CreateNeighborListAction::~CreateNeighborListAction() noexcept = default;

Result<> CreateNeighborListAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Validate the Numeric Type
  switch(m_Type)
  {
  case DataType::int8: {
    return CreateNeighbors<int8>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::uint8: {
    return CreateNeighbors<uint8>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::int16: {
    return CreateNeighbors<int16>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::uint16: {
    return CreateNeighbors<uint16>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::int32: {
    return CreateNeighbors<int32>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::uint32: {
    return CreateNeighbors<uint32>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::int64: {
    return CreateNeighbors<int64>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::uint64: {
    return CreateNeighbors<uint64>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::float32: {
    return CreateNeighbors<float32>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  case DataType::float64: {
    return CreateNeighbors<float64>(dataStructure, m_TupleCount, getCreatedPath(), mode);
  }
  default:
    throw std::runtime_error(fmt::format("CreateNeighborListAction: Invalid Numeric Type '{}'", to_underlying(m_Type)));
  }
}

IDataAction::UniquePointer CreateNeighborListAction::clone() const
{
  return std::make_unique<CreateNeighborListAction>(m_Type, m_TupleCount, getCreatedPath());
}

DataType CreateNeighborListAction::type() const
{
  return m_Type;
}

usize CreateNeighborListAction::tupleCount() const
{
  return m_TupleCount;
}

DataPath CreateNeighborListAction::path() const
{
  return getCreatedPath();
}

std::vector<DataPath> CreateNeighborListAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace nx::core
