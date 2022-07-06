#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SplitAttributeArrayInputValues inputValues;

  inputValues.InputArrayPath = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  inputValues.SplitArraysSuffix = filterArgs.value<StringParameter::ValueType>(k_SplitArraysSuffix_Key);

  return SplitAttributeArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT SplitAttributeArrayInputValues
{
  DataPath InputArrayPath;
  StringParameter::ValueType SplitArraysSuffix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT SplitAttributeArray
{
public:
  SplitAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitAttributeArrayInputValues* inputValues);
  ~SplitAttributeArray() noexcept;

  SplitAttributeArray(const SplitAttributeArray&) = delete;
  SplitAttributeArray(SplitAttributeArray&&) noexcept = delete;
  SplitAttributeArray& operator=(const SplitAttributeArray&) = delete;
  SplitAttributeArray& operator=(SplitAttributeArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitAttributeArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
