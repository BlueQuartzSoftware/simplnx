#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateStringArrayInputValues inputValues;

  inputValues.InitializationValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  inputValues.NewArray = filterArgs.value<DataPath>(k_NewArray_Key);

  return CreateStringArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateStringArrayInputValues
{
  StringParameter::ValueType InitializationValue;
  DataPath NewArray;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateStringArray
{
public:
  CreateStringArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateStringArrayInputValues* inputValues);
  ~CreateStringArray() noexcept;

  CreateStringArray(const CreateStringArray&) = delete;
  CreateStringArray(CreateStringArray&&) noexcept = delete;
  CreateStringArray& operator=(const CreateStringArray&) = delete;
  CreateStringArray& operator=(CreateStringArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateStringArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
