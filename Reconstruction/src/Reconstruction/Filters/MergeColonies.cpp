#include "MergeColonies.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeColonies::name() const
{
  return FilterTraits<MergeColonies>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeColonies::className() const
{
  return FilterTraits<MergeColonies>::className;
}

//------------------------------------------------------------------------------
Uuid MergeColonies::uuid() const
{
  return FilterTraits<MergeColonies>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeColonies::humanName() const
{
  return "Merge Colonies";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeColonies::defaultTags() const
{
  return {"#Reconstruction", "#Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeColonies::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "", false));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_IdentifyGlobAlpha_Key, "Identify Glob Alpha", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_GlobAlphaArrayName_Key, "Glob Alpha", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_IdentifyGlobAlpha_Key, k_GlobAlphaArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeColonies::clone() const
{
  return std::make_unique<MergeColonies>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeColonies::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pIdentifyGlobAlphaValue = filterArgs.value<bool>(k_IdentifyGlobAlpha_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pGlobAlphaArrayNameValue = filterArgs.value<DataPath>(k_GlobAlphaArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

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
Result<> MergeColonies::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pIdentifyGlobAlphaValue = filterArgs.value<bool>(k_IdentifyGlobAlpha_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pGlobAlphaArrayNameValue = filterArgs.value<DataPath>(k_GlobAlphaArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
