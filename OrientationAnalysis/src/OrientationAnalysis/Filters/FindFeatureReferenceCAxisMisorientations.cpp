#include "FindFeatureReferenceCAxisMisorientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::name() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::className() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureReferenceCAxisMisorientations::uuid() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::humanName() const
{
  return "Find Feature Reference C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureReferenceCAxisMisorientations::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureReferenceCAxisMisorientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureAvgCAxisMisorientationsArrayName_Key, "Average C-Axis Misorientations", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureStdevCAxisMisorientationsArrayName_Key, "Feature Stdev C-Axis Misorientations", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureReferenceCAxisMisorientationsArrayName_Key, "Feature Reference C-Axis Misorientations", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureReferenceCAxisMisorientations::clone() const
{
  return std::make_unique<FindFeatureReferenceCAxisMisorientations>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindFeatureReferenceCAxisMisorientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStdevCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeatureReferenceCAxisMisorientationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureReferenceCAxisMisorientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStdevCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
