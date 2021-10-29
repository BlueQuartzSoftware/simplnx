#include "FindTriangleGeomNeighbors.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTriangleGeomNeighbors::name() const
{
  return FilterTraits<FindTriangleGeomNeighbors>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomNeighbors::className() const
{
  return FilterTraits<FindTriangleGeomNeighbors>::className;
}

//------------------------------------------------------------------------------
Uuid FindTriangleGeomNeighbors::uuid() const
{
  return FilterTraits<FindTriangleGeomNeighbors>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomNeighbors::humanName() const
{
  return "Find Feature Neighbors from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTriangleGeomNeighbors::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindTriangleGeomNeighbors::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Face Feature Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumNeighborsArrayName_Key, "Number of Neighbors", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborListArrayName_Key, "Neighbor List", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTriangleGeomNeighbors::clone() const
{
  return std::make_unique<FindTriangleGeomNeighbors>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindTriangleGeomNeighbors::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pNumNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  auto pNeighborListArrayNameValue = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindTriangleGeomNeighborsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindTriangleGeomNeighbors::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pNumNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  auto pNeighborListArrayNameValue = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
