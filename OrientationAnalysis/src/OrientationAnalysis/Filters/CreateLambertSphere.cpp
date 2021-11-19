#include "CreateLambertSphere.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  return {"#Surface Meshing", "#Generation"};
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}));
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
IFilter::PreflightResult CreateLambertSphere::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateLambertSphere::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
