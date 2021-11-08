#include "FindFeaturePhasesBinary.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinary::name() const
{
  return FilterTraits<FindFeaturePhasesBinary>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinary::className() const
{
  return FilterTraits<FindFeaturePhasesBinary>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeaturePhasesBinary::uuid() const
{
  return FilterTraits<FindFeaturePhasesBinary>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinary::humanName() const
{
  return "Find Feature Phases Binary";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeaturePhasesBinary::defaultTags() const
{
  return {"#Generic", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeaturePhasesBinary::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeaturePhasesBinary::clone() const
{
  return std::make_unique<FindFeaturePhasesBinary>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeaturePhasesBinary::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeaturePhasesBinaryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindFeaturePhasesBinary::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
