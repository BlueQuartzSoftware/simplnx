#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ExecuteProcessInputValues
{
  StringParameter::ValueType Arguments;
  bool Blocking;
  int32 Timeout;
  FileSystemPathParameter::ValueType LogFile;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ExecuteProcess
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

  // -----------------------------------------------------------------------------
  static std::vector<std::string> splitArgumentsString(const std::string& arguments);

private:
  DataStructure& m_DataStructure;
  const ExecuteProcessInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
