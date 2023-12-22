#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ExecuteProcessInputValues
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

class SIMPLNXCORE_EXPORT ExecuteProcess
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

} // namespace nx::core
