#include "CropImageGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct CropImageGeometryFilterCache
{
  uint64 xMin;
  uint64 xMax;
  uint64 yMax;
  uint64 yMin;
  uint64 zMax;
  uint64 zMin;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, CropImageGeometryFilterCache> s_HeaderCache;

const std::string k_TempGeometryName = ".cropped_image_geometry";

/**
 * @brief
 * @param dataStructure
 * @param imageGeomPath
 * @return
 */
FloatVec3 GetCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  FloatVec3 data = {0, 0, 0};
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(image != nullptr)
  {
    data = image->getSpacing();
  }
  return data;
}
} // namespace

//------------------------------------------------------------------------------
CropImageGeometryFilter::CropImageGeometryFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
CropImageGeometryFilter::~CropImageGeometryFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::name() const
{
  return FilterTraits<CropImageGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::className() const
{
  return FilterTraits<CropImageGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CropImageGeometryFilter::uuid() const
{
  return FilterTraits<CropImageGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Crop Image Geometry", "Image Geometry", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters CropImageGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UsePhysicalBounds_Key, "Use Physical Units For Bounds", "If true define physical coordinates for bounds, If false define voxel indices for bounds", true));
  params.insert(std::make_unique<BoolParameter>(k_CropXDim_Key, "Crop X Dimension", "Enable cropping in the X dimension.", true));
  params.insert(std::make_unique<BoolParameter>(k_CropYDim_Key, "Crop Y Dimension", "Enable cropping in the Y dimension.", true));
  params.insert(std::make_unique<BoolParameter>(k_CropZDim_Key, "Crop Z Dimension", "Enable cropping in the Z dimension.", true));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinVoxel_Key, "Min Voxel", "Lower bound of voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxVoxel_Key, "Max Voxel [Inclusive]", "Upper bound in voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MinCoord_Key, "Min Coordinate (Physical Units)", "Lower bound in real units of the volume to crop.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MaxCoord_Key, "Max Coordinate (Physical Units) [Inclusive]", "Upper bound in real units of the volume to crop.",
                                                         std::vector<float64>{0.0, 0.0, 0.0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "DataPath to the source Image Geometry", DataPath(), std::set{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Optional Renumber Features"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "The DataPath to store the created Image Geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UsePhysicalBounds_Key, k_MinVoxel_Key, false);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxVoxel_Key, false);

  params.linkParameters(k_UsePhysicalBounds_Key, k_MinCoord_Key, true);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxCoord_Key, true);

  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CropImageGeometryFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer CropImageGeometryFilter::clone() const
{
  return std::make_unique<CropImageGeometryFilter>();
}

IFilter::PreflightResult CropImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto featureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto minVoxels = filterArgs.value<std::vector<uint64>>(k_MinVoxel_Key);
  auto maxVoxels = filterArgs.value<std::vector<uint64>>(k_MaxVoxel_Key);
  auto shouldRenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto cellFeatureAmPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto pUsePhysicalBounds = filterArgs.value<bool>(k_UsePhysicalBounds_Key);
  auto pCropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  auto pCropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  auto pCropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  uint64& xMin = s_HeaderCache[m_InstanceId].xMin;
  uint64& xMax = s_HeaderCache[m_InstanceId].xMax;
  uint64& yMax = s_HeaderCache[m_InstanceId].yMax;
  uint64& yMin = s_HeaderCache[m_InstanceId].yMin;
  uint64& zMax = s_HeaderCache[m_InstanceId].zMax;
  uint64& zMin = s_HeaderCache[m_InstanceId].zMin;

  auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  {
    // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
    std::string cropOptionsStr = "This filter will crop the image in the following dimension(s):  ";
    cropOptionsStr.append(pCropXDim ? "X" : "");
    cropOptionsStr.append(pCropYDim ? "Y" : "");
    cropOptionsStr.append(pCropZDim ? "Z" : "");
    preflightUpdatedValues.push_back({"Crop Dimensions", cropOptionsStr});

    preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom.getDimensions(), srcImageGeom.getSpacing(),
                                                                                                                          srcImageGeom.getOrigin(), srcImageGeom.getUnits())});
  }

  if(!pCropXDim && !pCropYDim && !pCropZDim)
  {
    return {MakeErrorResult<OutputActions>(-4010, "At least one dimension must be selected to crop!"), preflightUpdatedValues};
  }

  xMin = pCropXDim ? minVoxels[0] : 0;
  xMax = pCropXDim ? maxVoxels[0] : srcImageGeom.getNumXCells() - 1;
  yMin = pCropYDim ? minVoxels[1] : 0;
  yMax = pCropYDim ? maxVoxels[1] : srcImageGeom.getNumYCells() - 1;
  zMin = pCropZDim ? minVoxels[2] : 0;
  zMax = pCropZDim ? maxVoxels[2] : srcImageGeom.getNumZCells() - 1;

  if(!pUsePhysicalBounds)
  {
    if(pCropXDim && xMax < xMin)
    {
      const std::string errMsg = fmt::format("X Max ({}) less than X Min ({})", xMax, xMin);
      return {MakeErrorResult<OutputActions>(-4011, errMsg), preflightUpdatedValues};
    }
    if(pCropYDim && yMax < yMin)
    {
      const std::string errMsg = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
      return {MakeErrorResult<OutputActions>(-4012, errMsg), preflightUpdatedValues};
    }
    if(pCropZDim && zMax < zMin)
    {
      const std::string errMsg = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
      return {MakeErrorResult<OutputActions>(-4013, errMsg), preflightUpdatedValues};
    }
  }

  // Validate the incoming DataContainer, Geometry, and AttributeMatrix.
  const auto spacing = GetCurrentVolumeDataContainerResolutions(dataStructure, srcImagePath);

  const auto* srcImageGeomPtr = dataStructure.getDataAs<ImageGeom>(srcImagePath);
  auto srcOrigin = srcImageGeomPtr->getOrigin();

  if(pUsePhysicalBounds)
  {
    auto maxCropCoord = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaxCoord_Key);
    auto minCropCoord = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MinCoord_Key);

    // Validate basic information about the coordinates
    bool equalCoords = true;
    if(pCropXDim && minCropCoord[0] != maxCropCoord[0])
    {
      equalCoords = false;
    }
    if(pCropYDim && minCropCoord[1] != maxCropCoord[1])
    {
      equalCoords = false;
    }
    if(pCropZDim && minCropCoord[2] != maxCropCoord[2])
    {
      equalCoords = false;
    }
    if(equalCoords)
    {
      const std::string errMsg = "All minimum and maximum values are equal. The cropped region would be a ZERO volume. Please change the maximum values to be larger than the minimum values.";
      return {MakeErrorResult<OutputActions>(-50556, errMsg), preflightUpdatedValues};
    }

    auto bounds = srcImageGeomPtr->getBoundingBoxf();
    const Point3Df& minPoint = bounds.getMinPoint();
    const Point3Df& maxPoint = bounds.getMaxPoint();

    std::vector<std::string> errLabels = {"X", "Y", "Z"};
    std::vector<bool> dimEnabled = {pCropXDim, pCropYDim, pCropZDim};
    for(uint8 i = 0; i < 3; i++)
    {
      if(dimEnabled[i] && maxCropCoord[i] < minCropCoord[i])
      {
        const std::string errMsg = fmt::format("The max value {} ({}) is lower then the min value {} ({}). Please ensure the maximum value is greater than the minimum value.", errLabels[i],
                                               maxCropCoord[i], errLabels[i], minCropCoord[i]);
        return {MakeErrorResult<OutputActions>(-50559, errMsg), preflightUpdatedValues};
      }

      if(dimEnabled[i] && maxCropCoord[i] < minPoint[i] && minCropCoord[i] < minPoint[i])
      {
        const std::string errMsg = fmt::format(
            "Both the Minimum and Maximum {} crop values are less than the minimum {} bounds ({}). Please ensure at least part of the crop is within the bounding box of min=[{}] and max=[{}]",
            errLabels[i], errLabels[i], maxPoint[i], fmt::join(minPoint.begin(), minPoint.end(), ","), fmt::join(maxPoint.begin(), maxPoint.end(), ","));
        return {MakeErrorResult<OutputActions>(-50560, errMsg), preflightUpdatedValues};
      }

      if(dimEnabled[i] && maxCropCoord[i] > maxPoint[i] && minCropCoord[i] > maxPoint[i])
      {
        const std::string errMsg = fmt::format(
            "Both the Minimum and Maximum {} crop values are greater than the maximum {} bounds ({}). Please ensure at least part of the crop is within the bounding box of min=[{}] and max=[{}]",
            errLabels[i], errLabels[i], maxPoint[i], fmt::join(minPoint.begin(), minPoint.end(), ","), fmt::join(maxPoint.begin(), maxPoint.end(), ","));
        return {MakeErrorResult<OutputActions>(-50560, errMsg), preflightUpdatedValues};
      }

      if(dimEnabled[i] && minCropCoord[i] < minPoint[i])
      {
        resultOutputActions.m_Warnings.push_back(
            Warning({-50503, fmt::format("The {} minimum crop value {} is less than the {} minimum bounds value of {}. The filter will use the minimum bounds value instead.", errLabels[i],
                                         minCropCoord[i], errLabels[i], minPoint[i])}));
      }
      if(dimEnabled[i] && maxCropCoord[i] > maxPoint[i])
      {
        resultOutputActions.m_Warnings.push_back(
            Warning({-50503, fmt::format("The {} maximum crop value {} is greater than the {} maximum bounds value of {}. The filter will use the maximum bounds value instead.", errLabels[i],
                                         maxCropCoord[i], errLabels[i], maxPoint[i])}));
      }
    }

    // if we have made it here the coordinate bounds are valid so figure out and assign index values to xMax, xMin, ...
    auto srcSpacing = srcImageGeomPtr->getSpacing();
    xMin = (pCropXDim && minCropCoord[0] >= srcOrigin[0]) ? static_cast<uint64>(std::floor((minCropCoord[0] - srcOrigin[0]) / spacing[0])) : 0;
    yMin = (pCropYDim && minCropCoord[1] >= srcOrigin[1]) ? static_cast<uint64>(std::floor((minCropCoord[1] - srcOrigin[1]) / spacing[1])) : 0;
    zMin = (pCropZDim && minCropCoord[2] >= srcOrigin[2]) ? static_cast<uint64>(std::floor((minCropCoord[2] - srcOrigin[2]) / spacing[2])) : 0;

    xMax = (pCropXDim && maxCropCoord[0] <= maxPoint[0]) ? static_cast<uint64>(std::floor((maxCropCoord[0] - srcOrigin[0]) / spacing[0])) : srcImageGeomPtr->getNumXCells() - 1;
    yMax = (pCropYDim && maxCropCoord[1] <= maxPoint[1]) ? static_cast<uint64>(std::floor((maxCropCoord[1] - srcOrigin[1]) / spacing[1])) : srcImageGeomPtr->getNumYCells() - 1;
    zMax = (pCropZDim && maxCropCoord[2] <= maxPoint[2]) ? static_cast<uint64>(std::floor((maxCropCoord[2] - srcOrigin[2]) / spacing[2])) : srcImageGeomPtr->getNumZCells() - 1;
  }

  if(pCropXDim && xMax > srcImageGeomPtr->getNumXCells() - 1)
  {
    const std::string errMsg = fmt::format("The X Max ({}) is greater than the Image Geometry X extent ({})", xMax, srcImageGeomPtr->getNumXCells() - 1);
    return {MakeErrorResult<OutputActions>(-5553, errMsg), preflightUpdatedValues};
  }

  if(pCropYDim && yMax > srcImageGeomPtr->getNumYCells() - 1)
  {
    const std::string errMsg = fmt::format("The Y Max ({}) is greater than the Image Geometry Y extent ({})", yMax, srcImageGeomPtr->getNumYCells() - 1);
    return {MakeErrorResult<OutputActions>(-5554, errMsg), preflightUpdatedValues};
  }

  if(pCropZDim && zMax > srcImageGeomPtr->getNumZCells() - 1)
  {
    const std::string errMsg = fmt::format("The Z Max ({}) is greater than the Image Geometry Z extent ({})", zMax, srcImageGeomPtr->getNumZCells() - 1);
    return {MakeErrorResult<OutputActions>(-5555, errMsg), preflightUpdatedValues};
  }

  if(static_cast<int>(xMax) - static_cast<int>(xMin) < 0)
  {
    xMax = xMin + 1;
  }
  if(static_cast<int>(yMax) - static_cast<int>(yMin) < 0)
  {
    yMax = yMin + 1;
  }
  if(static_cast<int>(zMax) - static_cast<int>(zMin) < 0)
  {
    zMax = zMin + 1;
  }

  // The ImageGeometryDimensions go from Fastest to Slowest, XYZ.
  std::vector<usize> geomDims = {(xMax - xMin) + 1, (yMax - yMin) + 1, (zMax - zMin) + 1};
  std::vector<usize> dataArrayShape = {geomDims[2], geomDims[1], geomDims[0]}; // The DataArray shape goes slowest to fastest (ZYX)

  std::vector<float32> targetOrigin(3);
  targetOrigin[0] = static_cast<float>(xMin) * spacing[0] + srcOrigin[0];
  targetOrigin[1] = static_cast<float>(yMin) * spacing[1] + srcOrigin[1];
  targetOrigin[2] = static_cast<float>(zMin) * spacing[2] + srcOrigin[2];

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  if(pRemoveOriginalGeometry)
  {
    // Generate a new name for the current Image Geometry
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);
    // Rename the current image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcImagePath, tempName));
    // After the execute function has been done, delete the moved image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcImagePath.getPathVector();
    tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  // This section gets the cell attribute matrix for the input Image Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in the CreateImageGeometryAction
    const AttributeMatrix* selectedCellData = srcImageGeomPtr->getCellData();
    if(selectedCellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-4014, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString())), preflightUpdatedValues};
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(
        destImagePath, geomDims, targetOrigin, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, cellDataName, srcImageGeomPtr->getUnits()));

    // Now loop over each array in the source image geometry's cell attribute matrix and create the corresponding arrays
    // in the destination image geometry's attribute matrix
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }

    preflightUpdatedValues.push_back(
        {"Cropped Image Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(geomDims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, targetOrigin,
                                                                                                     srcImageGeomPtr->getUnits())});
  }
  // This section covers the option of renumbering the Feature Data where we need to do a
  // similar creation of the Data Arrays based on the arrays in the Source Image Geometry's
  // Feature Attribute Matrix
  if(shouldRenumberFeatures)
  {
    ignorePaths.push_back(cellFeatureAmPath);

    const auto& srcCellFeatureData = dataStructure.getDataRefAs<AttributeMatrix>(cellFeatureAmPath);
    std::string warningMsg;
    DataPath destCellFeatureAmPath = destImagePath.createChildPath(cellFeatureAmPath.getTargetName());
    auto tDims = srcCellFeatureData.getShape();
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAmPath, tDims));
    for(const auto& [identifier, object] : srcCellFeatureData)
    {
      if(const auto* srcArray = dynamic_cast<const IDataArray*>(object.get()); srcArray != nullptr)
      {
        DataType dataType = srcArray->getDataType();
        ShapeType componentShape = srcArray->getIDataStoreRef().getComponentShape();
        DataPath dataArrayPath = destCellFeatureAmPath.createChildPath(srcArray->getName());
        resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tDims, std::move(componentShape), dataArrayPath));
      }
      else if(const auto* srcNeighborListArray = dynamic_cast<const INeighborList*>(object.get()); srcNeighborListArray != nullptr)
      {
        warningMsg += "\n" + cellFeatureAmPath.toString() + "/" + srcNeighborListArray->getName();
      }
    }
    if(!warningMsg.empty())
    {
      preflightUpdatedValues.push_back(
          {"Invalidated NeighborLists",
           fmt::format(
               "This filter will modify the Cell Level Array(s) '{}' which causes all feature level NeighborLists to become invalid. These NeighborLists will not be copied to the new geometry:{}",
               featureIdsArrayPath.toString(), warningMsg)});
    }
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry
  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
      DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
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
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> CropImageGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CropImageGeometryInputValues inputValues;

  inputValues.InputImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.OutputImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.MinVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MinVoxel_Key);
  inputValues.MaxVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MaxVoxel_Key);
  inputValues.RenumberFeatures = filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  inputValues.RemoveOriginalGeometry = filterArgs.value<BoolParameter::ValueType>(k_RemoveOriginalGeometry_Key);
  inputValues.CropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  inputValues.CropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  inputValues.CropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  inputValues.XMin = s_HeaderCache[m_InstanceId].xMin;
  inputValues.XMax = s_HeaderCache[m_InstanceId].xMax;
  inputValues.YMin = s_HeaderCache[m_InstanceId].yMin;
  inputValues.YMax = s_HeaderCache[m_InstanceId].yMax;
  inputValues.ZMin = s_HeaderCache[m_InstanceId].zMin;
  inputValues.ZMax = s_HeaderCache[m_InstanceId].zMax;

  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_XMinKey = "XMin";
