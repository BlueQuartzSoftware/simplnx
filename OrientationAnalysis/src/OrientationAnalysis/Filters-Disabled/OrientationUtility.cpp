#include "OrientationUtility.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/OrientationUtilityFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string OrientationUtility::name() const
{
  return FilterTraits<OrientationUtility>::name.str();
}

//------------------------------------------------------------------------------
std::string OrientationUtility::className() const
{
  return FilterTraits<OrientationUtility>::className;
}

//------------------------------------------------------------------------------
Uuid OrientationUtility::uuid() const
{
  return FilterTraits<OrientationUtility>::uuid;
}

//------------------------------------------------------------------------------
std::string OrientationUtility::humanName() const
{
  return "Orientation Utility";
}

//------------------------------------------------------------------------------
std::vector<std::string> OrientationUtility::defaultTags() const
{
  return {"#Utilities", "#OrientationAnalysis"};
}

//------------------------------------------------------------------------------
Parameters OrientationUtility::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<OrientationUtilityFilterParameter>(k__Key, "", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer OrientationUtility::clone() const
{
  return std::make_unique<OrientationUtility>();
}

//------------------------------------------------------------------------------
Result<OutputActions> OrientationUtility::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<OrientationUtilityAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> OrientationUtility::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
