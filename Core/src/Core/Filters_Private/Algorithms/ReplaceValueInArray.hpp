#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReplaceValueInArrayInputValues inputValues;

  inputValues.RemoveValue = filterArgs.value<float64>(k_RemoveValue_Key);
  inputValues.ReplaceValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  inputValues.SelectedArray = filterArgs.value<DataPath>(k_SelectedArray_Key);

  return ReplaceValueInArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ReplaceValueInArrayInputValues
{
  float64 RemoveValue;
  float64 ReplaceValue;
  DataPath SelectedArray;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ReplaceValueInArray
{
public:
  ReplaceValueInArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReplaceValueInArrayInputValues* inputValues);
  ~ReplaceValueInArray() noexcept;

  ReplaceValueInArray(const ReplaceValueInArray&) = delete;
  ReplaceValueInArray(ReplaceValueInArray&&) noexcept = delete;
  ReplaceValueInArray& operator=(const ReplaceValueInArray&) = delete;
  ReplaceValueInArray& operator=(ReplaceValueInArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReplaceValueInArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
