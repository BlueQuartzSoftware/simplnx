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
  StringParameter::ValueType arguments;
  bool blocking{};
  int32 timeout{};
  FileSystemPathParameter::ValueType log_file;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ExecuteProcess
{
public:
  ExecuteProcess(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExecuteProcessInputValues* inputValues);
  ~ExecuteProcess() noexcept = default;

  ExecuteProcess(const ExecuteProcess&) = delete;
  ExecuteProcess(ExecuteProcess&&) noexcept = delete;
  ExecuteProcess& operator=(const ExecuteProcess&) = delete;
  ExecuteProcess& operator=(ExecuteProcess&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  // -----------------------------------------------------------------------------
  static std::vector<std::string> SplitArgumentsString(const std::string& arguments);

private:
  DataStructure& m_DataStructure;
  const ExecuteProcessInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
