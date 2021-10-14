#include "ExtractTripleLinesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ExtractTripleLinesFromTriangleGeometry::name() const
{
  return FilterTraits<ExtractTripleLinesFromTriangleGeometry>::name.str();
}

Uuid ExtractTripleLinesFromTriangleGeometry::uuid() const
{
  return FilterTraits<ExtractTripleLinesFromTriangleGeometry>::uuid;
}

std::string ExtractTripleLinesFromTriangleGeometry::humanName() const
{
  return "Extract Triple Lines from Triangle Geometry";
}

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

IFilter::UniquePointer ExtractTripleLinesFromTriangleGeometry::clone() const
{
  return std::make_unique<ExtractTripleLinesFromTriangleGeometry>();
}

Result<OutputActions> ExtractTripleLinesFromTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSmoothTripleLinesValue = filterArgs.value<bool>(k_SmoothTripleLines_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pEdgeGeometryValue = filterArgs.value<StringParameter::ValueType>(k_EdgeGeometry_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pNodeTypesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NodeTypesArrayName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExtractTripleLinesFromTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ExtractTripleLinesFromTriangleGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
