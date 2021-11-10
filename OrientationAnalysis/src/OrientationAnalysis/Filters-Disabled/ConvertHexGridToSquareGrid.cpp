#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ConvertHexGridToSquareGridFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::name() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::className() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertHexGridToSquareGrid::uuid() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::humanName() const
{
  return "Convert Hexagonal Grid Data to Square Grid Data (TSL - .ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertHexGridToSquareGrid::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters ConvertHexGridToSquareGrid::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ConvertHexGridToSquareGridFilterParameter>(k_HexGridStack_Key, "Convert Hex Grid ANG Files", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertHexGridToSquareGrid::clone() const
{
  return std::make_unique<ConvertHexGridToSquareGrid>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertHexGridToSquareGrid::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pHexGridStackValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_HexGridStack_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ConvertHexGridToSquareGridAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGrid::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pHexGridStackValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_HexGridStack_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
