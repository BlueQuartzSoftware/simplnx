#include "ExecuteProcess.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>
#include <reproc++/run.hpp>

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
  auto absPath = m_InputValues->LogFile;
  if(!absPath.is_absolute())
  {
    try
    {
      absPath = fs::absolute(absPath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult(-15000,
                             fmt::format("ExecuteProcess::operator()() threw an error when creating absolute path from '{}'. Reported error is '{}'", m_InputValues->LogFile.string(), error.what()));
    }
  }

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(absPath.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  // open the log file for storing the process output
  std::ofstream outFile;
  outFile.open(m_InputValues->LogFile, std::ios_base::out);
  if(!outFile.is_open())
  {
    return MakeErrorResult<>(-56411, fmt::format("Error creating output log file with file path {}", m_InputValues->LogFile.string()));
  }

  // gather the command/arguments and process options
  std::vector<std::string> arguments = ExecuteProcess::splitArgumentsString(m_InputValues->Arguments);
  auto args = reproc::arguments(arguments);

  reproc::process process;
  reproc::stop_actions stop = {{reproc::stop::terminate, reproc::milliseconds(m_InputValues->Timeout)}, {reproc::stop::kill, reproc::milliseconds(m_InputValues->Timeout)}};
  reproc::options options;
  options.stop = stop;
  std::error_code ec = process.start(args, options);

  if(ec == std::errc::no_such_file_or_directory)
  {
    outFile.close();
    return MakeErrorResult<>(-56410, fmt::format("Program {} not found. Make sure it's available from the PATH.", arguments[0]));
  }
  else if(ec)
  {
    outFile.close();
    return MakeErrorResult<>(-56410, fmt::format("An error occurred while starting process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
  }

  std::string output;
  if(m_InputValues->Blocking)
  {
    reproc::sink::string sink(output);
    ec = reproc::drain(process, sink, reproc::sink::null);
    if(ec)
    {
      outFile.close();
      return MakeErrorResult<>(-56413, fmt::format("An error occurred while executing process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
    }
    outFile << output;
  }
  else
  {
    // redirect the process output to the output string so we can save it to a log file
    std::mutex mutex;
    auto drain_async = std::async(std::launch::async, [&process, &output, &mutex]() {
      reproc::sink::thread_safe::string sink(output, mutex);
      return reproc::drain(process, sink, sink);
    });

    // Show new output every 2 seconds.
    while(drain_async.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
    {
      std::lock_guard<std::mutex> lock(mutex);
      outFile << output;
      //  Clear output that's already been flushed
      output.clear();

      if(getCancel())
      {
        break;
      }
    }
    // Flush any remaining output of `process`.
    outFile << output;

    // Check if any errors occurred in the background thread.
    ec = drain_async.get();
    if(ec)
    {
      outFile.close();
      return MakeErrorResult<>(-56413, fmt::format("An error occured while executing process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
    }
  }

  int status = 0;
  std::tie(status, ec) = process.wait(reproc::infinite);
  if(ec)
  {
    outFile.close();
    return MakeErrorResult<>(-56414, fmt::format("An error occured while executing process '{}'\n{} : {}", m_InputValues->Arguments, ec.value(), ec.message()));
  }

  if(m_InputValues->Blocking)
  {
    outFile << output;
  }

  outFile.close();
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
