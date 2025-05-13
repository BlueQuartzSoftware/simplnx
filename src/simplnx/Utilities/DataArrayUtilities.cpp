#include "DataArrayUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"

using namespace nx::core;

namespace
{
template <class T>
Result<> ReplaceArray(DataStructure& dataStructure, const DataPath& dataPath, const std::vector<usize>& tupleShape, IDataAction::Mode mode, const IDataArray& inputDataArray)
{
  auto& castInputArray = dynamic_cast<const DataArray<T>&>(inputDataArray);
  const IDataStore::ShapeType componentShape = castInputArray.getDataStoreRef().getComponentShape();
  dataStructure.removeData(dataPath);
  return ArrayCreationUtilities::CreateArray<T>(dataStructure, tupleShape, componentShape, dataPath, mode);
}

struct InitializeNeighborListFunctor
{
  template <typename T>
  void operator()(INeighborList* iNeighborList)
  {
    auto* neighborListPtr = dynamic_cast<NeighborList<T>*>(iNeighborList);
    neighborListPtr->setList(neighborListPtr->getNumberOfTuples() - 1, typename NeighborList<T>::SharedVectorType(new typename NeighborList<T>::VectorType));
  }
};
} // namespace

namespace nx::core
{
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, bool invertMask)
{
  const IDataArray& iDataArray = dynamic_cast<IDataArray&>(inputDataObject);
  const nx::core::DataType arrayType = iDataArray.getDataType();
  return ExecuteDataFunction(ConditionalReplaceValueInArrayFromString{}, arrayType, valueAsStr, inputDataObject, conditionalDataArray, invertMask);
}

//-----------------------------------------------------------------------------
Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, std::vector<usize>& tupleShape, IDataAction::Mode mode)
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

//-----------------------------------------------------------------------------
Result<> ValidateFeatureIdsToFeatureAttributeMatrixIndexing(const DataStructure& dataStructure, const DataPath& sourceDataPath, const Int32Array& featureIds,
                                                            const IFilter::MessageHandler& messageHandler)
{
  messageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Validating range of values within input array '{}'...", featureIds.getName())});

  usize numFeatures = 0;
  std::string sourceName;

  // Check if an Attribute Matrix was passed in
  auto* targetAttributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(sourceDataPath);
  if(nullptr != targetAttributeMatrixPtr)
  {
    numFeatures = targetAttributeMatrixPtr->getNumTuples();
  }
  // Check if a feature array was passed in
  auto* targetFeatureArrayPtr = dataStructure.getDataAs<IArray>(sourceDataPath);
  if(nullptr != targetFeatureArrayPtr)
  {
    numFeatures = targetFeatureArrayPtr->getNumberOfTuples();
  }

  auto& featureIdsStore = featureIds.getDataStoreRef();
  auto [minFeatureId, maxFeatureId] = std::minmax_element(featureIdsStore.begin(), featureIdsStore.end());

  if(*minFeatureId < 0)
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

//-----------------------------------------------------------------------------
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

namespace TransferGeometryElementData
{
void transferElementData(DataStructure& m_DataStructure, AttributeMatrix& destCellDataAM, const std::vector<DataPath>& sourceDataPaths, const std::vector<usize>& newEdgesIndexList,
                         const std::atomic_bool& m_ShouldCancel, const IFilter::MessageHandler& m_MessageHandler)
{
  // The actual cropping of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
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
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
}

void CreateDataArrayActions(const DataStructure& dataStructure, const AttributeMatrix* sourceAttrMatPtr, const MultiArraySelectionParameter::ValueType& selectedArrayPaths,
                            const DataPath& reducedGeometryPathAttrMatPath, Result<OutputActions>& resultOutputActions)
{
  // Now loop over each array in selectedEdgeArrays and create the corresponding arrays
  // in the destination geometry's attribute matrix
  for(const auto& dataPath : selectedArrayPaths)
  {
    const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
    DataType dataType = srcArray.getDataType();
    IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
    DataPath dataArrayPath = reducedGeometryPathAttrMatPath.createChildPath(srcArray.getName());
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, sourceAttrMatPtr->getShape(), std::move(componentShape), dataArrayPath));
  }
}
} // namespace TransferGeometryElementData
} // namespace nx::core
