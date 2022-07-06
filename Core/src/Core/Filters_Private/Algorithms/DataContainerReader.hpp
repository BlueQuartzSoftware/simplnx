#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DataContainerReaderInputValues inputValues;

  inputValues.OverwriteExistingDataContainers = filterArgs.value<bool>(k_OverwriteExistingDataContainers_Key);
  inputValues.InputFileDataContainerArrayProxy = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileDataContainerArrayProxy_Key);

  return DataContainerReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT DataContainerReaderInputValues
{
  bool OverwriteExistingDataContainers;
  <<<NOT_IMPLEMENTED>>> InputFileDataContainerArrayProxy;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT DataContainerReader
{
public:
  DataContainerReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DataContainerReaderInputValues* inputValues);
  ~DataContainerReader() noexcept;

  DataContainerReader(const DataContainerReader&) = delete;
  DataContainerReader(DataContainerReader&&) noexcept = delete;
  DataContainerReader& operator=(const DataContainerReader&) = delete;
  DataContainerReader& operator=(DataContainerReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DataContainerReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
