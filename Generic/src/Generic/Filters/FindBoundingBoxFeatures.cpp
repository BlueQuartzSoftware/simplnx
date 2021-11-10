#include "FindBoundingBoxFeatures.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundingBoxFeatures::name() const
{
  return FilterTraits<FindBoundingBoxFeatures>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundingBoxFeatures::className() const
{
  return FilterTraits<FindBoundingBoxFeatures>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundingBoxFeatures::uuid() const
{
  return FilterTraits<FindBoundingBoxFeatures>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundingBoxFeatures::humanName() const
{
  return "Find Biased Features (Bounding Box)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundingBoxFeatures::defaultTags() const
{
  return {"#Generic", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindBoundingBoxFeatures::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalcByPhase_Key, "Apply Phase by Phase", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceFeaturesArrayPath_Key, "Surface Features", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_BiasedFeaturesArrayName_Key, "Biased Features", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CalcByPhase_Key, k_PhasesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundingBoxFeatures::clone() const
{
  return std::make_unique<FindBoundingBoxFeatures>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundingBoxFeatures::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCalcByPhaseValue = filterArgs.value<bool>(k_CalcByPhase_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pPhasesArrayPathValue = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  auto pBiasedFeaturesArrayNameValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayName_Key);

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
  auto action = std::make_unique<FindBoundingBoxFeaturesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindBoundingBoxFeatures::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCalcByPhaseValue = filterArgs.value<bool>(k_CalcByPhase_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pPhasesArrayPathValue = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  auto pBiasedFeaturesArrayNameValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
