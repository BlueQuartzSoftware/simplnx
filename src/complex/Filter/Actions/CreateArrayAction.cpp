#include "CreateArrayAction.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace complex;

namespace complex
{
CreateArrayAction::CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat)
: IDataCreationAction(path)
, m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_DataFormat(dataFormat)
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  switch(m_Type)
  {
  case DataType::int8: {
    return CreateArray<int8>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::uint8: {
    return CreateArray<uint8>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::int16: {
    return CreateArray<int16>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::uint16: {
    return CreateArray<uint16>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::int32: {
    return CreateArray<int32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::uint32: {
    return CreateArray<uint32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::int64: {
    return CreateArray<int64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::uint64: {
    return CreateArray<uint64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::float32: {
    return CreateArray<float32>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::float64: {
    return CreateArray<float64>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  case DataType::boolean: {
    return CreateArray<bool>(dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat);
  }
  default: {
    static constexpr StringLiteral prefix = "CreateArrayAction: ";
    throw std::runtime_error(fmt::format("{}CreateArrayAction: Invalid DataType '{}'", prefix, to_underlying(m_Type)));
  }
  }
}

IDataAction::UniquePointer CreateArrayAction::clone() const
{
  return std::make_unique<CreateArrayAction>(m_Type, m_Dims, m_CDims, getCreatedPath());
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

std::string CreateArrayAction::dataFormat() const
{
  return m_DataFormat;
}
} // namespace complex
