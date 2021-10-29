#include "FindFeatureReferenceMisorientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientations::name() const
{
  return FilterTraits<FindFeatureReferenceMisorientations>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientations::className() const
{
  return FilterTraits<FindFeatureReferenceMisorientations>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureReferenceMisorientations::uuid() const
{
  return FilterTraits<FindFeatureReferenceMisorientations>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientations::humanName() const
{
  return "Find Feature Reference Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureReferenceMisorientations::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureReferenceMisorientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ReferenceOrientation_Key, "Reference Orientation", "", 0, ChoicesParameter::Choices{"Average Orientation", "Orientation at Feature Centroid"}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBEuclideanDistancesArrayPath_Key, "Boundary Euclidean Distances", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureReferenceMisorientationsArrayName_Key, "Feature Reference Misorientations", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureAvgMisorientationsArrayName_Key, "Average Misorientations", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReferenceOrientation_Key, k_GBEuclideanDistancesArrayPath_Key, 0);
  params.linkParameters(k_ReferenceOrientation_Key, k_AvgQuatsArrayPath_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureReferenceMisorientations::clone() const
{
  return std::make_unique<FindFeatureReferenceMisorientations>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindFeatureReferenceMisorientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pReferenceOrientationValue = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceOrientation_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFeatureReferenceMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceMisorientationsArrayName_Key);
  auto pFeatureAvgMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgMisorientationsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeatureReferenceMisorientationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureReferenceMisorientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReferenceOrientationValue = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceOrientation_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFeatureReferenceMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceMisorientationsArrayName_Key);
  auto pFeatureAvgMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgMisorientationsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
