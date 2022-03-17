#include "DataArrayUtilities.hpp"

#include "complex/Common/TypesUtility.hpp"

using namespace complex;

namespace
{
template <class T>
Result<> ReplaceArray(DataStructure& dataStructure, const DataPath& dataPath, const std::vector<usize>& tupleShape, IDataAction::Mode mode, const IDataArray& inputDataArray)
{
  const DataArray<T>& castInputArray = dynamic_cast<const DataArray<T>&>(inputDataArray);
  IDataStore::ShapeType componentShape = castInputArray.getDataStoreRef().getComponentShape();
  dataStructure.removeData(dataPath);
  return CreateArray<T>(dataStructure, tupleShape, componentShape, dataPath, mode);
}
} // namespace

namespace complex
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
  if(TemplateHelpers::CanDynamicCast<DataArray<usize>>()(&inputDataArray))
  {
    return CheckValuesUnsignedInt<usize>(value, Constants::k_USize);
  }

  return {MakeErrorResult(-259, fmt::format("Input DataObject could not be cast to any primitive type."))};
}

//-----------------------------------------------------------------------------
Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray)
{
  IDataArray& iDataArray = dynamic_cast<IDataArray&>(inputDataObject);
  complex::DataType arrayType = iDataArray.getDataType();
  Result<> resultFromConversion;
  switch(arrayType)
  {
  case complex::DataType::int8:
    return ConditionalReplaceValueInArrayFromString<int8>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::uint8:
    return ConditionalReplaceValueInArrayFromString<uint8>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::int16:
    return ConditionalReplaceValueInArrayFromString<int16>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::uint16:
    return ConditionalReplaceValueInArrayFromString<uint16>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::int32:
    return ConditionalReplaceValueInArrayFromString<int32>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::uint32:
    return ConditionalReplaceValueInArrayFromString<uint32>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::int64:
    return ConditionalReplaceValueInArrayFromString<int64>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::uint64:
    return ConditionalReplaceValueInArrayFromString<uint64>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::float32:
    return ConditionalReplaceValueInArrayFromString<float32>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::float64:
    return ConditionalReplaceValueInArrayFromString<float64>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  case complex::DataType::boolean:
    return ConditionalReplaceValueInArrayFromString<bool>(valueAsStr, inputDataObject, conditionalDataArray);
    break;
  }
  return {};
}

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
  if(TemplateHelpers::CanDynamicCast<DataArray<usize>>()(inputDataArray))
  {
    return ReplaceArray<usize>(dataStructure, dataPath, tupleShape, mode, *inputDataArray);
  }

  return MakeErrorResult(-401, fmt::format("The input array at DataPath '{}' was of an unsupported type", dataPath.toString()));
}
} // namespace complex
