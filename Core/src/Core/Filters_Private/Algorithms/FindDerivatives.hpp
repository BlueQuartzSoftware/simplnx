#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindDerivativesInputValues inputValues;

  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.DerivativesArrayPath = filterArgs.value<DataPath>(k_DerivativesArrayPath_Key);

  return FindDerivatives(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT FindDerivativesInputValues
{
  DataPath SelectedArrayPath;
  DataPath DerivativesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT FindDerivatives
{
public:
  FindDerivatives(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindDerivativesInputValues* inputValues);
  ~FindDerivatives() noexcept;

  FindDerivatives(const FindDerivatives&) = delete;
  FindDerivatives(FindDerivatives&&) noexcept = delete;
  FindDerivatives& operator=(const FindDerivatives&) = delete;
  FindDerivatives& operator=(FindDerivatives&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindDerivativesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
