#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometry::name() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometry::className() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractInternalSurfacesFromTriangleGeometry::uuid() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometry::humanName() const
{
  return "Extract Internal Surfaces from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractInternalSurfacesFromTriangleGeometry::defaultTags() const
{
  return {"#DREAM3D Review", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters ExtractInternalSurfacesFromTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainerName_Key, "Triangle Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_InternalTrianglesName_Key, "Internal Triangles Data Container", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractInternalSurfacesFromTriangleGeometry::clone() const
{
  return std::make_unique<ExtractInternalSurfacesFromTriangleGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractInternalSurfacesFromTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pInternalTrianglesNameValue = filterArgs.value<StringParameter::ValueType>(k_InternalTrianglesName_Key);

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
  auto action = std::make_unique<ExtractInternalSurfacesFromTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pInternalTrianglesNameValue = filterArgs.value<StringParameter::ValueType>(k_InternalTrianglesName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
