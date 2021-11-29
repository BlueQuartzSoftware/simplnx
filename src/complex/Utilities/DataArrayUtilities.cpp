#include "DataArrayUtilities.hpp"

namespace complex
{
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

Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const BoolArray& conditionalDataArray)
{
  if(ConditionalReplaceValueInArrayFromString<float32>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<float64>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<uint8>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint16>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint32>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint64>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<int8>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int16>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int32>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int64>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<usize>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  return {MakeErrorResult(-260, fmt::format("Input DataObject could not be cast to any primitive type."))};
}

#define DAU_BODY(type)                                                                                                                                                                                 \
  DataArray<type>* castInputArray = dynamic_cast<DataArray<type>*>(inputDataArray);                                                                                                                    \
  IDataStore<type>::ShapeType componentShape = castInputArray->getDataStore()->getComponentShape();                                                                                                    \
  dataStructure.removeData(dataPath);                                                                                                                                                                  \
  return CreateArray<type>(dataStructure, tupleShape, componentShape, dataPath, mode);

Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, std::vector<usize>& tupleShape, complex::IDataAction::Mode mode)
{
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(dataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    DAU_BODY(float)
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    DAU_BODY(double)
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    DAU_BODY(int8_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    DAU_BODY(uint8_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    DAU_BODY(int16_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    DAU_BODY(uint16_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    DAU_BODY(int32_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    DAU_BODY(uint32_t)
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    DAU_BODY(int64_t)
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    DAU_BODY(uint64_t)
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  {
    DAU_BODY(bool)
  }
  if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(inputDataArray))
  {
    DAU_BODY(size_t)
  }

  return MakeErrorResult(-401, fmt::format("The input array at DataPath '{}' was of an unsupported type", dataPath.toString()));
}

} // namespace complex
