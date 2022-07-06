#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ClearDataMaskInputValues inputValues;

  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return ClearDataMask(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT ClearDataMaskInputValues
{
  DataPath MaskArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT ClearDataMask
{
public:
  ClearDataMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ClearDataMaskInputValues* inputValues);
  ~ClearDataMask() noexcept;

  ClearDataMask(const ClearDataMask&) = delete;
  ClearDataMask(ClearDataMask&&) noexcept = delete;
  ClearDataMask& operator=(const ClearDataMask&) = delete;
  ClearDataMask& operator=(ClearDataMask&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ClearDataMaskInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
