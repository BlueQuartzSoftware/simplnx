#include "MatchCrystallography.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MatchCrystallography::name() const
{
  return FilterTraits<MatchCrystallography>::name.str();
}

//------------------------------------------------------------------------------
std::string MatchCrystallography::className() const
{
  return FilterTraits<MatchCrystallography>::className;
}

//------------------------------------------------------------------------------
Uuid MatchCrystallography::uuid() const
{
  return FilterTraits<MatchCrystallography>::uuid;
}

//------------------------------------------------------------------------------
std::string MatchCrystallography::humanName() const
{
  return "Match Crystallography";
}

//------------------------------------------------------------------------------
std::vector<std::string> MatchCrystallography::defaultTags() const
{
  return {"#Synthetic Building", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters MatchCrystallography::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_MaxIterations_Key, "Maximum Number of Iterations (Swaps)", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath({"FeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{complex::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceFeaturesArrayPath_Key, "Surface Features", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedSurfaceAreaListArrayPath_Key, "Shared Surface Area List", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEulerAnglesArrayName_Key, "Euler Angles", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesArrayName_Key, "Volumes", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureEulerAnglesArrayName_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgQuatsArrayName_Key, "Average Quaternions", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MatchCrystallography::clone() const
{
  return std::make_unique<MatchCrystallography>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MatchCrystallography::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMaxIterationsValue = filterArgs.value<int32>(k_MaxIterations_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pSharedSurfaceAreaListArrayPathValue = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pAvgQuatsArrayNameValue = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MatchCrystallography::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaxIterationsValue = filterArgs.value<int32>(k_MaxIterations_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pSharedSurfaceAreaListArrayPathValue = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pAvgQuatsArrayNameValue = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
