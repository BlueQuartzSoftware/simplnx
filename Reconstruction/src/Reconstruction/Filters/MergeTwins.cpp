#include "MergeTwins.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeTwins::name() const
{
  return FilterTraits<MergeTwins>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeTwins::className() const
{
  return FilterTraits<MergeTwins>::className;
}

//------------------------------------------------------------------------------
Uuid MergeTwins::uuid() const
{
  return FilterTraits<MergeTwins>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeTwins::humanName() const
{
  return "Merge Twins";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeTwins::defaultTags() const
{
  return {"#Reconstruction", "#Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeTwins::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "", false));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeTwins::clone() const
{
  return std::make_unique<MergeTwins>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeTwins::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

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
  auto action = std::make_unique<MergeTwinsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MergeTwins::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
