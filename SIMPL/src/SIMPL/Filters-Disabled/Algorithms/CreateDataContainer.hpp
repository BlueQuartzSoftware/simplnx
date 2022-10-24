#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateDataContainerInputValues inputValues;

  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);

  return CreateDataContainer(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateDataContainerInputValues
{
  DataPath DataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateDataContainer
{
public:
  CreateDataContainer(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataContainerInputValues* inputValues);
  ~CreateDataContainer() noexcept;

  CreateDataContainer(const CreateDataContainer&) = delete;
  CreateDataContainer(CreateDataContainer&&) noexcept = delete;
  CreateDataContainer& operator=(const CreateDataContainer&) = delete;
  CreateDataContainer& operator=(CreateDataContainer&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateDataContainerInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
