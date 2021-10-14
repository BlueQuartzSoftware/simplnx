#include "CreateLambertSphere.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"

using namespace complex;

namespace complex
{
std::string CreateLambertSphere::name() const
{
  return FilterTraits<CreateLambertSphere>::name.str();
}

Uuid CreateLambertSphere::uuid() const
{
  return FilterTraits<CreateLambertSphere>::uuid;
}

std::string CreateLambertSphere::humanName() const
{
  return "Create Sphere (Lambert Projection Technique)";
}

Parameters CreateLambertSphere::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Hemisphere_Key, "Select Hemisphere to Generate", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateVertexGeometry_Key, "Create Vertex Geometry", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexDataContainerName_Key, "Output Vertex DataContainer Name", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateEdgeGeometry_Key, "Create Edge Geometry", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_EdgeDataContainerName_Key, "Output Edge DataContainer Name", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateTriangleGeometry_Key, "Create Triangle Geometry", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_TriangleDataContainerName_Key, "Output Triangle DataContainer Name", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateQuadGeometry_Key, "Create Quad Geometry", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_QuadDataContainerName_Key, "Output Quad DataContainer Name", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CreateVertexGeometry_Key, k_VertexDataContainerName_Key, true);
  params.linkParameters(k_CreateEdgeGeometry_Key, k_EdgeDataContainerName_Key, true);
  params.linkParameters(k_CreateTriangleGeometry_Key, k_TriangleDataContainerName_Key, true);
  params.linkParameters(k_CreateQuadGeometry_Key, k_QuadDataContainerName_Key, true);

  return params;
}

IFilter::UniquePointer CreateLambertSphere::clone() const
{
  return std::make_unique<CreateLambertSphere>();
}

Result<OutputActions> CreateLambertSphere::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pHemisphereValue = filterArgs.value<ChoicesParameter::ValueType>(k_Hemisphere_Key);
  auto pCreateVertexGeometryValue = filterArgs.value<bool>(k_CreateVertexGeometry_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pCreateEdgeGeometryValue = filterArgs.value<bool>(k_CreateEdgeGeometry_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pCreateTriangleGeometryValue = filterArgs.value<bool>(k_CreateTriangleGeometry_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pCreateQuadGeometryValue = filterArgs.value<bool>(k_CreateQuadGeometry_Key);
  auto pQuadDataContainerNameValue = filterArgs.value<DataPath>(k_QuadDataContainerName_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateLambertSphereAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CreateLambertSphere::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pHemisphereValue = filterArgs.value<ChoicesParameter::ValueType>(k_Hemisphere_Key);
  auto pCreateVertexGeometryValue = filterArgs.value<bool>(k_CreateVertexGeometry_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pCreateEdgeGeometryValue = filterArgs.value<bool>(k_CreateEdgeGeometry_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pCreateTriangleGeometryValue = filterArgs.value<bool>(k_CreateTriangleGeometry_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pCreateQuadGeometryValue = filterArgs.value<bool>(k_CreateQuadGeometry_Key);
  auto pQuadDataContainerNameValue = filterArgs.value<DataPath>(k_QuadDataContainerName_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
