#include "RemoveFlaggedEdgesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedEdges.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
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
std::string RemoveFlaggedEdgesFilter::name() const
{
  return FilterTraits<RemoveFlaggedEdgesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedEdgesFilter::className() const
{
  return FilterTraits<RemoveFlaggedEdgesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedEdgesFilter::uuid() const
{
  return FilterTraits<RemoveFlaggedEdgesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedEdgesFilter::humanName() const
{
  return "Remove Flagged Edges";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedEdgesFilter::defaultTags() const
{
  return {"Surface Meshing", "Cleanup", "Edge Geometry", "Remove", "Delete"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedEdgesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputEdgeGeometryPath_Key, "Edge Geometry", "The Edge Geometry that will be processed.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The DataArrayPath to the mask array that marks each edge as either true (remove) or false(keep).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputEdgeGeometryPath_Key, "Created Geometry", "The name of the created Edge Geometry", DataPath({"ReducedGeometry"})));

  // Vertex Data Handling
  params.insertSeparator(Parameters::Separator{"Vertex Data Handling"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_VertexDataHandling_Key, "Vertex Data Handling", "How to handle Data that resides on the edges",
                                                                    detail::k_CopySelectedVertexArraysIdx, detail::k_VertexDataHandlingChoices));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_VertexDataSelectedAttributeMatrix_Key, "Vertex Data", "Vertex Attribute Matrix that will be copied to the reduced geometry", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_VertexDataSelectedArrays_Key, "Vertex Attribute Arrays to Copy", "Vertex DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.linkParameters(k_VertexDataHandling_Key, k_VertexDataSelectedAttributeMatrix_Key, detail::k_CopyAllVertexArraysIdx);
  params.linkParameters(k_VertexDataHandling_Key, k_VertexDataSelectedArrays_Key, detail::k_CopySelectedVertexArraysIdx);

  // Edge Data Handling
  params.insertSeparator(Parameters::Separator{"Edge Data Handling"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_EdgeDataHandling_Key, "Edge Data Handling", "How to handle Data that resides on the edges", detail::k_CopySelectedEdgeArraysIdx,
                                                                    detail::k_EdgeDataHandlingChoices));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_EdgeDataSelectedAttributeMatrix_Key, "Edge Data", "Edge Attribute Matrix that will be copied to the reduced geometry", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_EdgeDataSelectedArrays_Key, "Edge Attribute Arrays to Copy", "Edge DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.linkParameters(k_EdgeDataHandling_Key, k_EdgeDataSelectedArrays_Key, detail::k_CopySelectedEdgeArraysIdx);
  params.linkParameters(k_EdgeDataHandling_Key, k_EdgeDataSelectedAttributeMatrix_Key, detail::k_CopyAllEdgeArraysIdx);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedEdgesFilter::clone() const
{
  return std::make_unique<RemoveFlaggedEdgesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RemoveFlaggedEdgesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pInitialGeometryPathValue = filterArgs.value<DataPath>(k_InputEdgeGeometryPath_Key);
  auto pReducedGeometryPathValue = filterArgs.value<DataPath>(k_OutputEdgeGeometryPath_Key);
  auto pEdgeArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_EdgeDataHandling_Key);
  auto selectedEdgeArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_EdgeDataSelectedArrays_Key);
  auto selectedEdgeAttrMatPath = filterArgs.value<DataPath>(k_EdgeDataSelectedAttributeMatrix_Key);
  auto pVertexArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_VertexDataHandling_Key);
  auto selectedVertexArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_VertexDataSelectedArrays_Key);
  auto selectedVertexAttrMatPath = filterArgs.value<DataPath>(k_VertexDataSelectedAttributeMatrix_Key);

  Result<OutputActions> resultOutputActions;

  const auto* initialGeomPtr = dataStructure.getDataAs<INodeGeometry1D>(pInitialGeometryPathValue);

  std::string reducedVertexAttributeMatrixName = (initialGeomPtr->getVertexAttributeMatrix() == nullptr ? "Vertex Data" : initialGeomPtr->getVertexAttributeMatrix()->getName());
  std::string reducedEdgeAttributeMatrixName = (initialGeomPtr->getEdgeAttributeMatrix() == nullptr ? "Edge Data" : initialGeomPtr->getEdgeAttributeMatrix()->getName());

  DataPath reducedVertexAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(reducedVertexAttributeMatrixName);
  DataPath reducedEdgeAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(reducedEdgeAttributeMatrixName);

  std::vector<usize> edgeDataShape = {initialGeomPtr->getNumberOfEdges()};
  std::vector<usize> vertexDataShape = {initialGeomPtr->getNumberOfVertices()};

  if(initialGeomPtr->getGeomType() == IGeometry::Type::Edge)
  {
    auto createGeometryAction =
        std::make_unique<CreateGeometry1DAction<EdgeGeom>>(pReducedGeometryPathValue, initialGeomPtr->getNumberOfEdges(), initialGeomPtr->getNumberOfVertices(), reducedVertexAttributeMatrixName,
                                                           reducedEdgeAttributeMatrixName, initialGeomPtr->getVertices()->getName(), initialGeomPtr->getEdges()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  /** This section is for copying the Edge Data ***/
  // This _could_ be nullptr. We are going to hold off doing that check until inside each of the
  // conditional blocks below.
  {
    const AttributeMatrix* srcEdgeAttrMatPtr = initialGeomPtr->getEdgeAttributeMatrix();
    if(pEdgeArrayHandling == detail::k_CopySelectedEdgeArraysIdx)
    {
      if(!selectedEdgeArrays.empty() && nullptr == srcEdgeAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5651, fmt::format("'{}' must have edge data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      TransferGeometryElementData::createDataArrayActions<INodeGeometry1D>(dataStructure, srcEdgeAttrMatPtr, selectedEdgeArrays, reducedEdgeAttributeMatrixPath, resultOutputActions);
    }
    else if(pEdgeArrayHandling == detail::k_CopyAllEdgeArraysIdx)
    {
      if(nullptr == srcEdgeAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5652, fmt::format("'{}' must have edge data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      std::vector<DataPath> ignorePaths;

      auto getChildrenResult = GetAllChildArrayDataPaths(dataStructure, selectedEdgeAttrMatPath, ignorePaths);
      if(getChildrenResult.has_value())
      {
        selectedEdgeArrays = getChildrenResult.value();
        TransferGeometryElementData::createDataArrayActions<INodeGeometry1D>(dataStructure, srcEdgeAttrMatPtr, selectedEdgeArrays, reducedEdgeAttributeMatrixPath, resultOutputActions);
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
        return {MakeErrorResult<OutputActions>(-5653, fmt::format("'{}' must have Vertex data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      TransferGeometryElementData::createDataArrayActions<INodeGeometry1D>(dataStructure, srcVertexAttrMatPtr, selectedVertexArrays, reducedVertexAttributeMatrixPath, resultOutputActions);
    }
    else if(pVertexArrayHandling == detail::k_CopyAllVertexArraysIdx)
    {
      if(nullptr == srcVertexAttrMatPtr)
      {
        return {MakeErrorResult<OutputActions>(-5654, fmt::format("'{}' must have Vertex data attribute matrix", pInitialGeometryPathValue.toString()))};
      }
      std::vector<DataPath> ignorePaths;

      auto getChildrenResult = GetAllChildArrayDataPaths(dataStructure, selectedVertexAttrMatPath, ignorePaths);
      if(getChildrenResult.has_value())
      {
        selectedVertexArrays = getChildrenResult.value();
        TransferGeometryElementData::createDataArrayActions<INodeGeometry1D>(dataStructure, srcVertexAttrMatPtr, selectedVertexArrays, reducedVertexAttributeMatrixPath, resultOutputActions);
      }
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedEdgesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  RemoveFlaggedEdgesInputValues inputValues;

  inputValues.EdgeGeometry = filterArgs.value<DataPath>(k_InputEdgeGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ReducedEdgeGeometry = filterArgs.value<DataPath>(k_OutputEdgeGeometryPath_Key);

  inputValues.EdgeDataHandling = filterArgs.value<ChoicesParameter::ValueType>(k_EdgeDataHandling_Key);
  inputValues.EdgeAttributeMatrixPath = filterArgs.value<DataPath>(k_EdgeDataSelectedAttributeMatrix_Key);
  inputValues.SelectedEdgeData = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_EdgeDataSelectedArrays_Key);

  inputValues.VertexDataHandling = filterArgs.value<ChoicesParameter::ValueType>(k_VertexDataHandling_Key);
  inputValues.VertexAttributeMatrixPath = filterArgs.value<DataPath>(k_VertexDataSelectedAttributeMatrix_Key);
  inputValues.SelectedVertexData = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_VertexDataSelectedArrays_Key);

  return RemoveFlaggedEdges(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_TriangleGeometryKey = "EdgeGeometry";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ReducedTriangleGeometryKey = "ReducedEdgeGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> RemoveFlaggedEdgesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RemoveFlaggedEdgesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_TriangleGeometryKey, k_InputEdgeGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_ReducedTriangleGeometryKey, k_OutputEdgeGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
