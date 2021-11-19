#include "CreateGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
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
  params.linkParameters(k_GeometryType_Key, k_Origin_Key, 0);
  params.linkParameters(k_GeometryType_Key, k_Spacing_Key, 0);
  params.linkParameters(k_GeometryType_Key, k_ImageCellAttributeMatrixName_Key, 0);
  params.linkParameters(k_GeometryType_Key, k_XBoundsArrayPath_Key, 1);
  params.linkParameters(k_GeometryType_Key, k_YBoundsArrayPath_Key, 1);
  params.linkParameters(k_GeometryType_Key, k_ZBoundsArrayPath_Key, 1);
  params.linkParameters(k_GeometryType_Key, k_RectGridCellAttributeMatrixName_Key, 1);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath0_Key, 2);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName0_Key, 2);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath1_Key, 3);
  params.linkParameters(k_GeometryType_Key, k_SharedEdgeListArrayPath_Key, 3);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName1_Key, 3);
  params.linkParameters(k_GeometryType_Key, k_EdgeAttributeMatrixName_Key, 3);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath2_Key, 4);
  params.linkParameters(k_GeometryType_Key, k_SharedTriListArrayPath_Key, 4);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName2_Key, 4);
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName0_Key, 4);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath3_Key, 5);
  params.linkParameters(k_GeometryType_Key, k_SharedQuadListArrayPath_Key, 5);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName3_Key, 5);
  params.linkParameters(k_GeometryType_Key, k_FaceAttributeMatrixName1_Key, 5);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath4_Key, 6);
  params.linkParameters(k_GeometryType_Key, k_SharedTetListArrayPath_Key, 6);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName4_Key, 6);
  params.linkParameters(k_GeometryType_Key, k_TetCellAttributeMatrixName_Key, 6);
  params.linkParameters(k_GeometryType_Key, k_SharedVertexListArrayPath5_Key, 7);
  params.linkParameters(k_GeometryType_Key, k_SharedHexListArrayPath_Key, 7);
  params.linkParameters(k_GeometryType_Key, k_VertexAttributeMatrixName5_Key, 7);
  params.linkParameters(k_GeometryType_Key, k_HexCellAttributeMatrixName_Key, 7);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateGeometry::clone() const
{
  return std::make_unique<CreateGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string boxDimensions;

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
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"BoxDimensions", boxDimensions});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
