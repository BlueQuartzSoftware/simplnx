#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RenameDataContainerInputValues inputValues;

  inputValues.SelectedDataContainerName = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  inputValues.NewDataContainerName = filterArgs.value<DataPath>(k_NewDataContainerName_Key);

  return RenameDataContainer(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RenameDataContainerInputValues
{
  DataPath SelectedDataContainerName;
  DataPath NewDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RenameDataContainer
{
public:
  RenameDataContainer(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameDataContainerInputValues* inputValues);
  ~RenameDataContainer() noexcept;

  RenameDataContainer(const RenameDataContainer&) = delete;
  RenameDataContainer(RenameDataContainer&&) noexcept = delete;
  RenameDataContainer& operator=(const RenameDataContainer&) = delete;
  RenameDataContainer& operator=(RenameDataContainer&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RenameDataContainerInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
