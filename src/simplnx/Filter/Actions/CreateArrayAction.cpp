#include "CreateArrayAction.hpp"

#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
/**
 * @struct CreateArrayFunctor
 * @brief Dispatches array creation to a selected value type.
 */
struct CreateArrayFunctor
{
  /**
   * @brief Creates one dispatched numeric array.
   * @tparam T Dispatched array value type.
   * @param dataStructure Destination data structure.
   * @param tDims Row-major tuple dimensions.
   * @param cDims Component dimensions.
   * @param path Created array path.
   * @param mode Preflight or execute action mode.
   * @param dataFormat Requested storage format.
   * @param fillValue Serialized initial value.
   * @return Creation warnings or errors.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, IDataAction::Mode mode, const std::string& dataFormat,
                      std::string fillValue)
  {
    return ArrayCreationUtilities::CreateArray<T>(dataStructure, tDims, cDims, path, mode, dataFormat, fillValue);
  }
};
} // namespace

namespace nx::core
{
CreateArrayAction::CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat, std::string fillValue)
: IDataCreationAction(path)
, m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_DataFormat(std::move(dataFormat))
, m_FillValue(std::move(fillValue))
{
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  return ExecuteDataFunction(::CreateArrayFunctor{}, m_Type, dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat, m_FillValue);
}

IDataAction::UniquePointer CreateArrayAction::clone() const
{
  return std::make_unique<CreateArrayAction>(m_Type, m_Dims, m_CDims, getCreatedPath(), m_DataFormat, m_FillValue);
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

std::string CreateArrayAction::fillValue() const
{
  return m_FillValue;
}

std::string CreateArrayAction::dataFormat() const
{
  return m_DataFormat;
}
} // namespace nx::core
