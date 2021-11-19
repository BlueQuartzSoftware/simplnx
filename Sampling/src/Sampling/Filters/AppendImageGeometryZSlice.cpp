#include "AppendImageGeometryZSlice.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
