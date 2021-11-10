#include "ExtractTripleLinesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractTripleLinesFromTriangleGeometry::name() const
{
  return FilterTraits<ExtractTripleLinesFromTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractTripleLinesFromTriangleGeometry::className() const
{
  return FilterTraits<ExtractTripleLinesFromTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractTripleLinesFromTriangleGeometry::uuid() const
{
  return FilterTraits<ExtractTripleLinesFromTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractTripleLinesFromTriangleGeometry::humanName() const
{
  return "Extract Triple Lines from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractTripleLinesFromTriangleGeometry::defaultTags() const
{
  return {"#Sampling", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters ExtractTripleLinesFromTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_SmoothTripleLines_Key, "Compactify Triple Lines", "", false));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_EdgeGeometry_Key, "Edge Geometry", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_NodeTypesArrayName_Key, "Node Types", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<StringParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractTripleLinesFromTriangleGeometry::clone() const
{
  return std::make_unique<ExtractTripleLinesFromTriangleGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractTripleLinesFromTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSmoothTripleLinesValue = filterArgs.value<bool>(k_SmoothTripleLines_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pEdgeGeometryValue = filterArgs.value<StringParameter::ValueType>(k_EdgeGeometry_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pNodeTypesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NodeTypesArrayName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeAttributeMatrixName_Key);

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
  auto action = std::make_unique<ExtractTripleLinesFromTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExtractTripleLinesFromTriangleGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSmoothTripleLinesValue = filterArgs.value<bool>(k_SmoothTripleLines_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pEdgeGeometryValue = filterArgs.value<StringParameter::ValueType>(k_EdgeGeometry_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pNodeTypesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NodeTypesArrayName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
