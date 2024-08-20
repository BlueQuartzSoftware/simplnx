#include "RemoveFlaggedTrianglesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedTriangles.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::name() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::className() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedTrianglesFilter::uuid() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::humanName() const
{
  return "Remove Flagged Triangles";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedTrianglesFilter::defaultTags() const
{
  return {"Surface Meshing", "Cleanup", "Remove", "Delete"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedTrianglesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Triangle Geometry", "The Triangle Geometry that will be processed.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The DataArrayPath to the mask array that marks each face as either true (remove) or false(keep).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Geometry", "The name of the created Triangle Geometry", DataPath({"ReducedGeometry"})));

  // Vertex Data Handling
  params.insertSeparator(Parameters::Separator{"Vertex Data Handling"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_VertexDataHandling_Key, "Vertex Data Handling", "How to handle Data that resides on the triangles",
                                                                    detail::k_CopySelectedVertexArraysIdx, detail::k_VertexDataHandlingChoices));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_VertexDataSelectedAttributeMatrix_Key, "Vertex Data", "Vertex Attribute Matrix that will be copied to the reduced geometry", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_VertexDataSelectedArrays_Key, "Vertex Attribute Arrays to Copy", "Vertex DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.linkParameters(k_VertexDataHandling_Key, k_VertexDataSelectedAttributeMatrix_Key, detail::k_CopyAllVertexArraysIdx);
  params.linkParameters(k_VertexDataHandling_Key, k_VertexDataSelectedArrays_Key, detail::k_CopySelectedVertexArraysIdx);

  // Triangle Data Handling
  params.insertSeparator(Parameters::Separator{"Triangle Data Handling"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_TriangleDataHandling_Key, "Triangle Data Handling", "How to handle Data that resides on the triangles",
                                                                    detail::k_CopySelectedTriangleArraysIdx, detail::k_TriangleDataHandlingChoices));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_TriangleDataSelectedAttributeMatrix_Key, "Triangle Data", "Triangle Attribute Matrix that will be copied to the reduced geometry",
                                                                    DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_TriangleDataSelectedArrays_Key, "Triangle Attribute Arrays to Copy", "Triangle DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.linkParameters(k_TriangleDataHandling_Key, k_TriangleDataSelectedArrays_Key, detail::k_CopySelectedTriangleArraysIdx);
  params.linkParameters(k_TriangleDataHandling_Key, k_TriangleDataSelectedAttributeMatrix_Key, detail::k_CopyAllTriangleArraysIdx);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedTrianglesFilter::clone() const
{
  return std::make_unique<RemoveFlaggedTrianglesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RemoveFlaggedTrianglesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pInitialGeometryPathValue = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto pReducedGeometryPathValue = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto pTriangleArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_TriangleDataHandling_Key);
  auto selectedTriangleArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_TriangleDataSelectedArrays_Key);
  auto selectedTriangleAttrMatPath = filterArgs.value<DataPath>(k_TriangleDataSelectedAttributeMatrix_Key);

  auto pVertexArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_VertexDataHandling_Key);
  auto selectedVertexArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_VertexDataSelectedArrays_Key);
  auto selectedVertexAttrMatPath = filterArgs.value<DataPath>(k_VertexDataSelectedAttributeMatrix_Key);

  Result<OutputActions> resultOutputActions;

  const auto* initialGeomPtr = dataStructure.getDataAs<INodeGeometry2D>(pInitialGeometryPathValue);

  std::string reducedVertexAttributeMatrixName = (initialGeomPtr->getVertexAttributeMatrix() == nullptr ? "Vertex Data" : initialGeomPtr->getVertexAttributeMatrix()->getName());
  std::string reducedFaceAttributeMatrixName = (initialGeomPtr->getEdgeAttributeMatrix() == nullptr ? "Face Data" : initialGeomPtr->getEdgeAttributeMatrix()->getName());

  DataPath reducedVertexAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(reducedVertexAttributeMatrixName);
  DataPath reducedFaceAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(reducedFaceAttributeMatrixName);

  std::vector<usize> triangleDataShape = {initialGeomPtr->getNumberOfFaces()};
  std::vector<usize> vertexDataShape = {initialGeomPtr->getNumberOfVertices()};

  if(initialGeomPtr->getGeomType() == IGeometry::Type::Triangle)
  {
    auto createGeometryAction =
        std::make_unique<CreateGeometry2DAction<TriangleGeom>>(pReducedGeometryPathValue, initialGeomPtr->getNumberOfFaces(), initialGeomPtr->getNumberOfVertices(), reducedVertexAttributeMatrixName,
                                                               reducedFaceAttributeMatrixName, initialGeomPtr->getVertices()->getName(), initialGeomPtr->getFaces()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  if(initialGeomPtr->getGeomType() == IGeometry::Type::Quad)
  {
    auto createGeometryAction =
        std::make_unique<CreateGeometry2DAction<QuadGeom>>(pReducedGeometryPathValue, initialGeomPtr->getNumberOfFaces(), initialGeomPtr->getNumberOfVertices(), reducedVertexAttributeMatrixName,
                                                           reducedFaceAttributeMatrixName, initialGeomPtr->getVertices()->getName(), initialGeomPtr->getFaces()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  /** This section is for copying the Face Data ***/
  // This _could_ be nullptr. We are going to hold off doing that check until inside each of the
  // conditional blocks below.
  {
    const AttributeMatrix* srcTriangleAttrMatPtr = initialGeomPtr->getFaceAttributeMatrix();
    if(pTriangleArrayHandling == detail::k_CopySelectedTriangleArraysIdx)
    {
      if(!selectedTriangleArrays.empty() && nullptr == srcTriangleAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5251, fmt::format("'{}' must have face data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      TransferGeometryElementData::CreateDataArrayActions(dataStructure, srcTriangleAttrMatPtr, selectedTriangleArrays, reducedFaceAttributeMatrixPath, resultOutputActions);
    }
    else if(pTriangleArrayHandling == detail::k_CopyAllTriangleArraysIdx)
    {
      if(nullptr == srcTriangleAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5252, fmt::format("'{}' must have face data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      std::vector<DataPath> ignorePaths;

      auto getChildrenResult = GetAllChildArrayDataPaths(dataStructure, selectedTriangleAttrMatPath, ignorePaths);
      if(getChildrenResult.has_value())
      {
        selectedTriangleArrays = getChildrenResult.value();
        TransferGeometryElementData::CreateDataArrayActions(dataStructure, srcTriangleAttrMatPtr, selectedTriangleArrays, reducedFaceAttributeMatrixPath, resultOutputActions);
      }
    }
  }

  /** This section is for copying the Vertex Data ***/
  // This _could_ be nullptr. We are going to hold off doing that check until inside each of the
  // conditional blocks below.
  {
    const AttributeMatrix* srcVertexAttrMatPtr = initialGeomPtr->getVertexAttributeMatrix();
    if(pVertexArrayHandling == detail::k_CopySelectedVertexArraysIdx)
    {
      if(!selectedVertexArrays.empty() && nullptr == srcVertexAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5553, fmt::format("'{}' must have Vertex data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      TransferGeometryElementData::CreateDataArrayActions(dataStructure, srcVertexAttrMatPtr, selectedVertexArrays, reducedVertexAttributeMatrixPath, resultOutputActions);
    }
    else if(pVertexArrayHandling == detail::k_CopyAllVertexArraysIdx)
    {
      if(nullptr == srcVertexAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5554, fmt::format("'{}' must have Vertex data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      std::vector<DataPath> ignorePaths;

      auto getChildrenResult = GetAllChildArrayDataPaths(dataStructure, selectedVertexAttrMatPath, ignorePaths);
      if(getChildrenResult.has_value())
      {
        selectedVertexArrays = getChildrenResult.value();
        TransferGeometryElementData::CreateDataArrayActions(dataStructure, srcVertexAttrMatPtr, selectedVertexArrays, reducedVertexAttributeMatrixPath, resultOutputActions);
      }
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedTrianglesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  RemoveFlaggedTrianglesInputValues inputValues;

  inputValues.TriangleGeometry = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ReducedTriangleGeometry = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);

  inputValues.TriangleDataHandling = filterArgs.value<ChoicesParameter::ValueType>(k_TriangleDataHandling_Key);
  inputValues.TriangleAttributeMatrixPath = filterArgs.value<DataPath>(k_TriangleDataSelectedAttributeMatrix_Key);
  inputValues.SelectedTriangleData = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_TriangleDataSelectedArrays_Key);

  inputValues.VertexDataHandling = filterArgs.value<ChoicesParameter::ValueType>(k_VertexDataHandling_Key);
  inputValues.VertexAttributeMatrixPath = filterArgs.value<DataPath>(k_VertexDataSelectedAttributeMatrix_Key);
  inputValues.SelectedVertexData = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_VertexDataSelectedArrays_Key);

  return RemoveFlaggedTriangles(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_TriangleGeometryKey = "TriangleGeometry";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ReducedTriangleGeometryKey = "ReducedTriangleGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> RemoveFlaggedTrianglesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RemoveFlaggedTrianglesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_TriangleGeometryKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_ReducedTriangleGeometryKey, k_CreatedTriangleGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
