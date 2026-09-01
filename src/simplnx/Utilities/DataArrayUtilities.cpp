#include "DataArrayUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @brief Replaces one numeric array with resolver-selected storage and a new tuple shape.
 * @tparam T Specifies the array value type.
 * @param dataStructure Owns the array.
 * @param dataPath Identifies the array to replace.
 * @param tupleShape Specifies the new tuple shape.
 * @param mode Selects preflight or execution behavior.
 * @param inputDataArray Supplies the component shape before removal.
 * @return Array-creation result.
 *
 * The original array is removed before replacement creation. A creation failure
 * leaves dataPath absent.
 */
template <class T>
Result<> ReplaceArray(DataStructure& dataStructure, const DataPath& dataPath, const ShapeType& tupleShape, IDataAction::Mode mode, const IDataArray& inputDataArray)
{
  auto& castInputArray = dynamic_cast<const DataArray<T>&>(inputDataArray);
  const ShapeType componentShape = castInputArray.getDataStoreRef().getComponentShape();
  dataStructure.removeData(dataPath);
  return ArrayCreationUtilities::CreateArray<T>(dataStructure, tupleShape, componentShape, dataPath, mode);
}

/**
 * @struct InitializeNeighborListFunctor
 * @brief Dispatches creation of an empty final list for one neighbor value type.
 */
struct InitializeNeighborListFunctor
{
  /**
   * @brief Replaces the final tuple with an empty list.
   * @tparam T Specifies the neighbor value type.
   * @param iNeighborList Supplies the runtime-validated neighbor list.
   */
  template <typename T>
  void operator()(INeighborList* iNeighborList)
  {
    auto* neighborListPtr = dynamic_cast<NeighborList<T>*>(iNeighborList);
    neighborListPtr->setList(neighborListPtr->getNumberOfTuples() - 1, typename NeighborList<T>::SharedVectorType(new typename NeighborList<T>::VectorType));
  }
};

/**
 * @struct CreateDefaultValueDataArrayFunctor
 * @brief Creates and fills one resolver-selected numeric array.
 */
struct CreateDefaultValueDataArrayFunctor
{
  /**
   * @brief Converts one default value and creates a filled array.
   * @tparam T Specifies the numeric value type.
   * @param destDataStructure Receives the array.
   * @param name Specifies the array name.
   * @param tupleShape Specifies tuple dimensions.
   * @param componentShape Specifies component dimensions.
   * @param defaultValue Supplies the value as text.
   * @param parentId Identifies an optional parent object.
   * @return Created array, or a conversion, parent, allocation, or insertion error.
   *
   * If a parent has multiple paths, the first data-structure path selects storage resolution.
   */
  template <typename T>
  Result<IArray*> operator()(DataStructure& destDataStructure, const std::string& name, const ShapeType& tupleShape, const ShapeType& componentShape, const std::string& defaultValue,
                             const std::optional<DataObject::IdType> parentId)
  {
    auto result = StringInterpretationUtilities::Convert<T>(defaultValue);
    if(result.invalid())
    {
      return ConvertResultTo<IArray*>(ConvertResult(std::move(result)), {});
    }

    DataPath arrayPath({name});
    if(parentId.has_value())
    {
      const std::vector<DataPath> parentPaths = destDataStructure.getDataPathsForId(*parentId);
      if(parentPaths.empty())
      {
        return MakeErrorResult<IArray*>(-1053, fmt::format("Unable to resolve the parent path while creating default-initialized data array '{}'.", name));
      }
      arrayPath = parentPaths.front().createChildPath(name);
    }

    auto dataStore = DataStoreUtilities::CreateDataStore<T>(destDataStructure, arrayPath, tupleShape, componentShape);
    if(dataStore == nullptr)
    {
      return MakeErrorResult<IArray*>(-1054, fmt::format("Unable to allocate storage for default-initialized data array '{}'.", name));
    }
    dataStore->fill(result.value());
    auto* newDataArray = DataArray<T>::Create(destDataStructure, name, std::move(dataStore), parentId);
    if(newDataArray == nullptr)
    {
      return MakeErrorResult<IArray*>(-1055, fmt::format("Unable to insert default-initialized data array '{}'.", name));
    }
    return {newDataArray};
  }
};

