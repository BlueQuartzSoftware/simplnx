#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExecuteProcessInputValues inputValues;

  inputValues.Arguments = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);

  return ExecuteProcess(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ExecuteProcessInputValues
{
  StringParameter::ValueType Arguments;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ExecuteProcess
{
public:
  ExecuteProcess(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExecuteProcessInputValues* inputValues);
  ~ExecuteProcess() noexcept;

  ExecuteProcess(const ExecuteProcess&) = delete;
  ExecuteProcess(ExecuteProcess&&) noexcept = delete;
  ExecuteProcess& operator=(const ExecuteProcess&) = delete;
  ExecuteProcess& operator=(ExecuteProcess&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExecuteProcessInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
