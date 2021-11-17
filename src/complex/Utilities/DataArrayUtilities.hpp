#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"

#include <iostream>
#include <string>

namespace complex
{

#define CDA_CREATE_CONVERTOR(TYPE, FUNCTION)                                                                                                                                                           \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static std::pair<TYPE, Result<>> convert(const std::string& input)                                                                                                                                 \
    {                                                                                                                                                                                                  \
      TYPE value;                                                                                                                                                                                      \
      try                                                                                                                                                                                              \
      {                                                                                                                                                                                                \
        value = static_cast<TYPE>(FUNCTION(input));                                                                                                                                                    \
      } catch(std::invalid_argument const& e)                                                                                                                                                          \
      {                                                                                                                                                                                                \
        return {value, MakeErrorResult(-100, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION))};                                                  \
      } catch(std::out_of_range const& e)                                                                                                                                                              \
      {                                                                                                                                                                                                \
        return {value, MakeErrorResult(-100, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION))};                                         \
      }                                                                                                                                                                                                \
      return {value, {}};                                                                                                                                                                              \
    }                                                                                                                                                                                                  \
  };

template <class T>
struct ConvertTo
{
};

/**
 * These macros will create convertor objects that convert from a string to a numeric type
 */

CDA_CREATE_CONVERTOR(uint8_t, std::stoull)
CDA_CREATE_CONVERTOR(int8_t, std::stoll)
CDA_CREATE_CONVERTOR(uint16_t, std::stoull)
CDA_CREATE_CONVERTOR(int16_t, std::stoll)
CDA_CREATE_CONVERTOR(uint32_t, std::stoull)
CDA_CREATE_CONVERTOR(int32_t, std::stoll)
CDA_CREATE_CONVERTOR(uint64_t, std::stoull)
CDA_CREATE_CONVERTOR(int64_t, std::stoll)
CDA_CREATE_CONVERTOR(size_t, std::stoull)
// CDA_CREATE_CONVERTOR(bool, toInt)
CDA_CREATE_CONVERTOR(float, std::stof)
CDA_CREATE_CONVERTOR(double, std::stod)

/**
 * @brief Checks if the given string can be correctly converted into the given type
 * @tparam T The primitive type to conver the string into
 * @param valueAsStr The value to convert
 * @param strType The primitive type. The valid values can be found in a constants file
 * @return Result<> object that is either valid or has an error message/code
 */
