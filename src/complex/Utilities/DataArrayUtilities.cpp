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

} // namespace complex
