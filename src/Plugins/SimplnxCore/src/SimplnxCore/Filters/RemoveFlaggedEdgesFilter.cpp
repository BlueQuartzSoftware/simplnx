#include "RemoveFlaggedEdgesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedEdges.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Edge Geometry", "The Edge Geometry that will be processed.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The DataArrayPath to the mask array that marks each edge as either true (remove) or false(keep).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  // Edge Data Handling
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_EdgeDataHandling_Key, "Edge Data Handling", "How to handle Data that resides on the edges", k_IgnoreArraysIdx, k_ArrayHandlingChoices));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_EdgeDataSelectedAttributeMatrix_Key, "Edge Data", "Edge Attribute Matrix that will be copied to the reduced geometry", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_EdgeDataSelectedArrays_Key, "Edge Attribute Arrays to Copy", "DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.linkParameters(k_EdgeDataHandling_Key, k_EdgeDataSelectedArrays_Key, k_CopySelectedArraysIdx);
  params.linkParameters(k_EdgeDataHandling_Key, k_EdgeDataSelectedAttributeMatrix_Key, k_CopyAllArraysIdx);

  // Vertex Data Handling

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Geometry", "The name of the created Edge Geometry", DataPath({"ReducedGeometry"})));

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
  auto pInitialGeometryPathValue = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto pReducedGeometryPathValue = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto pArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_EdgeDataHandling_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* initialGeom = dataStructure.getDataAs<INodeGeometry1D>(pInitialGeometryPathValue);

  if(initialGeom->getGeomType() == IGeometry::Type::Edge)
  {
    auto createGeometryAction = std::make_unique<CreateGeometry1DAction<EdgeGeom>>(
        pReducedGeometryPathValue, initialGeom->getNumberOfEdges(), initialGeom->getNumberOfVertices(),
        (initialGeom->getVertexAttributeMatrix() == nullptr ? "VertexAM" : initialGeom->getVertexAttributeMatrix()->getName()),
        (initialGeom->getEdgeAttributeMatrix() == nullptr ? "FaceAM" : initialGeom->getEdgeAttributeMatrix()->getName()), initialGeom->getVertices()->getName(), initialGeom->getEdges()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  std::vector<usize> dataArrayShape = {initialGeom->getNumberOfEdges()}; // The DataArray shape goes slowest to fastest (ZYX)

  // This section gets the cell attribute matrix for the input Edge Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  if(pArrayHandling == k_CopyAllArraysIdx)
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in the CreateImageGeometryAction
    const AttributeMatrix* selectedCellData = initialGeom->getEdgeAttributeMatrix();
    if(selectedCellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", pInitialGeometryPathValue.toString()))};
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(pInitialGeometryPathValue.createChildPath(cellDataName));

    // Now loop over each array in the source image geometry's cell attribute matrix and create the corresponding arrays
    // in the destination geometry's attribute matrix
    DataPath newCellAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }
  }
  if(pArrayHandling == k_CopySelectedArraysIdx)
  {
    const AttributeMatrix* selectedCellData = initialGeom->getEdgeAttributeMatrix();
    std::string cellDataName = selectedCellData->getName();

    auto selectedEdgeArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_EdgeDataSelectedArrays_Key);
    // Now loop over each array in selectedEdgeArrays and create the corresponding arrays
    // in the destination geometry's attribute matrix
    DataPath newCellAttributeMatrixPath = pReducedGeometryPathValue.createChildPath(cellDataName);
    for(const auto& dataPath : selectedEdgeArrays)
    {
      const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedEdgesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  RemoveFlaggedEdgesInputValues inputValues;

  inputValues.EdgeGeometry = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ReducedEdgeGeometry = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);

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

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_TriangleGeometryKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_ReducedTriangleGeometryKey, k_CreatedTriangleGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