template <typename T>
Result<> CheckValuesUnsignedInt(const std::string& valueAsStr, const std::string& strType)
{
  using CovertorType = ConvertTo<uint64_t>;
  using ConversionResultType = std::pair<uint64_t, Result<>>;
  if(valueAsStr[0] == '-')
  {
    return complex::MakeErrorResult<>(-255, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                                        std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
  }
  ConversionResultType conversionResult = CovertorType::convert(valueAsStr);
  if(conversionResult.second.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    uint64_t replaceValue = conversionResult.first;
    std::string ss;
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return complex::MakeErrorResult<>(-256, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                                          std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return conversionResult.second;
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> CheckValuesSignedInt(const std::string& valueAsStr, const std::string& strType)
{
  using CovertorType = ConvertTo<int64_t>;
  using ConversionResultType = std::pair<int64_t, Result<>>;
  ConversionResultType conversionResult = CovertorType::convert(valueAsStr);
  if(conversionResult.second.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    int64_t replaceValue = conversionResult.first;
    std::string ss;
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return complex::MakeErrorResult<>(-257, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                                          std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return conversionResult.second;
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> CheckValuesFloatDouble(const std::string& valueAsStr, const std::string& strType)
{
  using CovertorType = ConvertTo<double>;
  using ConversionResultType = std::pair<T, Result<>>;
  ConversionResultType conversionResult = CovertorType::convert(valueAsStr);
  if(conversionResult.second.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    double replaceValue = conversionResult.first;
    std::string ss;

    if(!(((replaceValue >= static_cast<T>(-1) * std::numeric_limits<T>::max()) && (replaceValue <= static_cast<T>(-1) * std::numeric_limits<T>::min())) || (replaceValue == 0) ||
         ((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max()))))
    {
      return complex::MakeErrorResult<>(-258, fmt::format("The {} replace value was invalid. The valid ranges are -{} to -{}, 0, %{} to %{}", std::numeric_limits<T>::max(), strType,
                                                          std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return conversionResult.second;
}

/**
 * @brief Validates whether the string can be converted to the primitive type used in the DataObject.
 *
 * The validate will check overflow and underflow and that the string represents some sort of numeric value
 * @param value
 * @param inputDataArray
 * @return
 */
Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject* inputDataArray)
{

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    return CheckValuesFloatDouble<float>(value, complex::Constants::k_Float32);
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    return CheckValuesFloatDouble<double>(value, complex::Constants::k_Float64);
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    return CheckValuesSignedInt<int8_t>(value, complex::Constants::k_Int8);
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    return CheckValuesUnsignedInt<uint8_t>(value, complex::Constants::k_UInt8);
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    return CheckValuesSignedInt<int16_t>(value, complex::Constants::k_Int16);
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    return CheckValuesUnsignedInt<uint16_t>(value, complex::Constants::k_UInt16);
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    return CheckValuesSignedInt<int32_t>(value, complex::Constants::k_Int32);
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    return CheckValuesUnsignedInt<uint32_t>(value, complex::Constants::k_UInt32);
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    return CheckValuesSignedInt<int64_t>(value, complex::Constants::k_Int64);
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    return CheckValuesUnsignedInt<uint64_t>(value, complex::Constants::k_UInt64);
  }

  //   if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  //  {
  //    const BoolArray* data = dynamic_cast<const BoolArray*>(inputDataObject);
  //  }

  if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(inputDataArray))
  {
    return CheckValuesUnsignedInt<size_t>(value, complex::Constants::k_SizeT);
  }

  return {complex::MakeErrorResult(-259, fmt::format("Input DataObject could not be cast to any primitive type."))};
}

/**
 * @brief Replaces every value in an array based on a `mask` array.
 * @tparam T The primitive type used in the data array
 * @param inputArrayPtr InputArray that will have values replaced
 * @param condDataPtr The mask array as a boolean array
 * @param replaceValue The value that will be used for every place the conditional array is TRUE
 */
template <typename T>
void ReplaceValue(DataArray<T>* inputArrayPtr, BoolArray* condDataPtr, T replaceValue)
{
  T replaceVal = static_cast<T>(replaceValue);
  size_t numTuples = inputArrayPtr->getNumberOfTuples();

  for(size_t tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    if((*condDataPtr)[tupleIndex])
    {
      inputArrayPtr->initializeTuple(tupleIndex, replaceValue);
    }
  }
}

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @tparam T Primitive type used for the DataArray
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return True or False whether the replacement algorithm was run. This function can
 * return FALSE if the wrong array type is specified as the template parameter
 */
template <typename T>
bool ConditionalReplaceValueInArrayFromString(const std::string& valueAsStr, DataObject* inputDataObject, BoolArray* conditionalDataArray)
{
  using DataArrayType = DataArray<T>;
  if(TemplateHelpers::CanDynamicCast<DataArrayType>()(inputDataObject))
  {
    DataArrayType* inputDataArray = dynamic_cast<DataArrayType*>(inputDataObject);
    using CovertorType = ConvertTo<T>;
    using ConversionResultType = std::pair<T, Result<>>;
    ConversionResultType conversionResult = CovertorType::convert(valueAsStr);
    ReplaceValue<T>(inputDataArray, conditionalDataArray, conversionResult.first);
    return true;
  }
  return false;
}

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return
 */
Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject* inputDataObject, BoolArray* conditionalDataArray)
{

  if(ConditionalReplaceValueInArrayFromString<float32>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<double>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<uint8_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint16_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint32_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<uint64_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<int8_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int16_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int32_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }
  if(ConditionalReplaceValueInArrayFromString<int64_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  if(ConditionalReplaceValueInArrayFromString<size_t>(valueAsStr, inputDataObject, conditionalDataArray))
  {
    return {};
  }

  //   if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataObject))
  //  {
  //    const BoolArray* data = dynamic_cast<const BoolArray*>(inputDataObject);
  //  }

  if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(inputDataObject))
  {
    return CheckValuesUnsignedInt<size_t>(valueAsStr, complex::Constants::k_SizeT);
  }

  return {complex::MakeErrorResult(-260, fmt::format("Input DataObject could not be cast to any primitive type."))};
}

/**
 * @brief Creates a DataStore with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param tupleShape The Tuple Dimensions
 * @param componentShape The component dimensions
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
IDataStore<T>* CreateDataStore(const typename IDataStore<T>::ShapeType& tupleShape, const typename IDataStore<T>::ShapeType& componentShape, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return new EmptyDataStore<T>(tupleShape, componentShape);
  }
  case IDataAction::Mode::Execute: {
    return new DataStore<T>(tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& dims, uint64 nComp, const DataPath& path, IDataAction::Mode mode)
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> id;

  if(parentPath.getLength() != 0)
  {
    auto parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Parent object \"{}\" does not exist", parentPath.toString())}})};
    }

    id = parentObject->getId();
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  auto* store = CreateDataStore<T>(dims, {nComp}, mode);
  auto dataArray = DataArray<T>::Create(dataStructure, name, store, id);
  if(dataArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("Unable to create DataArray at \"{}\"", path.toString())}})};
  }

  return {};
}
} // namespace complex
