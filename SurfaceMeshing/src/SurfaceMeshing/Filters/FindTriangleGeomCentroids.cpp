#include "FindTriangleGeomCentroids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroids::name() const
{
  return FilterTraits<FindTriangleGeomCentroids>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroids::className() const
{
  return FilterTraits<FindTriangleGeomCentroids>::className;
}

//------------------------------------------------------------------------------
Uuid FindTriangleGeomCentroids::uuid() const
{
  return FilterTraits<FindTriangleGeomCentroids>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroids::humanName() const
{
  return "Find Feature Centroids from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTriangleGeomCentroids::defaultTags() const
{
  return {"#Generic", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindTriangleGeomCentroids::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixName_Key, "Face Feature Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CentroidsArrayName_Key, "Centroids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTriangleGeomCentroids::clone() const
{
  return std::make_unique<FindTriangleGeomCentroids>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindTriangleGeomCentroids::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pCentroidsArrayNameValue = filterArgs.value<DataPath>(k_CentroidsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindTriangleGeomCentroidsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindTriangleGeomCentroids::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pCentroidsArrayNameValue = filterArgs.value<DataPath>(k_CentroidsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
