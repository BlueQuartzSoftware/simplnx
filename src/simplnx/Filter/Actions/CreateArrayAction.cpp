#include "CreateArrayAction.hpp"

#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct CreateArrayFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, IDataAction::Mode mode, std::string dataFormat,
                      std::string fillValue)
  {
    return ArrayCreationUtilities::CreateArray<T>(dataStructure, tDims, cDims, path, mode, dataFormat, fillValue);
  }
};
} // namespace

namespace nx::core
{
CreateArrayAction::CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat, std::string fillValue)
: AbstractDataCreationAction(path)
, m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_DataFormat(dataFormat)
, m_FillValue(fillValue)
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  return ExecuteDataFunction(::CreateArrayFunctor{}, m_Type, dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat, m_FillValue);
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

std::string CreateArrayAction::fillValue() const
{
  return m_FillValue;
}
} // namespace nx::core
