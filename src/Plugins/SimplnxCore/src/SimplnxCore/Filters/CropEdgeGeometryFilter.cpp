#include "CropEdgeGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/CropEdgeGeometry.hpp"

using namespace nx::core;

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

  params.insertSeparator(Parameters::Separator{"Output Edge Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedEdgeGeometryPath_Key, "Created Edge Geometry", "The DataPath to store the created Edge Geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedEdgeGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CropEdgeGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropEdgeGeometryFilter::clone() const
{
  return std::make_unique<CropEdgeGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropEdgeGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto srcEdgeGeomPath = filterArgs.value<DataPath>(k_SelectedEdgeGeometryPath_Key);
  auto destEdgeGeomPath = filterArgs.value<DataPath>(k_CreatedEdgeGeometryPath_Key);
  auto minCoords = filterArgs.value<std::vector<float32>>(k_MinCoord_Key);
  auto maxCoords = filterArgs.value<std::vector<float32>>(k_MaxCoord_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto pCropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  auto pCropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  auto pCropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

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

  // This section gets the cell attribute matrix for the input Edge Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination edge geometry
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

    // Now loop over each array in the source edge geometry's cell attribute matrix and create the corresponding arrays
    // in the destination edge geometry's cell attribute matrix
    DataPath newEdgeAttributeMatrixPath = destEdgeGeomPath.createChildPath(selectedEdgeData.getName());
    for(const auto& [identifier, object] : selectedEdgeData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newEdgeAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, std::vector<usize>{1}, std::move(componentShape), dataArrayPath));
    }

    // Now loop over each array in the source edge geometry's vertex attribute matrix and create the corresponding arrays
    // in the destination edge geometry's vertex attribute matrix
    DataPath newVertexAttributeMatrixPath = destEdgeGeomPath.createChildPath(selectedVertexData.getName());

    auto vertexArraysResult = selectedVertexData.findAllChildrenOfType<IDataArray>();
    if(!vertexArraysResult.empty())
    {
      // Detected at least one vertex array, throw a warning
      resultOutputActions.warnings().push_back(
          {-100, "A vertex data array was detected in the selected edge geometry.  This filter currently only interpolates vertex positions, associated vertex data values will not be interpolated."});
    }

    for(const auto& [identifier, object] : selectedVertexData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newVertexAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, std::vector<usize>{1}, std::move(componentShape), dataArrayPath));
    }

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
    std::string cropOptionsStr = "This filter will crop the edge geometry in the following dimension(s):  ";
    cropOptionsStr.append(pCropXDim ? "X" : "");
    cropOptionsStr.append(pCropYDim ? "Y" : "");
    cropOptionsStr.append(pCropZDim ? "Z" : "");
    preflightUpdatedValues.push_back({"Crop Dimensions", cropOptionsStr});
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry

  auto childPaths = GetAllChildDataPaths(dataStructure, srcEdgeGeomPath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), srcEdgeGeomPath.getTargetName(), destEdgeGeomPath.getTargetName());
      DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), srcEdgeGeomPath.getTargetName(), destEdgeGeomPath.getTargetName());
            allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
          }
        }
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
      }
      else
      {
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
      }
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
                                             const std::atomic_bool& shouldCancel) const
{
  CropEdgeGeometryInputValues inputValues;

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
