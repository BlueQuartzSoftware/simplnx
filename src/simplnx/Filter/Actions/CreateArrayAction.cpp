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
                      std::string fillValue, const std::optional<ShapeType>& chunkShapeHint, DataStoreInitializationMode initializationMode)
  {
    return ArrayCreationUtilities::CreateArray<T>(dataStructure, tDims, cDims, path, mode, dataFormat, fillValue, chunkShapeHint, initializationMode);
  }
};
} // namespace

namespace nx::core
{
CreateArrayAction::CreateArrayAction(DataType type, const std::vector<usize>& tDims, const std::vector<usize>& cDims, const DataPath& path, std::string dataFormat, std::string fillValue,
                                     std::optional<ShapeType> chunkShapeHint, DataStoreInitializationMode initializationMode)
: IDataCreationAction(path)
, m_Type(type)
, m_Dims(tDims)
, m_CDims(cDims)
, m_DataFormat(std::move(dataFormat))
, m_FillValue(std::move(fillValue))
, m_ChunkShapeHint(std::move(chunkShapeHint))
, m_InitializationMode(initializationMode)
{
  if(!m_FillValue.empty())
  {
    m_InitializationMode = DataStoreInitializationMode::Default;
  }
}

CreateArrayAction::~CreateArrayAction() noexcept = default;

Result<> CreateArrayAction::apply(DataStructure& dataStructure, Mode mode) const
{
  return ExecuteDataFunction(::CreateArrayFunctor{}, m_Type, dataStructure, m_Dims, m_CDims, getCreatedPath(), mode, m_DataFormat, m_FillValue, m_ChunkShapeHint, m_InitializationMode);
}

IDataAction::UniquePointer CreateArrayAction::clone() const
{
  return std::make_unique<CreateArrayAction>(m_Type, m_Dims, m_CDims, getCreatedPath(), m_DataFormat, m_FillValue, m_ChunkShapeHint, m_InitializationMode);
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

const std::optional<ShapeType>& CreateArrayAction::chunkShapeHint() const
{
  return m_ChunkShapeHint;
}

DataStoreInitializationMode CreateArrayAction::initializationMode() const noexcept
{
  return m_InitializationMode;
}
} // namespace nx::core
