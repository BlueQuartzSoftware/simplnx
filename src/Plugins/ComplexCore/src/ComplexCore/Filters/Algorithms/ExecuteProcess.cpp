#include "ExecuteProcess.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/StringUtilities.hpp"

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
  std::string command = arguments[0];
  arguments.erase(arguments.begin());

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
