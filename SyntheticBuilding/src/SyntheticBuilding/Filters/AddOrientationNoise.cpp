#include "AddOrientationNoise.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AddOrientationNoise::name() const
{
  return FilterTraits<AddOrientationNoise>::name.str();
}

//------------------------------------------------------------------------------
std::string AddOrientationNoise::className() const
{
  return FilterTraits<AddOrientationNoise>::className;
}

//------------------------------------------------------------------------------
Uuid AddOrientationNoise::uuid() const
{
  return FilterTraits<AddOrientationNoise>::uuid;
}

//------------------------------------------------------------------------------
std::string AddOrientationNoise::humanName() const
{
  return "Add Orientation Noise";
}

//------------------------------------------------------------------------------
std::vector<std::string> AddOrientationNoise::defaultTags() const
{
  return {"#Synthetic Building", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters AddOrientationNoise::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_Magnitude_Key, "Magnitude of Orientation Noise (Degrees)", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AddOrientationNoise::clone() const
{
  return std::make_unique<AddOrientationNoise>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AddOrientationNoise::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMagnitudeValue = filterArgs.value<float32>(k_Magnitude_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

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
  auto action = std::make_unique<AddOrientationNoiseAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> AddOrientationNoise::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMagnitudeValue = filterArgs.value<float32>(k_Magnitude_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
