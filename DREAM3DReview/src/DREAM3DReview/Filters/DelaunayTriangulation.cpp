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
//------------------------------------------------------------------------------
std::string DelaunayTriangulation::name() const
{
  return FilterTraits<DelaunayTriangulation>::name.str();
}

//------------------------------------------------------------------------------
std::string DelaunayTriangulation::className() const
{
  return FilterTraits<DelaunayTriangulation>::className;
}

//------------------------------------------------------------------------------
Uuid DelaunayTriangulation::uuid() const
{
  return FilterTraits<DelaunayTriangulation>::uuid;
}

//------------------------------------------------------------------------------
std::string DelaunayTriangulation::humanName() const
{
  return "Delaunay Triangulation";
}

//------------------------------------------------------------------------------
std::vector<std::string> DelaunayTriangulation::defaultTags() const
{
  return {"#Surface Meshing", "#Generation"};
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
IFilter::UniquePointer DelaunayTriangulation::clone() const
{
  return std::make_unique<DelaunayTriangulation>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DelaunayTriangulation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOffsetValue = filterArgs.value<float64>(k_Offset_Key);
  auto pToleranceValue = filterArgs.value<float64>(k_Tolerance_Key);
  auto pTriangulateByFeatureValue = filterArgs.value<bool>(k_TriangulateByFeature_Key);
  auto pInputGeometryValue = filterArgs.value<DataPath>(k_InputGeometry_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_FaceAttributeMatrixName_Key);

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
  auto action = std::make_unique<DelaunayTriangulationAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> DelaunayTriangulation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
