#include "CreateGridMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateGridMontage::name() const
{
  return FilterTraits<CreateGridMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateGridMontage::className() const
{
  return FilterTraits<CreateGridMontage>::className;
}

//------------------------------------------------------------------------------
Uuid CreateGridMontage::uuid() const
{
  return FilterTraits<CreateGridMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateGridMontage::humanName() const
{
  return "Create Grid Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateGridMontage::defaultTags() const
{
  return {"#Core", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateGridMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateGridMontage::clone() const
{
  return std::make_unique<CreateGridMontage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateGridMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateGridMontageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateGridMontage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
