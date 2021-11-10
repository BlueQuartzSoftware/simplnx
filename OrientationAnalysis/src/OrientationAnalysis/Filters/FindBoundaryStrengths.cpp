#include "FindBoundaryStrengths.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryStrengths::name() const
{
  return FilterTraits<FindBoundaryStrengths>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryStrengths::className() const
{
  return FilterTraits<FindBoundaryStrengths>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryStrengths::uuid() const
{
  return FilterTraits<FindBoundaryStrengths>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryStrengths::humanName() const
{
  return "Find Feature Boundary Strength Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryStrengths::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryStrengths::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Loading_Key, "Loading Direction (XYZ)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshF1sArrayName_Key, "F1s", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshF1sptsArrayName_Key, "F1spts", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshF7sArrayName_Key, "F7s", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshmPrimesArrayName_Key, "mPrimes", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryStrengths::clone() const
{
  return std::make_unique<FindBoundaryStrengths>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryStrengths::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pLoadingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Loading_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshF1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sArrayName_Key);
  auto pSurfaceMeshF1sptsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sptsArrayName_Key);
  auto pSurfaceMeshF7sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF7sArrayName_Key);
  auto pSurfaceMeshmPrimesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshmPrimesArrayName_Key);

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
  auto action = std::make_unique<FindBoundaryStrengthsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindBoundaryStrengths::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLoadingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Loading_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshF1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sArrayName_Key);
  auto pSurfaceMeshF1sptsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sptsArrayName_Key);
  auto pSurfaceMeshF7sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF7sArrayName_Key);
  auto pSurfaceMeshmPrimesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshmPrimesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
