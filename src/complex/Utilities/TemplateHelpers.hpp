#pragma once

#include <memory>
#include <vector>

#include "complex/Common/ComplexConstants.hpp"
#include "complex/DataStructure/DataObject.hpp"

#define VALIDATE_NUMERIC_TYPE(input_data, replace_value, result)                                                                                                                                       \
  if(TemplateHelpers::CanDynamicCast<Float32Array>()(input_data))                                                                                                                                      \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesFloatDouble<float>(replace_value, complex::Constants::k_Float32);                                                                                                            \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Float64Array>()(input_data))                                                                                                                                 \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesFloatDouble<double>(replace_value, complex::Constants::k_Float64);                                                                                                           \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int8Array>()(input_data))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<int8_t>(replace_value, complex::Constants::k_Int8);                                                                                                                      \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt8Array>()(input_data))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<uint8_t>(replace_value, complex::Constants::k_UInt8);                                                                                                                    \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int16Array>()(input_data))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<int16_t>(replace_value, complex::Constants::k_Int16);                                                                                                                    \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt16Array>()(input_data))                                                                                                                                  \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<uint16_t>(replace_value, complex::Constants::k_UInt16);                                                                                                                  \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int32Array>()(input_data))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<int32_t>(replace_value, complex::Constants::k_Int32);                                                                                                                    \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt32Array>()(input_data))                                                                                                                                  \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<uint32_t>(replace_value, complex::Constants::k_UInt32);                                                                                                                  \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int64Array>()(input_data))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<int64_t>(replace_value, complex::Constants::k_Int64);                                                                                                                    \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt64Array>()(input_data))                                                                                                                                  \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<uint64_t>(replace_value, complex::Constants::k_UInt64);                                                                                                                  \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<BoolArray>()(input_data))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    const BoolArray* data = dynamic_cast<const BoolArray*>(inputDataObject);                                                                                                                           \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(input_data))                                                                                                                            \
  {                                                                                                                                                                                                    \
    (result) = CheckValuesInt<size_t>(replace_value, complex::Constants::k_SizeT);                                                                                                                     \
  }                                                                                                                                                                                                    \
  else                                                                                                                                                                                                 \
  {                                                                                                                                                                                                    \
    (result) = complex::MakeErrorResult(-4060, "Incorrect data scalar type");                                                                                                                          \
  }

/**
 * @brief This macro includes the 'bool' type.
 */
#define EXECUTE_FUNCTION_TEMPLATE(templateName, result, inputData, ...)                                                                                                                                \
  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputData))                                                                                                                                       \
  {                                                                                                                                                                                                    \
    Float32Array* data = dynamic_cast<Float32Array*>(inputDataObject);                                                                                                                                 \
    templateName<float>(__VA_ARGS__);                                                                                                                                                                  \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputData))                                                                                                                                  \
  {                                                                                                                                                                                                    \
    Float64Array* data = dynamic_cast<Float64Array*>(inputDataObject);                                                                                                                                 \
    templateName<double>(__VA_ARGS__);                                                                                                                                                                 \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputData))                                                                                                                                     \
  {                                                                                                                                                                                                    \
    Int8Array* data = dynamic_cast<Int8Array*>(inputDataObject);                                                                                                                                       \
    templateName<int8_t>(__VA_ARGS__);                                                                                                                                                                 \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputData))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    UInt8Array* data = dynamic_cast<UInt8Array*>(inputDataObject);                                                                                                                                     \
    templateName<uint8_t>(__VA_ARGS__);                                                                                                                                                                \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputData))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    Int16Array* data = dynamic_cast<Int16Array*>(inputDataObject);                                                                                                                                     \
    templateName<int16_t>(__VA_ARGS__);                                                                                                                                                                \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputData))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    UInt16Array* data = dynamic_cast<UInt16Array*>(inputDataObject);                                                                                                                                   \
    templateName<uint16_t>(__VA_ARGS__);                                                                                                                                                               \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputData))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    Int32Array* data = dynamic_cast<Int32Array*>(inputDataObject);                                                                                                                                     \
    templateName<int32_t>(__VA_ARGS__);                                                                                                                                                                \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputData))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    UInt32Array* data = dynamic_cast<UInt32Array*>(inputDataObject);                                                                                                                                   \
    templateName<uint32_t>(__VA_ARGS__);                                                                                                                                                               \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputData))                                                                                                                                    \
  {                                                                                                                                                                                                    \
    Int64Array* data = dynamic_cast<Int64Array*>(inputDataObject);                                                                                                                                     \
    templateName<int64_t>(__VA_ARGS__);                                                                                                                                                                \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputData))                                                                                                                                   \
  {                                                                                                                                                                                                    \
    UInt64Array* data = dynamic_cast<UInt64Array*>(inputDataObject);                                                                                                                                   \
    templateName<uint64_t>(__VA_ARGS__);                                                                                                                                                               \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputData))                                                                                                                                     \
  {                                                                                                                                                                                                    \
    BoolArray* data = dynamic_cast<BoolArray*>(inputDataObject);                                                                                                                                       \
    templateName<bool>(__VA_ARGS__);                                                                                                                                                                   \
  }                                                                                                                                                                                                    \
  else if(TemplateHelpers::CanDynamicCast<DataArray<size_t>>()(inputData))                                                                                                                             \
  {                                                                                                                                                                                                    \
    DataArray<size_t>* data = dynamic_cast<DataArray<size_t>*>(inputDataObject);                                                                                                                       \
    templateName<size_t>(__VA_ARGS__);                                                                                                                                                                 \
  }                                                                                                                                                                                                    \
  else                                                                                                                                                                                                 \
  {                                                                                                                                                                                                    \
    result = complex::MakeErrorResult(-1, "help");                                                                                                                                                     \
    /*  observableObj->setErrorConditionWithPrefix(TemplateHelpers::Errors::UnsupportedDataType, #templateName, "The input array was of unsupported type"); */                                         \
  }

namespace complex
{
namespace TemplateHelpers
{
/**
 * @brief This class (functor) simply returns true or false if the IDataArray can be downcast to a certain DataArray type
 * parameterized by the template parameter T.
 */
template <typename T>
class CanDynamicCast
{
public:
  CanDynamicCast() = default;
  virtual ~CanDynamicCast() = default;
  CanDynamicCast(const CanDynamicCast&) = delete;            // Copy Constructor Not Implemented
  CanDynamicCast(CanDynamicCast&&) = delete;                 // Move Constructor Not Implemented
  CanDynamicCast& operator=(const CanDynamicCast&) = delete; // Copy Assignment Not Implemented
  CanDynamicCast& operator=(CanDynamicCast&&) = delete;      // Move Assignment Not Implemented

  bool operator()(std::shared_ptr<DataObject> p)
  {
    return (std::dynamic_pointer_cast<T>(p) != nullptr);
  }
  bool operator()(DataObject* p)
  {
    return (dynamic_cast<T*>(p) != nullptr);
  }

  bool operator()(const DataObject* p)
  {
    return (dynamic_cast<const T*>(p) != nullptr);
  }
};

} // end namespace TemplateHelpers

} // end namespace complex
