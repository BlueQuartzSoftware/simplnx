#include "FindFeatureCentroids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureCentroids::name() const
{
  return FilterTraits<FindFeatureCentroids>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureCentroids::className() const
{
  return FilterTraits<FindFeatureCentroids>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureCentroids::uuid() const
{
  return FilterTraits<FindFeatureCentroids>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureCentroids::humanName() const
{
  return "Find Feature Centroids";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureCentroids::defaultTags() const
{
  return {"#Generic", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureCentroids::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureCentroids::clone() const
{
  return std::make_unique<FindFeatureCentroids>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindFeatureCentroids::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeatureCentroidsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureCentroids::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
