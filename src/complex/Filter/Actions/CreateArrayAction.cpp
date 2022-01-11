#include "CreateArrayAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateArrayAction::CreateArrayAction(NumericType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path)
: m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_Path(path)
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Validate the Numeric Type
  switch(m_Type)
  {
  case NumericType::int8: {
    return CreateArray<int8>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::uint8: {
    return CreateArray<uint8>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::int16: {
    return CreateArray<int16>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::uint16: {
    return CreateArray<uint16>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::int32: {
    return CreateArray<int32>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::uint32: {
    return CreateArray<uint32>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::int64: {
    return CreateArray<int64>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::uint64: {
    return CreateArray<uint64>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::float32: {
    return CreateArray<float32>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  case NumericType::float64: {
    return CreateArray<float64>(dataStructure, m_Dims, m_CDims, m_Path, mode);
  }
  default:
    throw std::runtime_error(fmt::format("CreateArrayAction: Invalid Numeric Type '{}'", static_cast<int8>(m_Type)));
  }
}

NumericType CreateArrayAction::type() const
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

const DataPath& CreateArrayAction::path() const
{
  return m_Path;
}
} // namespace complex
