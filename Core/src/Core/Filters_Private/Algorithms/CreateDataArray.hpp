#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateDataArrayInputValues inputValues;

  inputValues.ScalarType = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  inputValues.NumberOfComponents = filterArgs.value<int32>(k_NumberOfComponents_Key);
  inputValues.InitializationType = filterArgs.value<ChoicesParameter::ValueType>(k_InitializationType_Key);
  inputValues.InitializationValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  inputValues.InitializationRange = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitializationRange_Key);
  inputValues.NewArray = filterArgs.value<DataPath>(k_NewArray_Key);

  return CreateDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateDataArrayInputValues
{
  <<<NOT_IMPLEMENTED>>> ScalarType;
  /*[x]*/ int32 NumberOfComponents;
  ChoicesParameter::ValueType InitializationType;
  StringParameter::ValueType InitializationValue;
  <<<NOT_IMPLEMENTED>>> InitializationRange;
  /*[x]*/ DataPath NewArray;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateDataArray
{
public:
  CreateDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataArrayInputValues* inputValues);
  ~CreateDataArray() noexcept;

  CreateDataArray(const CreateDataArray&) = delete;
  CreateDataArray(CreateDataArray&&) noexcept = delete;
  CreateDataArray& operator=(const CreateDataArray&) = delete;
  CreateDataArray& operator=(CreateDataArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
