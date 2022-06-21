#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MultiThresholdObjects2InputValues inputValues;

  inputValues.SelectedThresholds = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  inputValues.DestinationArrayName = filterArgs.value<DataPath>(k_DestinationArrayName_Key);

  return MultiThresholdObjects2(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT MultiThresholdObjects2InputValues
{
  <<<NOT_IMPLEMENTED>>> SelectedThresholds;
/*[x]*/  DataPath DestinationArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT MultiThresholdObjects2
{
public:
  MultiThresholdObjects2(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjects2InputValues* inputValues);
  ~MultiThresholdObjects2() noexcept;

  MultiThresholdObjects2(const MultiThresholdObjects2&) = delete;
  MultiThresholdObjects2(MultiThresholdObjects2&&) noexcept = delete;
  MultiThresholdObjects2& operator=(const MultiThresholdObjects2&) = delete;
  MultiThresholdObjects2& operator=(MultiThresholdObjects2&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjects2InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
