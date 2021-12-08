#include "CreateNeighborListAction.hpp"

#include <fmt/core.h>

#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateNeighborListAction::CreateNeighborListAction(NumericType type, usize tupleCount, const DataPath& path)
: m_Type(type)
, m_TupleCount(tupleCount)
, m_Path(path)
{
}

CreateNeighborListAction::~CreateNeighborListAction() noexcept = default;

Result<> CreateNeighborListAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Validate the Numeric Type
  switch(m_Type)
  {
  case NumericType::int8: {
    return CreateNeighbors<int8>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::uint8: {
    return CreateNeighbors<uint8>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::int16: {
    return CreateNeighbors<int16>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::uint16: {
    return CreateNeighbors<uint16>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::int32: {
    return CreateNeighbors<int32>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::uint32: {
    return CreateNeighbors<uint32>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::int64: {
    return CreateNeighbors<int64>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::uint64: {
    return CreateNeighbors<uint64>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::float32: {
    return CreateNeighbors<float32>(dataStructure, m_TupleCount, m_Path, mode);
  }
  case NumericType::float64: {
    return CreateNeighbors<float64>(dataStructure, m_TupleCount, m_Path, mode);
  }
  default:
    throw std::runtime_error(fmt::format("CreateNeighborListAction: Invalid Numeric Type '{}'", m_Type));
  }
}

NumericType CreateNeighborListAction::type() const
{
  return m_Type;
}

usize CreateNeighborListAction::tupleCount() const
{
  return m_TupleCount;
}

DataPath CreateNeighborListAction::path() const
{
  return m_Path;
}
} // namespace complex
