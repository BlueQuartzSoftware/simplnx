#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InputCrystalCompliancesInputValues inputValues;

  inputValues.Compliances = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Compliances_Key);
  inputValues.CrystalCompliancesArrayPath = filterArgs.value<DataPath>(k_CrystalCompliancesArrayPath_Key);

  return InputCrystalCompliances(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT InputCrystalCompliancesInputValues
{
  <<<NOT_IMPLEMENTED>>> Compliances;
  /*[x]*/ DataPath CrystalCompliancesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT InputCrystalCompliances
{
public:
  InputCrystalCompliances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InputCrystalCompliancesInputValues* inputValues);
  ~InputCrystalCompliances() noexcept;

  InputCrystalCompliances(const InputCrystalCompliances&) = delete;
  InputCrystalCompliances(InputCrystalCompliances&&) noexcept = delete;
  InputCrystalCompliances& operator=(const InputCrystalCompliances&) = delete;
  InputCrystalCompliances& operator=(InputCrystalCompliances&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InputCrystalCompliancesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
