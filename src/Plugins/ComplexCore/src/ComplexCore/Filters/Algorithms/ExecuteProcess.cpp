#include "ExecuteProcess.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "reproc++/include/reproc++/drain.hpp"
#include "reproc++/include/reproc++/reproc.hpp"
#include "reproc++/include/reproc++/run.hpp"

#include <future>
#include <mutex>

using namespace complex;

// -----------------------------------------------------------------------------
ExecuteProcess::ExecuteProcess(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExecuteProcessInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExecuteProcess::~ExecuteProcess() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExecuteProcess::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExecuteProcess::operator()()
{
  std::vector<std::string> arguments = ExecuteProcess::splitArgumentsString(m_InputValues->Arguments);
  auto args = reproc::arguments::arguments(arguments);
  reproc::process process;

  // if(m_InputValues->Blocking)
  //{
  reproc::stop_actions stop = {{reproc::stop::terminate, reproc::milliseconds(m_InputValues->Timeout)}, {reproc::stop::kill, reproc::milliseconds(m_InputValues->Timeout)}};
  reproc::options options;
  options.stop = stop;
  std::error_code ec = process.start(args, options);

  if(ec == std::errc::no_such_file_or_directory)
  {
    return MakeErrorResult<>(-56410, fmt::format("Program {} not found. Make sure it's available from the PATH.", arguments[0]));
  }
  else if(ec)
  {
    return MakeErrorResult<>(-56410, fmt::format("An error occured while starting process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
  }

  std::string output;
  std::mutex mutex;
  auto drain_async = std::async(std::launch::async, [&process, &output, &mutex]() {
    reproc::sink::thread_safe::string sink(output, mutex);
    return reproc::drain(process, sink, sink);
  });
  int status = 0;
  // Show new output every 2 seconds.
  while(drain_async.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
  {
    std::lock_guard<std::mutex> lock(mutex);
    m_MessageHandler(IFilter::Message::Type::Info, output);
    //  Clear output that's already been flushed
    output.clear();

    if(getCancel())
    {
      std::tie(status, ec) = process.stop(options.stop);
      if(ec)
      {
        return MakeErrorResult<>(-56411, fmt::format("An error occured while stopping process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
      }
    }
  }
  // Flush any remaining output of `process`.
  m_MessageHandler(IFilter::Message::Type::Info, output);

  // Check if any errors occurred in the background thread.
  ec = drain_async.get();
  if(ec)
  {
    return MakeErrorResult<>(-56412, fmt::format("An error occured while executing process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
  }

  status = 0;
  std::tie(status, ec) = process.stop(options.stop);
  if(ec)
  {
    return MakeErrorResult<>(-56413, fmt::format("An error occured while executing process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
  }
  //}

  return {};
}

// -----------------------------------------------------------------------------
std::vector<std::string> ExecuteProcess::splitArgumentsString(const std::string& arguments)
{
  std::vector<std::string> argumentList;
  for(int i = 0; i < arguments.size(); i++)
  {
    if(arguments[i] == '\"')
    {
      i++;
      int start = i;
      int index = arguments.find_first_of("\"", start);
      if(index == -1)
      {
        index = arguments.size();
      }
      int end = index - 1;
      argumentList.push_back(arguments.substr(start, end - start + 1));
      i = index;
    }
    else
    {
      int start = i;
      int index = arguments.find_first_of(" ", start + 1);
      if(index == -1)
      {
        index = arguments.size();
      }
      int end = index - 1;
      argumentList.push_back(arguments.substr(start, end - start + 1));
      i = index;
    }
  }

  return argumentList;
}
