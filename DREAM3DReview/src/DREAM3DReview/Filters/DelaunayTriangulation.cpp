#include "DelaunayTriangulation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string DelaunayTriangulation::name() const
{
  return FilterTraits<DelaunayTriangulation>::name.str();
}

Uuid DelaunayTriangulation::uuid() const
{
  return FilterTraits<DelaunayTriangulation>::uuid;
}

std::string DelaunayTriangulation::humanName() const
{
  return "Delaunay Triangulation";
}

Parameters DelaunayTriangulation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Offset_Key, "Offset", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Tolerance_Key, "Tolerance", "", 2.3456789));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_TriangulateByFeature_Key, "Triangulate by Feature", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_InputGeometry_Key, "Input Vertices", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_TriangleDataContainerName_Key, "Triangle Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<StringParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_TriangulateByFeature_Key, k_FeatureIdsArrayPath_Key, true);

  return params;
}

IFilter::UniquePointer DelaunayTriangulation::clone() const
{
  return std::make_unique<DelaunayTriangulation>();
}

Result<OutputActions> DelaunayTriangulation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOffsetValue = filterArgs.value<float64>(k_Offset_Key);
  auto pToleranceValue = filterArgs.value<float64>(k_Tolerance_Key);
  auto pTriangulateByFeatureValue = filterArgs.value<bool>(k_TriangulateByFeature_Key);
  auto pInputGeometryValue = filterArgs.value<DataPath>(k_InputGeometry_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_FaceAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DelaunayTriangulationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> DelaunayTriangulation::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOffsetValue = filterArgs.value<float64>(k_Offset_Key);
  auto pToleranceValue = filterArgs.value<float64>(k_Tolerance_Key);
  auto pTriangulateByFeatureValue = filterArgs.value<bool>(k_TriangulateByFeature_Key);
  auto pInputGeometryValue = filterArgs.value<DataPath>(k_InputGeometry_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_FaceAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
