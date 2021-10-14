#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ExtractInternalSurfacesFromTriangleGeometry::name() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::name.str();
}

Uuid ExtractInternalSurfacesFromTriangleGeometry::uuid() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::uuid;
}

std::string ExtractInternalSurfacesFromTriangleGeometry::humanName() const
{
  return "Extract Internal Surfaces from Triangle Geometry";
}

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

IFilter::UniquePointer ExtractInternalSurfacesFromTriangleGeometry::clone() const
{
  return std::make_unique<ExtractInternalSurfacesFromTriangleGeometry>();
}

Result<OutputActions> ExtractInternalSurfacesFromTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pInternalTrianglesNameValue = filterArgs.value<StringParameter::ValueType>(k_InternalTrianglesName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExtractInternalSurfacesFromTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ExtractInternalSurfacesFromTriangleGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
