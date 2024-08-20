#include "DataArrayUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <set>

using namespace nx::core;

namespace
{
template <class T>
Result<> ReplaceArray(DataStructure& dataStructure, const DataPath& dataPath, const std::vector<usize>& tupleShape, IDataAction::Mode mode, const IDataArray& inputDataArray)
{
  auto& castInputArray = dynamic_cast<const DataArray<T>&>(inputDataArray);
  const IDataStore::ShapeType componentShape = castInputArray.getDataStoreRef().getComponentShape();
  dataStructure.removeData(dataPath);
  return CreateArray<T>(dataStructure, tupleShape, componentShape, dataPath, mode);
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
Result<> CheckValueConverts(const std::string& value, NumericType numericType)
{
  switch(numericType)
  {
  case NumericType::int8: {
    return CheckValuesSignedInt<int8>(value, Constants::k_Int8);
  }
  case NumericType::uint8: {
    return CheckValuesUnsignedInt<uint8>(value, Constants::k_UInt8);
  }
  case NumericType::int16: {
    return CheckValuesSignedInt<int16>(value, Constants::k_Int16);
  }
  case NumericType::uint16: {
    return CheckValuesUnsignedInt<uint16>(value, Constants::k_UInt16);
  }
  case NumericType::int32: {
    return CheckValuesSignedInt<int32>(value, Constants::k_Int32);
  }
  case NumericType::uint32: {
    return CheckValuesUnsignedInt<uint32>(value, Constants::k_UInt32);
  }
  case NumericType::int64: {
    return CheckValuesSignedInt<int64>(value, Constants::k_Int64);
  }
  case NumericType::uint64: {
    return CheckValuesUnsignedInt<uint64>(value, Constants::k_UInt64);
  }
  case NumericType::float32: {
    return CheckValuesFloatDouble<float32>(value, Constants::k_Float32);
  }
  case NumericType::float64: {
    return CheckValuesFloatDouble<float64>(value, Constants::k_Float64);
  }
  }
  return MakeErrorResult(-10102, fmt::format("CheckInitValueConverts: Cannot convert input value '{}' to type '{}'", value, NumericTypeToString(numericType)));
}

//-----------------------------------------------------------------------------
Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject& inputDataArray)
{
  if(TemplateHelpers::CanDynamicCast<Float32Array>()(&inputDataArray))
  {
    return CheckValuesFloatDouble<float32>(value, Constants::k_Float32);
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(&inputDataArray))
  {
    return CheckValuesFloatDouble<float64>(value, Constants::k_Float64);
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(&inputDataArray))
  {
    return CheckValuesSignedInt<int8>(value, Constants::k_Int8);
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(&inputDataArray))
  {
    return CheckValuesUnsignedInt<uint8>(value, Constants::k_UInt8);
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(&inputDataArray))
  {
    return CheckValuesSignedInt<int16>(value, Constants::k_Int16);
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(&inputDataArray))
  {
    return CheckValuesUnsignedInt<uint16>(value, Constants::k_UInt16);
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(&inputDataArray))
  {
    return CheckValuesSignedInt<int32>(value, Constants::k_Int32);
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(&inputDataArray))
  {
    return CheckValuesUnsignedInt<uint32>(value, Constants::k_UInt32);
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(&inputDataArray))
  {
    return CheckValuesSignedInt<int64>(value, Constants::k_Int64);
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(&inputDataArray))
  {
    return CheckValuesUnsignedInt<uint64>(value, Constants::k_UInt64);
  }

  return {MakeErrorResult(-259, fmt::format("Input DataObject could not be cast to any primitive type."))};
}

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
bool CheckMemoryRequirement(DataStructure& dataStructure, uint64 requiredMemory, std::string& format)
{
  static const uint64 k_AvailableMemory = Memory::GetTotalMemory();

  // Only check if format is set to in-memory
  if(!format.empty())
  {
    return true;
  }

  Preferences* preferencesPtr = Application::GetOrCreateInstance()->getPreferences();

  const uint64 memoryUsage = dataStructure.memoryUsage() + requiredMemory;
  const uint64 largeDataStructureSize = preferencesPtr->largeDataStructureSize();
  std::string largeDataFormat = preferencesPtr->largeDataFormat();

  if(memoryUsage >= largeDataStructureSize)
  {
    // Check if out-of-core is available / enabled
    if(largeDataFormat.empty() && memoryUsage >= k_AvailableMemory)
    {
      return false;
    }
    // Use out-of-core
    format = largeDataFormat;
  }

  return true;
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
Result<> ValidateNumFeaturesInArray(const DataStructure& dataStructure, const DataPath& arrayPath, const Int32Array& featureIds)
{
  const auto* featureArrayPtr = dataStructure.getDataAs<IDataArray>(arrayPath);
  if(featureArrayPtr == nullptr)
  {
    return MakeErrorResult(-5350, fmt::format("Could not find the input array path '{}' for validating number of features", arrayPath.toString()));
  }
  Result<> results = {};
  const usize numFeatures = featureArrayPtr->getNumberOfTuples();

  auto& featureIdsStore = featureIds.getDataStoreRef();

  for(const int32& featureId : featureIdsStore)
  {
    if(featureId < 0)
    {
      results = MakeErrorResult(
          -5355, fmt::format("Feature Ids array with name '{}' has negative values within the array. The first negative value encountered was '{}'. All values must be positive within the array",
                             featureIds.getName(), featureId));
      return results;
    }
    if(static_cast<usize>(featureId) >= numFeatures)
    {
      results = MakeErrorResult(-5351, fmt::format("Feature Ids array with name '{}' has a value '{}' that would exceed the number of tuples {} in the selected feature array '{}'",
                                                   featureIds.getName(), featureId, numFeatures, arrayPath.toString()));
      return results;
    }
  }
  return results;
}

//-----------------------------------------------------------------------------
void InitializeNeighborList(DataStructure& dataStructure, const DataPath& neighborListPath)
{
  auto* neighborListPtr = dataStructure.getDataAs<INeighborList>(neighborListPath);
  ExecuteNeighborFunction(InitializeNeighborListFunctor{}, neighborListPtr->getDataType(), neighborListPtr);
}

//-----------------------------------------------------------------------------
std::unique_ptr<MaskCompare> InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath)
{
  auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);

  return InstantiateMaskCompare(maskArray);
}

//-----------------------------------------------------------------------------
std::unique_ptr<MaskCompare> InstantiateMaskCompare(IDataArray& maskArray)
{
  switch(maskArray.getDataType())
  {
  case DataType::boolean: {
    return std::make_unique<BoolMaskCompare>(dynamic_cast<BoolArray&>(maskArray).getDataStoreRef());
  }
  case DataType::uint8: {
    return std::make_unique<UInt8MaskCompare>(dynamic_cast<UInt8Array&>(maskArray).getDataStoreRef());
  }
  default:
    throw std::runtime_error("InstantiateMaskCompare: The Mask Array being used is NOT of type bool or uint8.");
  }
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
