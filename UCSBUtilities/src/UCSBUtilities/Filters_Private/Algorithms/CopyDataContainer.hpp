#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyDataContainerInputValues inputValues;

  inputValues.SelectedDataContainerName = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  inputValues.NewDataContainerName = filterArgs.value<DataPath>(k_NewDataContainerName_Key);

  return CopyDataContainer(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT CopyDataContainerInputValues
{
  DataPath SelectedDataContainerName;
  DataPath NewDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT CopyDataContainer
{
public:
  CopyDataContainer(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyDataContainerInputValues* inputValues);
  ~CopyDataContainer() noexcept;

  CopyDataContainer(const CopyDataContainer&) = delete;
  CopyDataContainer(CopyDataContainer&&) noexcept = delete;
  CopyDataContainer& operator=(const CopyDataContainer&) = delete;
  CopyDataContainer& operator=(CopyDataContainer&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CopyDataContainerInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
