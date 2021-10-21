#include "ExecuteProcess.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExecuteProcess::name() const
{
  return FilterTraits<ExecuteProcess>::name.str();
}

//------------------------------------------------------------------------------
std::string ExecuteProcess::className() const
{
  return FilterTraits<ExecuteProcess>::className;
}

//------------------------------------------------------------------------------
Uuid ExecuteProcess::uuid() const
{
  return FilterTraits<ExecuteProcess>::uuid;
}

//------------------------------------------------------------------------------
std::string ExecuteProcess::humanName() const
{
  return "Execute Process";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExecuteProcess::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ExecuteProcess::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StringParameter>(k_Arguments_Key, "Command Line Arguments", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExecuteProcess::clone() const
{
  return std::make_unique<ExecuteProcess>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ExecuteProcess::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pArgumentsValue = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExecuteProcessAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExecuteProcess::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pArgumentsValue = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
