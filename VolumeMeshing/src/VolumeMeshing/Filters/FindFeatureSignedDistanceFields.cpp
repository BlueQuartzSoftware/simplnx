#include "FindFeatureSignedDistanceFields.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindFeatureSignedDistanceFields::name() const
{
  return FilterTraits<FindFeatureSignedDistanceFields>::name.str();
}

Uuid FindFeatureSignedDistanceFields::uuid() const
{
  return FilterTraits<FindFeatureSignedDistanceFields>::uuid;
}

std::string FindFeatureSignedDistanceFields::humanName() const
{
  return "Find Feature Signed Distance Fields";
}

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

IFilter::UniquePointer FindFeatureSignedDistanceFields::clone() const
{
  return std::make_unique<FindFeatureSignedDistanceFields>();
}

Result<OutputActions> FindFeatureSignedDistanceFields::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pSignedDistanceFieldsPrefixValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldsPrefix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeatureSignedDistanceFieldsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindFeatureSignedDistanceFields::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
