#include "ScalarSegmentFeatures.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ScalarSegmentFeatures::name() const
{
  return FilterTraits<ScalarSegmentFeatures>::name.str();
}

//------------------------------------------------------------------------------
std::string ScalarSegmentFeatures::className() const
{
  return FilterTraits<ScalarSegmentFeatures>::className;
}

//------------------------------------------------------------------------------
Uuid ScalarSegmentFeatures::uuid() const
{
  return FilterTraits<ScalarSegmentFeatures>::uuid;
}

//------------------------------------------------------------------------------
std::string ScalarSegmentFeatures::humanName() const
{
  return "Segment Features (Scalar)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ScalarSegmentFeatures::defaultTags() const
{
  return {"#Reconstruction", "#Segmentation"};
}

//------------------------------------------------------------------------------
Parameters ScalarSegmentFeatures::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_ScalarTolerance_Key, "Scalar Tolerance", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ScalarArrayPath_Key, "Scalar Array to Segment", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ScalarSegmentFeatures::clone() const
{
  return std::make_unique<ScalarSegmentFeatures>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ScalarSegmentFeatures::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pScalarToleranceValue = filterArgs.value<float32>(k_ScalarTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pScalarArrayPathValue = filterArgs.value<DataPath>(k_ScalarArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ScalarSegmentFeaturesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ScalarSegmentFeatures::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScalarToleranceValue = filterArgs.value<float32>(k_ScalarTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pScalarArrayPathValue = filterArgs.value<DataPath>(k_ScalarArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
