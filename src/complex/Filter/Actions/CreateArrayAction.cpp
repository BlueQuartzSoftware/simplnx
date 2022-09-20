#include "CreateArrayAction.hpp"

#include <fmt/core.h>

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateArrayAction::CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, bool inMemory)
: IDataCreationAction(path)
, m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_InMemory(inMemory)
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  switch(m_Type)
  {
  case DataType::int8: {
    return CreateArray<int8>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::uint8: {
    return CreateArray<uint8>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::int16: {
    return CreateArray<int16>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::uint16: {
    return CreateArray<uint16>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::int32: {
    return CreateArray<int32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::uint32: {
    return CreateArray<uint32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::int64: {
    return CreateArray<int64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::uint64: {
    return CreateArray<uint64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::float32: {
    return CreateArray<float32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::float64: {
    return CreateArray<float64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  case DataType::boolean: {
    return CreateArray<bool>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_InMemory);
  }
  default: {
    throw std::runtime_error(fmt::format("CreateArrayAction: Invalid DataType '{}'", to_underlying(m_Type)));
  }
  }
}

DataType CreateArrayAction::type() const
{
  return m_Type;
}

const std::vector<usize>& CreateArrayAction::dims() const
{
  return m_Dims;
}

const std::vector<usize>& CreateArrayAction::componentDims() const
{
  return m_CDims;
}

DataPath CreateArrayAction::path() const
{
  return getCreatedPath();
}

std::vector<DataPath> CreateArrayAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}

bool CreateArrayAction::inMemory() const
{
  return m_InMemory;
}
} // namespace complex
