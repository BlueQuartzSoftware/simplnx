#include "CreateLambertSphere.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateLambertSphere::name() const
{
  return FilterTraits<CreateLambertSphere>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateLambertSphere::className() const
{
  return FilterTraits<CreateLambertSphere>::className;
}

//------------------------------------------------------------------------------
Uuid CreateLambertSphere::uuid() const
{
  return FilterTraits<CreateLambertSphere>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateLambertSphere::humanName() const
{
  return "Create Sphere (Lambert Projection Technique)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateLambertSphere::defaultTags() const
{
  return {"Surface Meshing", "Generation"};
}

//------------------------------------------------------------------------------
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CreateVertexGeometry_Key, k_VertexDataContainerName_Key, true);
  params.linkParameters(k_CreateEdgeGeometry_Key, k_EdgeDataContainerName_Key, true);
  params.linkParameters(k_CreateTriangleGeometry_Key, k_TriangleDataContainerName_Key, true);
  params.linkParameters(k_CreateQuadGeometry_Key, k_QuadDataContainerName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateLambertSphere::clone() const
{
  return std::make_unique<CreateLambertSphere>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateLambertSphere::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
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

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pVertexDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pEdgeDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pTriangleDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pQuadDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateLambertSphere::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
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
