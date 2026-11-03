#include "CropEdgeGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "SimplnxCore/Filters/Algorithms/CropEdgeGeometry.hpp"

using namespace nx::core;

namespace
{
void CreateCroppedArrayActions(const AttributeMatrix& source, const DataPath& destination, OutputActions& actions)
{
  for(const auto& [identifier, object] : source)
  {
    const auto path = destination.createChildPath(object->getName());
    if(const auto* array = dynamic_cast<const IDataArray*>(object.get()); array != nullptr)
    {
      actions.appendAction(std::make_unique<CreateArrayAction>(array->getDataType(), ShapeType{1}, array->getIDataStoreRef().getComponentShape(), path));
    }
    else if(dynamic_cast<const StringArray*>(object.get()) != nullptr)
    {
      actions.appendAction(std::make_unique<CreateStringArrayAction>(ShapeType{1}, path));
    }
    else if(const auto* neighbors = dynamic_cast<const INeighborList*>(object.get()); neighbors != nullptr)
    {
      actions.appendAction(std::make_unique<CreateNeighborListAction>(neighbors->getDataType(), ShapeType{1}, path));
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
CropEdgeGeometryFilter::CropEdgeGeometryFilter()
{
}

//------------------------------------------------------------------------------
CropEdgeGeometryFilter::~CropEdgeGeometryFilter() noexcept
{
}

//------------------------------------------------------------------------------
std::string CropEdgeGeometryFilter::name() const
{
  return FilterTraits<CropEdgeGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string CropEdgeGeometryFilter::className() const
{
  return FilterTraits<CropEdgeGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CropEdgeGeometryFilter::uuid() const
{
  return FilterTraits<CropEdgeGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CropEdgeGeometryFilter::humanName() const
{
  return "Crop Geometry (Edge)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropEdgeGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Crop Edge Geometry", "Edge Geometry", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters CropEdgeGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_CropXDim_Key, "Crop X Dimension", "Enable cropping in the X dimension.", false));
  params.insert(std::make_unique<BoolParameter>(k_CropYDim_Key, "Crop Y Dimension", "Enable cropping in the Y dimension.", false));
  params.insert(std::make_unique<BoolParameter>(k_CropZDim_Key, "Crop Z Dimension", "Enable cropping in the Z dimension.", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinCoord_Key, "Min Coordinate", "Lower bound of the edge geometry to crop.", std::vector<float32>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxCoord_Key, "Max Coordinate [Inclusive]", "Upper bound of the edge geometry to crop.", std::vector<float32>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Replaces the original Edge Geometry after filter is completed", true));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundaryIntersectionBehavior_Key, "Boundary Intersection Behavior",
                                                   "The behavior to implement if an edge intersects a bound (one vertex is inside, one vertex is outside).\n\n\"Interpolate Outside Vertex\" will move "
                                                   "the outside vertex of a boundary-intersecting edge from its current position to the boundary edge.\n\"Ignore Edge\" will ignore any edge that "
                                                   "intersects a bound.\n\"Filter Error\" will make this filter throw an error when it encounters an edge that intersects a bound.",
                                                   to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex),
                                                   ChoicesParameter::Choices{"Interpolate Outside Vertex", "Ignore Edge", "Filter Error"}));

  params.insertSeparator(Parameters::Separator{"Input Edge Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_SelectedEdgeGeometryPath_Key, "Selected Edge Geometry", "DataPath to the source Edge Geometry", DataPath(), std::set{IGeometry::Type::Edge}));

  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
      "Recreates the selected Feature Attribute Matrix in the cropped geometry with no arrays. Cropping changes which edges belong to each feature, so every feature value computed before this "
      "filter is invalid. Recompute any feature data needed later in the pipeline.",
      true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Edge Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after cropping", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_EdgeFeatureIdsPath_Key, "Edge Feature Ids", "Specifies to which feature each edge belongs.", DataPath({"Edge Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Edge Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedEdgeGeometryPath_Key, "Created Edge Geometry", "The DataPath to store the created Edge Geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_EdgeFeatureIdsPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedEdgeGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CropEdgeGeometryFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropEdgeGeometryFilter::clone() const
{
  return std::make_unique<CropEdgeGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropEdgeGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcEdgeGeomPath = filterArgs.value<DataPath>(k_SelectedEdgeGeometryPath_Key);
  auto destEdgeGeomPath = filterArgs.value<DataPath>(k_CreatedEdgeGeometryPath_Key);
  auto minCoords = filterArgs.value<std::vector<float32>>(k_MinCoord_Key);
  auto maxCoords = filterArgs.value<std::vector<float32>>(k_MaxCoord_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto pCropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  auto pCropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  auto pCropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  const bool clearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  const bool shouldRenumberFeatures = clearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  const auto featureAmPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  const auto featureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_EdgeFeatureIdsPath_Key);

  auto& srcEdgeGeom = dataStructure.getDataRefAs<EdgeGeom>(srcEdgeGeomPath);

  if(!pCropXDim && !pCropYDim && !pCropZDim)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(CropEdgeGeometry::ErrorCodes::NoDimensionsChosen), "At least one dimension must be selected to crop!")};
  }

  float32 xMin = pCropXDim ? minCoords[0] : std::numeric_limits<float32>::lowest();
  float32 xMax = pCropXDim ? maxCoords[0] : std::numeric_limits<float32>::max();
  float32 yMin = pCropYDim ? minCoords[1] : std::numeric_limits<float32>::lowest();
  float32 yMax = pCropYDim ? maxCoords[1] : std::numeric_limits<float32>::max();
  float32 zMin = pCropZDim ? minCoords[2] : std::numeric_limits<float32>::lowest();
  float32 zMax = pCropZDim ? maxCoords[2] : std::numeric_limits<float32>::max();

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pCropXDim && xMax < xMin)
  {
    const std::string errMsg = fmt::format("X Max ({}) less than X Min ({})", xMax, xMin);
    return {MakeErrorResult<OutputActions>(to_underlying(CropEdgeGeometry::ErrorCodes::XMinLargerThanXMax), errMsg)};
  }
  if(pCropYDim && yMax < yMin)
  {
    const std::string errMsg = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
    return {MakeErrorResult<OutputActions>(to_underlying(CropEdgeGeometry::ErrorCodes::YMinLargerThanYMax), errMsg)};
  }
  if(pCropZDim && zMax < zMin)
  {
    const std::string errMsg = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
    return {MakeErrorResult<OutputActions>(to_underlying(CropEdgeGeometry::ErrorCodes::ZMinLargerThanZMax), errMsg)};
  }

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  if(pRemoveOriginalGeometry)
  {
    // Generate a new name for the current Edge Geometry
    auto tempPathVector = srcEdgeGeomPath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);
    // Rename the current edge geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcEdgeGeomPath, tempName));
    // After the execute function has been done, delete the moved edge geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcEdgeGeomPath.getPathVector();
    tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destEdgeGeomPath = DataPath({tempPathVector});
  }

  // Output actions create the Vertex and Edge Attribute Matrices and their arrays.
  // The ignore paths exclude these objects from the separate child-copy actions.
  {
    // Get the name of the Edge Attribute Matrix, so we can use that in the CreateEdgeGeometryAction
    const auto* srcEdgeGeomPtr = dataStructure.getDataAs<EdgeGeom>(srcEdgeGeomPath);
    const AttributeMatrix& selectedVertexData = *srcEdgeGeomPtr->getVertexAttributeMatrix();
    const AttributeMatrix& selectedEdgeData = *srcEdgeGeomPtr->getEdgeAttributeMatrix();
    {
      auto& vertexAttrMatrix = srcEdgeGeom.getVertexAttributeMatrixRef();
      auto& edgeAttrMatrix = srcEdgeGeom.getEdgeAttributeMatrixRef();
      auto& verticesArray = srcEdgeGeom.getVerticesRef();
      auto& edgesArray = srcEdgeGeom.getEdgesRef();
      resultOutputActions.value().appendAction(
          std::make_unique<CreateEdgeGeometryAction>(destEdgeGeomPath, 1, 1, vertexAttrMatrix.getName(), edgeAttrMatrix.getName(), verticesArray.getName(), edgesArray.getName()));

      auto vertexAttrMatDataPaths = vertexAttrMatrix.getDataPaths();
      auto edgeAttrMatDataPaths = edgeAttrMatrix.getDataPaths();
      auto verticesArrayDataPaths = verticesArray.getDataPaths();
      auto edgesArrayDataPaths = edgesArray.getDataPaths();
      ignorePaths.insert(ignorePaths.end(), vertexAttrMatDataPaths.begin(), vertexAttrMatDataPaths.end());
      ignorePaths.insert(ignorePaths.end(), edgeAttrMatDataPaths.begin(), edgeAttrMatDataPaths.end());
      ignorePaths.insert(ignorePaths.end(), verticesArrayDataPaths.begin(), verticesArrayDataPaths.end());
      ignorePaths.insert(ignorePaths.end(), edgesArrayDataPaths.begin(), edgesArrayDataPaths.end());
    }

    // The output arrays start with one tuple; execution resizes them to the retained number of edges.
    DataPath newEdgeAttributeMatrixPath = destEdgeGeomPath.createChildPath(selectedEdgeData.getName());
    CreateCroppedArrayActions(selectedEdgeData, newEdgeAttributeMatrixPath, resultOutputActions.value());

    // Execution resizes the vertex arrays to include retained vertices and distinct clip points.
    DataPath newVertexAttributeMatrixPath = destEdgeGeomPath.createChildPath(selectedVertexData.getName());

    auto vertexArraysResult = selectedVertexData.findAllChildrenOfType<IDataArray>();
    if(!vertexArraysResult.empty())
    {
      // Detected at least one vertex array, throw a warning
      resultOutputActions.warnings().push_back({-100, "A vertex DataArray was detected in the selected Edge Geometry. This filter only interpolates vertex positions. Vertex DataArray values are "
                                                      "copied from the source vertex, including for duplicated vertices, and are not interpolated."});
    }

    CreateCroppedArrayActions(selectedVertexData, newVertexAttributeMatrixPath, resultOutputActions.value());

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
    std::string cropOptionsStr = "This filter will crop the edge geometry in the following dimension(s):  ";
    cropOptionsStr.append(pCropXDim ? "X" : "");
    cropOptionsStr.append(pCropYDim ? "Y" : "");
    cropOptionsStr.append(pCropZDim ? "Z" : "");
    preflightUpdatedValues.push_back({"Crop Dimensions", cropOptionsStr});
  }

  // The cleared Feature AM keeps its source shape until optional renumbering.
  if(clearFeatureAttributeMatrix)
  {
    const auto* srcFeatureData = dataStructure.getDataAs<AttributeMatrix>(featureAmPath);
    if(srcFeatureData == nullptr || !srcEdgeGeom.contains(srcFeatureData))
    {
      return {MakeErrorResult<OutputActions>(
                  -67411, fmt::format("The Feature Attribute Matrix '{}' must be a direct child of the selected Edge Geometry '{}'.", featureAmPath.toString(), srcEdgeGeomPath.toString())),
              preflightUpdatedValues};
    }
    if(srcEdgeGeom.getEdgeAttributeMatrix() == srcFeatureData || srcEdgeGeom.getVertexAttributeMatrix() == srcFeatureData)
    {
      return {MakeErrorResult<OutputActions>(
                  -67412, fmt::format("The Feature Attribute Matrix '{}' is the Edge or Vertex Attribute Matrix of '{}'. Select the Attribute Matrix that holds Feature data, or turn "
                                      "off 'Clear Feature Attribute Matrix'.",
                                      featureAmPath.toString(), srcEdgeGeomPath.toString())),
              preflightUpdatedValues};
    }
    if(shouldRenumberFeatures && !IsChildOfAttributeMatrix(dataStructure, featureIdsPath, *srcEdgeGeom.getEdgeAttributeMatrix()))
    {
      return {
          MakeErrorResult<OutputActions>(
              -67413, fmt::format("**Edge Feature Ids** DataArray '{}' must be in the Edge Attribute Matrix '{}' of the selected Edge Geometry '{}'. Select the DataArray from that Attribute Matrix.",
                                  featureIdsPath.toString(), srcEdgeGeomPath.createChildPath(srcEdgeGeom.getEdgeAttributeMatrix()->getName()).toString(), srcEdgeGeomPath.toString())),
          preflightUpdatedValues};
    }
    if(shouldRenumberFeatures)
    {
      const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
      if(featureIds.getNumberOfTuples() != srcEdgeGeom.getNumberOfEdges())
      {
        return {MakeErrorResult<OutputActions>(
                    -67414, fmt::format("**Edge Feature Ids** DataArray '{}' has {} tuples, but the selected Edge Geometry '{}' has {} edges. Select a DataArray with one tuple per edge.",
                                        featureIdsPath.toString(), featureIds.getNumberOfTuples(), srcEdgeGeomPath.toString(), srcEdgeGeom.getNumberOfEdges())),
                preflightUpdatedValues};
      }
    }
    const auto featureAmPaths = srcFeatureData->getDataPaths();
    ignorePaths.insert(ignorePaths.end(), featureAmPaths.begin(), featureAmPaths.end());
    const DataPath destFeatureAmPath = destEdgeGeomPath.createChildPath(featureAmPath.getTargetName());
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destFeatureAmPath, srcFeatureData->getShape()));

    std::string clearedList;
    for(const auto& [identifier, object] : *srcFeatureData)
    {
      clearedList += "\n" + featureAmPath.createChildPath(object->getName()).toString();
    }
    std::string message = fmt::format("The Feature Attribute Matrix '{}' is recreated in the cropped geometry with no arrays.", featureAmPath.toString());
    if(!clearedList.empty())
    {
      message += fmt::format(" Its values describe the uncropped geometry and are not copied. Recompute any of the following Data Arrays:\n{}", clearedList);
    }
    if(shouldRenumberFeatures)
    {
      message += "\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the crop.";
    }
    preflightUpdatedValues.push_back({"Cleared Feature Data", std::move(message)});
  }
  else
  {
    preflightUpdatedValues.push_back({"Feature Data Not Cleared",
                                      "Attribute Matrices other than the Edge and Vertex Attribute Matrices are copied to the cropped geometry unchanged. If one of them holds "
                                      "Feature data, its values describe the uncropped geometry and are not valid for the cropped geometry."});
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry

  auto childPaths = GetAllChildDataPaths(dataStructure, srcEdgeGeomPath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      const DataPath copiedChildPath = childPath.rebase(srcEdgeGeomPath, destEdgeGeomPath).value();
      AppendCopyDataObjectAction(dataStructure, resultOutputActions.value(), childPath, copiedChildPath, clearFeatureAttributeMatrix ? featureAmPath : DataPath{});
    }
  }

  if(pRemoveOriginalGeometry)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destEdgeGeomPath, srcEdgeGeomPath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CropEdgeGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CropEdgeGeometryInputValues inputValues;

  inputValues.clearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.renumberFeatures = inputValues.clearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.featureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  inputValues.edgeFeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_EdgeFeatureIdsPath_Key);
  inputValues.srcEdgeGeomPath = filterArgs.value<DataPath>(k_SelectedEdgeGeometryPath_Key);
  inputValues.destEdgeGeomPath = filterArgs.value<DataPath>(k_CreatedEdgeGeometryPath_Key);
  inputValues.minCoords = filterArgs.value<std::vector<float32>>(k_MinCoord_Key);
  inputValues.maxCoords = filterArgs.value<std::vector<float32>>(k_MaxCoord_Key);
  inputValues.removeOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.cropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  inputValues.cropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  inputValues.cropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);
  inputValues.boundaryIntersectionBehavior = filterArgs.value<ChoicesParameter::ValueType>(k_BoundaryIntersectionBehavior_Key);

  return CropEdgeGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
