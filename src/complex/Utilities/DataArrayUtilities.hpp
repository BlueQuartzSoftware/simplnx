#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"
#include "complex/complex_export.hpp"

#include <iostream>
#include <string>

#define CDA_CREATE_CONVERTOR(TYPE, FUNCTION)                                                                                                                                                           \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      TYPE value;                                                                                                                                                                                      \
      try                                                                                                                                                                                              \
      {                                                                                                                                                                                                \
        value = static_cast<TYPE>(FUNCTION(input));                                                                                                                                                    \
      } catch(std::invalid_argument const& e)                                                                                                                                                          \
      {                                                                                                                                                                                                \
        return complex::MakeErrorResult<TYPE>(-100, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                            \
      } catch(std::out_of_range const& e)                                                                                                                                                              \
      {                                                                                                                                                                                                \
        return complex::MakeErrorResult<TYPE>(-101, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                   \
      }                                                                                                                                                                                                \
      return {value};                                                                                                                                                                                  \
    }                                                                                                                                                                                                  \
  };

namespace complex
{
template <class T>
struct ConvertTo
{
};

/**
 * These macros will create convertor objects that convert from a string to a numeric type
 */

CDA_CREATE_CONVERTOR(uint8, std::stoull)
CDA_CREATE_CONVERTOR(int8, std::stoll)
CDA_CREATE_CONVERTOR(uint16, std::stoull)
CDA_CREATE_CONVERTOR(int16, std::stoll)
CDA_CREATE_CONVERTOR(uint32, std::stoull)
CDA_CREATE_CONVERTOR(int32, std::stoll)
CDA_CREATE_CONVERTOR(uint64, std::stoull)
CDA_CREATE_CONVERTOR(int64, std::stoll)
#ifdef __APPLE__
CDA_CREATE_CONVERTOR(usize, std::stoull)
#endif
CDA_CREATE_CONVERTOR(float32, std::stof)
CDA_CREATE_CONVERTOR(float64, std::stod)

/**
 * @brief Checks if the given string can be correctly converted into the given type
 * @tparam T The primitive type to conver the string into
 * @param valueAsStr The value to convert
 * @param strType The primitive type. The valid values can be found in a constants file
 * @return Result<> object that is either valid or has an error message/code
 */
template <class T>
Result<> CheckValuesUnsignedInt(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_unsigned_v<T>);

  if(valueAsStr[0] == '-')
  {
    return MakeErrorResult(-255, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType, std::numeric_limits<T>::min(),
                                             std::numeric_limits<T>::max()));
  }
  Result<uint64> conversionResult = ConvertTo<uint64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    uint64 replaceValue = conversionResult.value();
    std::string ss;
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return MakeErrorResult(-256, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                               std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

// -----------------------------------------------------------------------------
template <class T>
Result<> CheckValuesSignedInt(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_signed_v<T>);

  Result<int64> conversionResult = ConvertTo<int64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    int64 replaceValue = conversionResult.value();
    std::string ss;
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return MakeErrorResult(-257, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                               std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

// -----------------------------------------------------------------------------
template <class T>
Result<> CheckValuesFloatDouble(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_floating_point_v<T>);

  Result<float64> conversionResult = ConvertTo<float64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    float64 replaceValue = conversionResult.value();
    std::string ss;

    if(!(((replaceValue >= static_cast<T>(-1) * std::numeric_limits<T>::max()) && (replaceValue <= static_cast<T>(-1) * std::numeric_limits<T>::min())) || (replaceValue == 0) ||
         ((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max()))))
    {
      return MakeErrorResult<>(-258, fmt::format("The {} replace value was invalid. The valid ranges are -{} to -{}, 0, %{} to %{}", std::numeric_limits<T>::max(), strType,
                                                 std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

/**
 * @brief Validates whether the string can be converted to the primitive type used in the DataObject.
 *
 * The validate will check overflow and underflow and that the string represents some sort of numeric value
 * @param value
 * @param inputDataArray
 * @return
 */
COMPLEX_EXPORT Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject& inputDataArray);

/**
 * @brief Replaces every value in an array based on a `mask` array.
 * @tparam T The primitive type used in the data array
 * @param inputArrayPtr InputArray that will have values replaced
 * @param condDataPtr The mask array as a boolean array
 * @param replaceValue The value that will be used for every place the conditional array is TRUE
 */
template <class T>
void ReplaceValue(DataArray<T>& inputArrayPtr, const BoolArray& condDataPtr, T replaceValue)
{
  T replaceVal = static_cast<T>(replaceValue);
  usize numTuples = inputArrayPtr.getNumberOfTuples();

  for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    if(condDataPtr[tupleIndex])
    {
      inputArrayPtr.initializeTuple(tupleIndex, replaceValue);
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
template <class T>
bool ConditionalReplaceValueInArrayFromString(const std::string& valueAsStr, DataObject& inputDataObject, const BoolArray& conditionalDataArray)
{
  using DataArrayType = DataArray<T>;
  if(!TemplateHelpers::CanDynamicCast<DataArrayType>()(&inputDataObject))
  {
    return false;
  }
  DataArrayType& inputDataArray = dynamic_cast<DataArrayType&>(inputDataObject);
  Result<T> conversionResult = ConvertTo<T>::convert(valueAsStr);
  if(conversionResult.invalid())
  {
    return false;
  }
  ReplaceValue<T>(inputDataArray, conditionalDataArray, conversionResult.value());
  return true;
}

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return
 */
COMPLEX_EXPORT Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const BoolArray& conditionalDataArray);

/**
 * @brief Creates a DataStore with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param tupleShape The Tuple Dimensions
 * @param componentShape The component dimensions
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
std::unique_ptr<IDataStore<T>> CreateDataStore(const typename IDataStore<T>::ShapeType& tupleShape, const typename IDataStore<T>::ShapeType& componentShape, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape);
  }
  case IDataAction::Mode::Execute: {
    return std::make_unique<DataStore<T>>(tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Creates a DataArray with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param dataStructure The DataStructure to use
 * @param tupleShape The Tuple Dimensions
 * @param nComp The number of components in the DataArray
 * @param path The DataPath to where the data will be stored.
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& tupleShape, uint64 nComp, const DataPath& path, IDataAction::Mode mode)
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> id;

  if(parentPath.getLength() != 0)
  {
    auto* parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-262, fmt::format("Parent object \"{}\" does not exist", parentPath.toString()));
    }

    id = parentObject->getId();
  }

  // Validate Number of Components
  if(nComp == 0)
  {
    return MakeErrorResult(-261, fmt::format("CreateArrayAction: Number of components is ZERO. Please set the number of components. Value being used:'{}'", nComp));
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  auto store = CreateDataStore<T>(tupleShape, {nComp}, mode);
  auto dataArray = DataArray<T>::Create(dataStructure, name, std::move(store), id);
  if(dataArray == nullptr)
  {
    return MakeErrorResult(-264, fmt::format("Unable to create DataArray at \"{}\"", path.toString()));
  }

  return {};
}
} // namespace complex