/**
 * @struct CreateDefaultValueNeighborListFunctor
 * @brief Creates an empty NeighborList for one runtime value type.
 */
struct CreateDefaultValueNeighborListFunctor
{
  /**
   * @brief Creates one NeighborList.
   * @tparam T Specifies the neighbor value type.
   * @param destDataStructure Receives the list.
   * @param name Specifies the list name.
   * @param tupleShape Specifies tuple dimensions.
   * @param parentId Identifies an optional parent object.
   * @return Created list pointer.
   */
  template <typename T>
  Result<IArray*> operator()(DataStructure& destDataStructure, const std::string& name, const ShapeType& tupleShape, const std::optional<DataObject::IdType> parentId)
  {
    auto newNeighborList = NeighborList<T>::Create(destDataStructure, name, tupleShape, parentId);
    return {newNeighborList};
  }
};
} // namespace

namespace nx::core
{
bool CheckArraysAreSameType(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths)
{
  std::set<nx::core::DataType> types;
  for(const auto& dataPath : dataArrayPaths)
  {
    const auto* dataArrayPtr = dataStructure.getDataAs<IDataArray>(dataPath);
    types.insert(dataArrayPtr->getDataType());
  }
  return types.size() == 1;
}

bool CheckArraysHaveSameTupleCount(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths)
{
  std::set<size_t> types;
  for(const auto& dataPath : dataArrayPaths)
  {
    const auto* iArrayPtr = dataStructure.getDataAs<IArray>(dataPath);
    types.insert(iArrayPtr->getNumberOfTuples());
  }
  return types.size() == 1;
}

Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, bool invertMask)
{
  const IDataArray& iDataArray = dynamic_cast<IDataArray&>(inputDataObject);
  const nx::core::DataType arrayType = iDataArray.getDataType();
  return ExecuteDataFunction(ConditionalReplaceValueInArrayFromString{}, arrayType, valueAsStr, inputDataObject, conditionalDataArray, invertMask);
}

Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, ShapeType& tupleShape, IDataAction::Mode mode)
{
  auto* inputDataArrayPtr = dataStructure.getDataAs<IDataArray>(dataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<float32>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<float64>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<int8>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<uint8>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<int16>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<uint16>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<int32>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<uint32>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<int64>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArrayPtr))
  {
    return ReplaceArray<uint64>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArrayPtr))
  {
    return ReplaceArray<bool>(dataStructure, dataPath, tupleShape, mode, *inputDataArrayPtr);
  }

  return MakeErrorResult(-401, fmt::format("The input array at DataPath '{}' was of an unsupported type", dataPath.toString()));
}

Result<> ValidateFeatureIdsToFeatureAttributeMatrixIndexing(const DataStructure& dataStructure, const DataPath& sourceDataPath, const Int32Array& featureIds, bool ignoreNegativeValues,
                                                            const IFilter::MessageHandler& messageHandler)
{
  return ValidateFeatureIdsToFeatureAttributeMatrixIndexing(dataStructure, sourceDataPath, featureIds, ignoreNegativeValues, messageHandler, nullptr);
}

