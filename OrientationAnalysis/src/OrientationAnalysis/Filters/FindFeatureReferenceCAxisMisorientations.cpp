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
IFilter::PreflightResult FindFeatureReferenceCAxisMisorientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStdevCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

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
  auto action = std::make_unique<FindFeatureReferenceCAxisMisorientationsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
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
