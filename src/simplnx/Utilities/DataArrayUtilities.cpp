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
    auto* neighborList = dynamic_cast<NeighborList<T>*>(iNeighborList);
    neighborList->setList(neighborList->getNumberOfTuples() - 1, typename NeighborList<T>::SharedVectorType(new typename NeighborList<T>::VectorType));
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
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
    types.insert(dataArray->getDataType());
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

  Preferences* preferences = Application::GetOrCreateInstance()->getPreferences();

  const uint64 memoryUsage = dataStructure.memoryUsage() + requiredMemory;
  const uint64 largeDataStructureSize = preferences->largeDataStructureSize();
  std::string largeDataFormat = preferences->largeDataFormat();

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
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(dataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    return ReplaceArray<float32>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    return ReplaceArray<float64>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    return ReplaceArray<int8>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    return ReplaceArray<uint8>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    return ReplaceArray<int16>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    return ReplaceArray<uint16>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    return ReplaceArray<int32>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    return ReplaceArray<uint32>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    return ReplaceArray<int64>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    return ReplaceArray<uint64>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  {
    return ReplaceArray<bool>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }

  return MakeErrorResult(-401, fmt::format("The input array at DataPath '{}' was of an unsupported type", dataPath.toString()));
}

//-----------------------------------------------------------------------------
Result<> ValidateNumFeaturesInArray(const DataStructure& dataStructure, const DataPath& arrayPath, const Int32Array& featureIds)
{
  const auto* featureArray = dataStructure.getDataAs<IDataArray>(arrayPath);
  if(featureArray == nullptr)
  {
    return MakeErrorResult(-5550, fmt::format("Could not find the input array path '{}' for validating number of features", arrayPath.toString()));
  }
  Result<> results = {};
  const usize numFeatures = featureArray->getNumberOfTuples();

  for(const int32& featureId : featureIds)
  {
    if(featureId < 0)
    {
      results = MakeErrorResult(
          -5555, fmt::format("Feature Ids array with name '{}' has negative values within the array. The first negative value encountered was '{}'. All values must be positive within the array",
                             featureIds.getName(), featureId));
      return results;
    }
    if(static_cast<usize>(featureId) >= numFeatures)
    {
      results = MakeErrorResult(-5551, fmt::format("Feature Ids array with name '{}' has a value '{}' that would exceed the number of tuples {} in the selected feature array '{}'",
                                                   featureIds.getName(), featureId, numFeatures, arrayPath.toString()));
      return results;
    }
  }
  return results;
}

//-----------------------------------------------------------------------------
void InitializeNeighborList(DataStructure& dataStructure, const DataPath& neighborListPath)
{
  auto* neighborList = dataStructure.getDataAs<INeighborList>(neighborListPath);
  ExecuteNeighborFunction(InitializeNeighborListFunctor{}, neighborList->getDataType(), neighborList);
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
    return ConvertDataArray<int8>(std::dynamic_pointer_cast<DataArray<int8>>(dataArray), dataFormat);
  case DataType::int16:
    return ConvertDataArray<int16>(std::dynamic_pointer_cast<DataArray<int16>>(dataArray), dataFormat);
  case DataType::int32:
    return ConvertDataArray<int32>(std::dynamic_pointer_cast<DataArray<int32>>(dataArray), dataFormat);
  case DataType::int64:
    return ConvertDataArray<int64>(std::dynamic_pointer_cast<DataArray<int64>>(dataArray), dataFormat);
  case DataType::uint8:
    return ConvertDataArray<uint8>(std::dynamic_pointer_cast<DataArray<uint8>>(dataArray), dataFormat);
  case DataType::uint16:
    return ConvertDataArray<uint16>(std::dynamic_pointer_cast<DataArray<uint16>>(dataArray), dataFormat);
  case DataType::uint32:
    return ConvertDataArray<uint32>(std::dynamic_pointer_cast<DataArray<uint32>>(dataArray), dataFormat);
  case DataType::uint64:
    return ConvertDataArray<uint64>(std::dynamic_pointer_cast<DataArray<uint64>>(dataArray), dataFormat);
  case DataType::boolean:
    return ConvertDataArray<bool>(std::dynamic_pointer_cast<DataArray<bool>>(dataArray), dataFormat);
  case DataType::float32:
    return ConvertDataArray<float32>(std::dynamic_pointer_cast<DataArray<float32>>(dataArray), dataFormat);
  case DataType::float64:
    return ConvertDataArray<float64>(std::dynamic_pointer_cast<DataArray<float64>>(dataArray), dataFormat);
  default:
    return false;
  }
}
} // namespace nx::core