Result<> ValidateFeatureIdsToFeatureAttributeMatrixIndexing(const DataStructure& dataStructure, const DataPath& sourceDataPath, const Int32Array& featureIds, bool ignoreNegativeValues,
                                                            const IFilter::MessageHandler& messageHandler, const std::atomic_bool* shouldCancel)
{
  messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Validating range of values within input array '{}'...", featureIds.getName())});

  usize numFeatures = 0;

  // The source can be an AttributeMatrix or one feature-level array.
  auto* targetAttributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(sourceDataPath);
  if(nullptr != targetAttributeMatrixPtr)
  {
    numFeatures = targetAttributeMatrixPtr->getNumberOfTuples();
  }
  auto* targetFeatureArrayPtr = dataStructure.getDataAs<IArray>(sourceDataPath);
  if(nullptr != targetFeatureArrayPtr)
  {
    numFeatures = targetFeatureArrayPtr->getNumberOfTuples();
  }

  auto& featureIdsStore = featureIds.getDataStoreRef();
  if(featureIdsStore.getNumberOfTuples() == 0)
  {
    return {};
  }

  // Fixed-size bulk reads avoid one virtual store and cache operation per feature ID.
  const usize totalTuples = featureIdsStore.getNumberOfTuples();
  constexpr usize k_BatchSize = 40000;
  int32 globalMin = std::numeric_limits<int32>::max();
  int32 globalMax = std::numeric_limits<int32>::lowest();
  std::vector<int32> batch(k_BatchSize);
  for(usize offset = 0; offset < totalTuples; offset += k_BatchSize)
  {
    if(shouldCancel != nullptr && *shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_BatchSize, totalTuples - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(batch.data(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    auto [batchMin, batchMax] = std::minmax_element(batch.begin(), batch.begin() + count);
    globalMin = std::min(globalMin, *batchMin);
    globalMax = std::max(globalMax, *batchMax);
  }
  const int32* minFeatureId = &globalMin;
  const int32* maxFeatureId = &globalMax;

  if(!ignoreNegativeValues && *minFeatureId < 0)
  {
    return MakeErrorResult(
        -5355, fmt::format("Feature Ids array with name '{}' has negative values within the array. The most negative value encountered was '{}'. All values must be positive within the array",
                           featureIds.getName(), *minFeatureId));
  }

  if(*maxFeatureId >= numFeatures)
  {
    return MakeErrorResult(-5351, fmt::format("Feature Ids array with name '{}' has a value '{}' that would exceed the number of tuples {} in the selected Data Path: '{}'", featureIds.getName(),
                                              *maxFeatureId, numFeatures, sourceDataPath.toString()));
  }

  return {};
}

void InitializeNeighborList(DataStructure& dataStructure, const DataPath& neighborListPath)
{
  auto* neighborListPtr = dataStructure.getDataAs<INeighborList>(neighborListPath);
  ExecuteNeighborFunction(InitializeNeighborListFunctor{}, neighborListPtr->getDataType(), neighborListPtr);
}

//-----------------------------------------------------------------------------
bool ConvertIDataArray(const std::shared_ptr<IDataArray>& dataArray, const std::string& dataFormat)
{
  auto dataType = dataArray->getDataType();
  switch(dataType)
  {
  case DataType::int8:
    return ConvertDataArrayDataStore<int8>(std::dynamic_pointer_cast<DataArray<int8>>(dataArray), dataFormat);
  case DataType::int16:
    return ConvertDataArrayDataStore<int16>(std::dynamic_pointer_cast<DataArray<int16>>(dataArray), dataFormat);
  case DataType::int32:
    return ConvertDataArrayDataStore<int32>(std::dynamic_pointer_cast<DataArray<int32>>(dataArray), dataFormat);
  case DataType::int64:
    return ConvertDataArrayDataStore<int64>(std::dynamic_pointer_cast<DataArray<int64>>(dataArray), dataFormat);
  case DataType::uint8:
    return ConvertDataArrayDataStore<uint8>(std::dynamic_pointer_cast<DataArray<uint8>>(dataArray), dataFormat);
  case DataType::uint16:
    return ConvertDataArrayDataStore<uint16>(std::dynamic_pointer_cast<DataArray<uint16>>(dataArray), dataFormat);
  case DataType::uint32:
    return ConvertDataArrayDataStore<uint32>(std::dynamic_pointer_cast<DataArray<uint32>>(dataArray), dataFormat);
  case DataType::uint64:
    return ConvertDataArrayDataStore<uint64>(std::dynamic_pointer_cast<DataArray<uint64>>(dataArray), dataFormat);
  case DataType::boolean:
    return ConvertDataArrayDataStore<bool>(std::dynamic_pointer_cast<DataArray<bool>>(dataArray), dataFormat);
  case DataType::float32:
    return ConvertDataArrayDataStore<float32>(std::dynamic_pointer_cast<DataArray<float32>>(dataArray), dataFormat);
  case DataType::float64:
    return ConvertDataArrayDataStore<float64>(std::dynamic_pointer_cast<DataArray<float64>>(dataArray), dataFormat);
  default:
    return false;
  }
}

Result<IArray*> CreateDefaultValueArrayFromArray(DataStructure& destDataStructure, IArray* array, const std::string& newArrayName, const ShapeType& tupleShape, const std::string& defaultValue,
                                                 const std::optional<DataObject::IdType> parentId)
{
  switch(array->getArrayType())
  {
  case IArray::ArrayType::StringArray: {
    auto newStringArray = StringArray::Create(destDataStructure, newArrayName, parentId);
    newStringArray->resizeTuples(tupleShape);
    std::fill(newStringArray->begin(), newStringArray->end(), defaultValue);
    return {newStringArray};
  }
  case IArray::ArrayType::DataArray: {
    auto iDataArray = dynamic_cast<IDataArray*>(array);
    auto result = ExecuteDataFunction(CreateDefaultValueDataArrayFunctor{}, iDataArray->getDataType(), destDataStructure, newArrayName, tupleShape, array->getComponentShape(), defaultValue, parentId);
    if(result.invalid())
    {
      return MakeErrorResult<IArray*>(-1050, fmt::format("Unable to create default-initialized data array to append to data array '{}': {}", array->getName(), result.errors()[0].message));
    }
    return result;
  }
  case IArray::ArrayType::NeighborListArray: {
    auto iNeighborList = dynamic_cast<INeighborList*>(array);
    auto result = ExecuteNeighborFunction(CreateDefaultValueNeighborListFunctor{}, iNeighborList->getDataType(), destDataStructure, newArrayName, tupleShape, parentId);
    if(result.invalid())
    {
      return MakeErrorResult<IArray*>(-1051, fmt::format("Unable to create default-initialized neighbor list to append to neighbor list '{}': {}", array->getName(), result.errors()[0].message));
    }
    return result;
  }
  case IArray::ArrayType::Any:
  default: {
    return MakeErrorResult<IArray*>(
        -1052, fmt::format("Unable to create a default-initialized array: array '{}' is not a StringArray, DataArray, or NeighborList, and these are the only array types supported by this filter!",
                           array->getName()));
  }
  }
}

namespace TransferGeometryElementData
{
void transferElementData(DataStructure& m_DataStructure, AttributeMatrix& destCellDataAM, const std::vector<DataPath>& sourceDataPaths, const std::vector<usize>& newEdgesIndexList,
                         const std::atomic_bool& m_ShouldCancel, const IFilter::MessageHandler& m_MessageHandler)
{
  // Each task owns one source and destination array pair. No two tasks access the
  // same DataArray.
  ParallelTaskAlgorithm taskRunner;
  for(const auto& edgeDataArrayPath : sourceDataPaths)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    const auto& oldDataArray = m_DataStructure.getDataRefAs<IDataArray>(edgeDataArrayPath);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Copying Data Array {}", srcName));
    ExecuteParallelFunction<CopyCellDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, newEdgesIndexList, m_ShouldCancel);
  }
  taskRunner.wait();
}

void CreateDataArrayActions(const DataStructure& dataStructure, const AttributeMatrix* sourceAttrMatPtr, const MultiArraySelectionParameter::ValueType& selectedArrayPaths,
                            const DataPath& reducedGeometryPathAttrMatPath, Result<OutputActions>& resultOutputActions)
{
  // Preserve each source value type and component shape in the destination action.
  for(const auto& dataPath : selectedArrayPaths)
  {
    const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
    DataType dataType = srcArray.getDataType();
    ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
    DataPath dataArrayPath = reducedGeometryPathAttrMatPath.createChildPath(srcArray.getName());
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, sourceAttrMatPtr->getShape(), std::move(componentShape), dataArrayPath));
  }
}
} // namespace TransferGeometryElementData
} // namespace nx::core
