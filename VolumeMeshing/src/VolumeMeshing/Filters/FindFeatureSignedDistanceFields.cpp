#include "FindFeatureSignedDistanceFields.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureSignedDistanceFields::name() const
{
  return FilterTraits<FindFeatureSignedDistanceFields>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureSignedDistanceFields::className() const
{
  return FilterTraits<FindFeatureSignedDistanceFields>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureSignedDistanceFields::uuid() const
{
  return FilterTraits<FindFeatureSignedDistanceFields>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureSignedDistanceFields::humanName() const
{
  return "Find Feature Signed Distance Fields";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureSignedDistanceFields::defaultTags() const
{
  return {"#Volume Meshing", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureSignedDistanceFields::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_SignedDistanceFieldsPrefix_Key, "Signed Distance Fields Prefix", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureSignedDistanceFields::clone() const
{
  return std::make_unique<FindFeatureSignedDistanceFields>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureSignedDistanceFields::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pSignedDistanceFieldsPrefixValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldsPrefix_Key);

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
  auto action = std::make_unique<FindFeatureSignedDistanceFieldsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindFeatureSignedDistanceFields::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pSignedDistanceFieldsPrefixValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldsPrefix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