constexpr StringLiteral k_YMinKey = "YMin";
constexpr StringLiteral k_ZMinKey = "ZMin";
constexpr StringLiteral k_XMaxKey = "XMax";
constexpr StringLiteral k_YMaxKey = "YMax";
constexpr StringLiteral k_ZMaxKey = "ZMax";
// constexpr StringLiteral k_OldBoxDimensionsKey = "OldBoxDimensions";
// constexpr StringLiteral k_NewBoxDimensionsKey = "NewBoxDimensions";
// constexpr StringLiteral k_UpdateOriginKey = "UpdateOrigin";
constexpr StringLiteral k_SaveAsNewDataContainerKey = "SaveAsNewDataContainer";
constexpr StringLiteral k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteral k_RenumberFeaturesKey = "RenumberFeatures";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
} // namespace SIMPL
} // namespace

Result<Arguments> CropImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CropImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMinKey, SIMPL::k_YMinKey, SIMPL::k_ZMinKey, k_MinVoxel_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMaxKey, SIMPL::k_YMaxKey, SIMPL::k_ZMaxKey, k_MaxVoxel_Key));
  // k_UpdateOrigin_Key currently disabled in NX.
  // results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UpdateOriginKey, k_UpdateOrigin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InvertedBooleanFilterParameterConverter>(args, json, SIMPL::k_SaveAsNewDataContainerKey, k_RemoveOriginalGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerNameKey, k_CreatedImageGeometryPath_Key));
  // Cell attribute matrix parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_RenumberFeaturesKey, k_RenumberFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixPathKey, k_FeatureAttributeMatrixPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
