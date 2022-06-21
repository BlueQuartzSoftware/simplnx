#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MultiThresholdObjectsInputValues inputValues;

  inputValues.SelectedThresholds = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  inputValues.DestinationArrayName = filterArgs.value<StringParameter::ValueType>(k_DestinationArrayName_Key);

  return MultiThresholdObjects(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT MultiThresholdObjectsInputValues
{
  <<<NOT_IMPLEMENTED>>> SelectedThresholds;
/*[x]*/  StringParameter::ValueType DestinationArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT MultiThresholdObjects
{
public:
  MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjects() noexcept;

  MultiThresholdObjects(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects(MultiThresholdObjects&&) noexcept = delete;
  MultiThresholdObjects& operator=(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects& operator=(MultiThresholdObjects&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
