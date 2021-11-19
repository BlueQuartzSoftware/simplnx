#include "FindNeighborhoods.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNeighborhoods::name() const
{
  return FilterTraits<FindNeighborhoods>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNeighborhoods::className() const
{
  return FilterTraits<FindNeighborhoods>::className;
}

//------------------------------------------------------------------------------
Uuid FindNeighborhoods::uuid() const
{
  return FilterTraits<FindNeighborhoods>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNeighborhoods::humanName() const
{
  return "Find Feature Neighborhoods";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighborhoods::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindNeighborhoods::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_MultiplesOfAverage_Key, "Multiples of Average Diameter", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborhoodsArrayName_Key, "Neighborhoods", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborhoodListArrayName_Key, "Neighborhood List", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighborhoods::clone() const
{
  return std::make_unique<FindNeighborhoods>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighborhoods::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMultiplesOfAverageValue = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pNeighborhoodsArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodsArrayName_Key);
  auto pNeighborhoodListArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodListArrayName_Key);

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
Result<> FindNeighborhoods::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMultiplesOfAverageValue = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pNeighborhoodsArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodsArrayName_Key);
  auto pNeighborhoodListArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodListArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
