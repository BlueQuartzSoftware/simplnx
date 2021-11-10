#include "AppendImageGeometryZSlice.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AppendImageGeometryZSlice::name() const
{
  return FilterTraits<AppendImageGeometryZSlice>::name.str();
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSlice::className() const
{
  return FilterTraits<AppendImageGeometryZSlice>::className;
}

//------------------------------------------------------------------------------
Uuid AppendImageGeometryZSlice::uuid() const
{
  return FilterTraits<AppendImageGeometryZSlice>::uuid;
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSlice::humanName() const
{
  return "Append Z Slice (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AppendImageGeometryZSlice::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters AppendImageGeometryZSlice::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_InputAttributeMatrix_Key, "Input Cell Data", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestinationAttributeMatrix_Key, "Destination Cell Data", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_CheckResolution_Key, "Check Spacing", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AppendImageGeometryZSlice::clone() const
{
  return std::make_unique<AppendImageGeometryZSlice>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AppendImageGeometryZSlice::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputAttributeMatrixValue = filterArgs.value<DataPath>(k_InputAttributeMatrix_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pCheckResolutionValue = filterArgs.value<bool>(k_CheckResolution_Key);

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
  auto action = std::make_unique<AppendImageGeometryZSliceAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> AppendImageGeometryZSlice::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputAttributeMatrixValue = filterArgs.value<DataPath>(k_InputAttributeMatrix_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pCheckResolutionValue = filterArgs.value<bool>(k_CheckResolution_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
