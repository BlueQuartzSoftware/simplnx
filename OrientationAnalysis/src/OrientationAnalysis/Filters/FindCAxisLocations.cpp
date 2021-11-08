#include "FindCAxisLocations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindCAxisLocations::name() const
{
  return FilterTraits<FindCAxisLocations>::name.str();
}

//------------------------------------------------------------------------------
std::string FindCAxisLocations::className() const
{
  return FilterTraits<FindCAxisLocations>::className;
}

//------------------------------------------------------------------------------
Uuid FindCAxisLocations::uuid() const
{
  return FilterTraits<FindCAxisLocations>::uuid;
}

//------------------------------------------------------------------------------
std::string FindCAxisLocations::humanName() const
{
  return "Find C-Axis Locations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindCAxisLocations::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindCAxisLocations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CAxisLocationsArrayName_Key, "C-Axis Locations", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindCAxisLocations::clone() const
{
  return std::make_unique<FindCAxisLocations>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindCAxisLocations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCAxisLocationsArrayNameValue = filterArgs.value<DataPath>(k_CAxisLocationsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindCAxisLocationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindCAxisLocations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCAxisLocationsArrayNameValue = filterArgs.value<DataPath>(k_CAxisLocationsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
