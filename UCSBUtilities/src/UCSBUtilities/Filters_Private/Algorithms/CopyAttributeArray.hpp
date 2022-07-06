#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyAttributeArrayInputValues inputValues;

  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayName = filterArgs.value<DataPath>(k_NewArrayName_Key);

  return CopyAttributeArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT CopyAttributeArrayInputValues
{
  DataPath SelectedArrayPath;
  DataPath NewArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT CopyAttributeArray
{
public:
  CopyAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyAttributeArrayInputValues* inputValues);
  ~CopyAttributeArray() noexcept;

  CopyAttributeArray(const CopyAttributeArray&) = delete;
  CopyAttributeArray(CopyAttributeArray&&) noexcept = delete;
  CopyAttributeArray& operator=(const CopyAttributeArray&) = delete;
  CopyAttributeArray& operator=(CopyAttributeArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CopyAttributeArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
