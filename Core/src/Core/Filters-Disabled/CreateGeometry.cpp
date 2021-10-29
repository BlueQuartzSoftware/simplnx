#include "CreateGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateGeometry::name() const
{
  return FilterTraits<CreateGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateGeometry::className() const
{
  return FilterTraits<CreateGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CreateGeometry::uuid() const
{
  return FilterTraits<CreateGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateGeometry::humanName() const
{
  return "Create Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateGeometry::defaultTags() const
{
  return {"#Core", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters CreateGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_GeometryType_Key, "Geometry Type", "", 0, ChoicesParameter::Choices{"Image", "Rectilinear Grid", "Vertex", "Edge", "Triangle", "Quadrilateral", "Tetrahedral", "Hexahedral"}));
  params.insert(std::make_unique<BoolParameter>(k_TreatWarningsAsErrors_Key, "Treat Geometry Warnings as Errors", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ArrayHandling_Key, "Array Handling", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageCellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_XBoundsArrayPath_Key, "X Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_YBoundsArrayPath_Key, "Y Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ZBoundsArrayPath_Key, "Z Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_RectGridCellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_BoxDimensions_Key, "Box Size in Length Units", "", {}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath0_Key, "Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName0_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath1_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedEdgeListArrayPath_Key, "Edge List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName1_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath2_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedTriListArrayPath_Key, "Triangle List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName2_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceAttributeMatrixName0_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath3_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedQuadListArrayPath_Key, "Quadrilateral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName3_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceAttributeMatrixName1_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath4_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedTetListArrayPath_Key, "Tetrahedral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName4_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TetCellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedVertexListArrayPath5_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SharedHexListArrayPath_Key, "Hexahedral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName5_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_HexCellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GeometryType_Key, k_Dimensions_Key, 0);
  params.linkParameters(k_GeometryType_Key, k_Origin_Key, 1);
  params.linkParameters(k_GeometryType_Key, k_Spacing_Key, 2);
  params.linkParameters(k_GeometryType_Key, k_BoxDimensions_Key, 3);
  params.linkParameters(k_GeometryType_Key, k_ImageCellAttributeMatrixName_Key, 4);
  params.linkParameters(k_GeometryType_Key, k_XBoundsArrayPath_Key, 5);
  params.linkParameters(k_GeometryType_Key, k_YBoundsArrayPath_Key, 6);
  params.linkParameters(k_GeometryType_Key, k_ZBoundsArrayPath_Key, 7);
  params.linkParameters(k_GeometryType_Key, k_RectGridCellAttributeMatrixName_Key, 8);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath0_Key, 9);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName0_Key, 10);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath1_Key, 11);
  params.linkParameters(k_GeometryType_Key, k_SharedEdgeListArrayPath_Key, 12);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName1_Key, 13);
  params.linkParameters(k_GeometryType_Key, k_EdgeAttributeMatrixName_Key, 14);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath2_Key, 15);
  params.linkParameters(k_GeometryType_Key, k_SharedTriListArrayPath_Key, 16);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName2_Key, 17);
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName0_Key, 18);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath3_Key, 19);
  params.linkParameters(k_GeometryType_Key, k_SharedQuadListArrayPath_Key, 20);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName3_Key, 21);
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName1_Key, 22);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath4_Key, 23);
  params.linkParameters(k_GeometryType_Key, k_SharedTetListArrayPath_Key, 24);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName4_Key, 25);
  params.linkParameters(k_GeometryType_Key, k_TetCellAttributeMatrixName_Key, 26);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath5_Key, 27);
  params.linkParameters(k_GeometryType_Key, k_SharedHexListArrayPath_Key, 28);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName5_Key, 29);
  params.linkParameters(k_GeometryType_Key, k_HexCellAttributeMatrixName_Key, 30);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateGeometry::clone() const
{
  return std::make_unique<CreateGeometry>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CreateGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGeometryTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_GeometryType_Key);
  auto pTreatWarningsAsErrorsValue = filterArgs.value<bool>(k_TreatWarningsAsErrors_Key);
  auto pArrayHandlingValue = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pImageCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_ImageCellAttributeMatrixName_Key);
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pRectGridCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_RectGridCellAttributeMatrixName_Key);
  auto pBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pVertexAttributeMatrixName0Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pVertexAttributeMatrixName1Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName1_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pVertexAttributeMatrixName2Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName2_Key);
  auto pFaceAttributeMatrixName0Value = filterArgs.value<DataPath>(k_FaceAttributeMatrixName0_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pVertexAttributeMatrixName3Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName3_Key);
  auto pFaceAttributeMatrixName1Value = filterArgs.value<DataPath>(k_FaceAttributeMatrixName1_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pVertexAttributeMatrixName4Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName4_Key);
  auto pTetCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TetCellAttributeMatrixName_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);
  auto pVertexAttributeMatrixName5Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName5_Key);
  auto pHexCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_HexCellAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGeometryTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_GeometryType_Key);
  auto pTreatWarningsAsErrorsValue = filterArgs.value<bool>(k_TreatWarningsAsErrors_Key);
  auto pArrayHandlingValue = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pImageCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_ImageCellAttributeMatrixName_Key);
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pRectGridCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_RectGridCellAttributeMatrixName_Key);
  auto pBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pVertexAttributeMatrixName0Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pVertexAttributeMatrixName1Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName1_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pVertexAttributeMatrixName2Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName2_Key);
  auto pFaceAttributeMatrixName0Value = filterArgs.value<DataPath>(k_FaceAttributeMatrixName0_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pVertexAttributeMatrixName3Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName3_Key);
  auto pFaceAttributeMatrixName1Value = filterArgs.value<DataPath>(k_FaceAttributeMatrixName1_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pVertexAttributeMatrixName4Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName4_Key);
  auto pTetCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TetCellAttributeMatrixName_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);
  auto pVertexAttributeMatrixName5Value = filterArgs.value<DataPath>(k_VertexAttributeMatrixName5_Key);
  auto pHexCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_HexCellAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
