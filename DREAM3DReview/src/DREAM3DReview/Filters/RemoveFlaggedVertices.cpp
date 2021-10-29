#include "RemoveFlaggedVertices.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RemoveFlaggedVertices::name() const
{
  return FilterTraits<RemoveFlaggedVertices>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedVertices::className() const
{
  return FilterTraits<RemoveFlaggedVertices>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedVertices::uuid() const
{
  return FilterTraits<RemoveFlaggedVertices>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedVertices::humanName() const
{
  return "Remove Flagged Vertices";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedVertices::defaultTags() const
{
  return {"#DREAM3D Review", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedVertices::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexGeometry_Key, "Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_ReducedVertexGeometry_Key, "Reduced Vertex Data Container", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedVertices::clone() const
{
  return std::make_unique<RemoveFlaggedVertices>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RemoveFlaggedVertices::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pVertexGeometryValue = filterArgs.value<DataPath>(k_VertexGeometry_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pReducedVertexGeometryValue = filterArgs.value<StringParameter::ValueType>(k_ReducedVertexGeometry_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RemoveFlaggedVerticesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedVertices::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pVertexGeometryValue = filterArgs.value<DataPath>(k_VertexGeometry_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pReducedVertexGeometryValue = filterArgs.value<StringParameter::ValueType>(k_ReducedVertexGeometry_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
