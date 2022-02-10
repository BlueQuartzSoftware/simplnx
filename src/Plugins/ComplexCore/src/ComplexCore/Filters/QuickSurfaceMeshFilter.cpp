#include "QuickSurfaceMeshFilter.hpp"

#include "ComplexCore/Filters/Algorithms/QuickSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::name() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::className() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid QuickSurfaceMeshFilter::uuid() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::humanName() const
{
  return "Quick Surface Mesh";
}

//------------------------------------------------------------------------------
std::vector<std::string> QuickSurfaceMeshFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Generation", "#Create", "#Triangle", "#Geoemtry"};
}

//------------------------------------------------------------------------------
Parameters QuickSurfaceMeshFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_GenerateTripleLines_Key, "Generate Triple Lines", "", true));

  params.insert(std::make_unique<BoolParameter>(k_FixProblemVoxels_Key, "Attempt to Fix Problem Voxels", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});

  params.insert(std::make_unique<DataPathSelectionParameter>(k_GridGeometryDataPath_Key, "Grid Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, false, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Transfer", "", MultiArraySelectionParameter::ValueType{}));

  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ParentDataGroupPath_Key, "Parent DataGroup", "", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<StringParameter>(k_TriangleGeometryName_Key, "Geometry Name [Data Group]", "", std::string("[Triangle Geometry]")));

  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexDataGroupName_Key, "Vertex Data [DataGroup]", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NodeTypesArrayName_Key, "Node Types", "", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_FaceDataGroupName_Key, "Face Data [DataGroup]", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceLabelsArrayName_Key, "Face Labels", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer QuickSurfaceMeshFilter::clone() const
{
  return std::make_unique<QuickSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult QuickSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pGenerateTripleLines = filterArgs.value<bool>(k_GenerateTripleLines_Key);
  auto pFixProblemVoxelsValue = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  auto pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pParentDataGroupPath = filterArgs.value<DataPath>(k_ParentDataGroupPath_Key);
  auto pTriangleGeometryName = filterArgs.value<std::string>(k_TriangleGeometryName_Key);
  auto pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexDataGroupName_Key);
  auto pNodeTypesDataPath = filterArgs.value<DataPath>(k_NodeTypesArrayName_Key);
  auto pFaceGroupDataPath = filterArgs.value<DataPath>(k_FaceDataGroupName_Key);
  auto pFaceLabelsDataPath = filterArgs.value<DataPath>(k_FaceLabelsArrayName_Key);

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

  DataPath geometryDataPath = pParentDataGroupPath.createChildPath(pTriangleGeometryName);

  // Assign the outputAction to the Result<OutputActions>::actions vector via a push_back
  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point, we are going to scope each section so that we don't accidentally introduce bugs

  // Create the Triangle Geometry action and store it
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(geometryDataPath, 1, 3);
    resultOutputActions.value().actions.push_back(std::move(createTriangleGeometryAction));
  }
  // Create Triangle Vertex Data action and store it
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pVertexGroupDataPath);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // Create the face NodesType DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::int8, std::vector<usize>{3}, std::vector<usize>{1}, pNodeTypesDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create Triangle FaceData (for the Normals) action and store it
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pFaceGroupDataPath);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // Create the face Labels DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::int32, std::vector<usize>{1}, std::vector<usize>{2}, pFaceLabelsDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    auto createArrayAction = std::make_unique<CopyArrayInstanceAction>(selectedDataPath, createdDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> QuickSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  complex::QuickSurfaceMeshInputValues inputs;

  inputs.pGenerateTripleLines = filterArgs.value<bool>(k_GenerateTripleLines_Key);
  inputs.pFixProblemVoxels = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  inputs.pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  inputs.pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputs.pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputs.pParentDataGroupPath = filterArgs.value<DataPath>(k_ParentDataGroupPath_Key);
  inputs.pTriangleGeometryName = filterArgs.value<std::string>(k_TriangleGeometryName_Key);
  inputs.pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexDataGroupName_Key);
  inputs.pNodeTypesDataPath = filterArgs.value<DataPath>(k_NodeTypesArrayName_Key);
  inputs.pFaceGroupDataPath = filterArgs.value<DataPath>(k_FaceDataGroupName_Key);
  inputs.pFaceLabelsDataPath = filterArgs.value<DataPath>(k_FaceLabelsArrayName_Key);

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputs.pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = inputs.pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputs.pCreatedDataArrayPaths = createdDataPaths;

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return complex::QuickSurfaceMesh(dataStructure, &inputs, this, messageHandler)();
}
} // namespace complex
