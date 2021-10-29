#include "MinSize.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MinSize::name() const
{
  return FilterTraits<MinSize>::name.str();
}

//------------------------------------------------------------------------------
std::string MinSize::className() const
{
  return FilterTraits<MinSize>::className;
}

//------------------------------------------------------------------------------
Uuid MinSize::uuid() const
{
  return FilterTraits<MinSize>::uuid;
}

//------------------------------------------------------------------------------
std::string MinSize::humanName() const
{
  return "Minimum Size";
}

//------------------------------------------------------------------------------
std::vector<std::string> MinSize::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters MinSize::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_MinAllowedFeatureSize_Key, "Minimum Allowed Feature Size", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_PhaseNumber_Key, "Phase Index", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsArrayPath_Key, "Number of Cells", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, true);
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhasesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MinSize::clone() const
{
  return std::make_unique<MinSize>();
}

//------------------------------------------------------------------------------
Result<OutputActions> MinSize::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMinAllowedFeatureSizeValue = filterArgs.value<int32>(k_MinAllowedFeatureSize_Key);
  auto pApplyToSinglePhaseValue = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  auto pPhaseNumberValue = filterArgs.value<int32>(k_PhaseNumber_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNumCellsArrayPathValue = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<MinSizeAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> MinSize::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinAllowedFeatureSizeValue = filterArgs.value<int32>(k_MinAllowedFeatureSize_Key);
  auto pApplyToSinglePhaseValue = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  auto pPhaseNumberValue = filterArgs.value<int32>(k_PhaseNumber_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNumCellsArrayPathValue = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
