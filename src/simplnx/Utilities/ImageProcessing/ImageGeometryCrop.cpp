#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/ranges.h>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".cropped_image_geometry";

FloatVec3 GetCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  return image == nullptr ? FloatVec3{0, 0, 0} : image->getSpacing();
}
} // namespace

IFilter::PreflightResult nx::core::PreflightImageGeometryCrop(const DataStructure& dataStructure, const ImageGeometryCropOptions& options, ImageGeometryCropBounds& bounds)
{
  const DataPath& srcImagePath = options.inputImageGeometryPath;
  DataPath destImagePath = options.outputImageGeometryPath;
  const DataPath& featureIdsArrayPath = options.featureIdsPath;
  const std::vector<uint64>& minVoxels = options.minVoxel;
  const std::vector<uint64>& maxVoxels = options.maxVoxel;
  const bool shouldRenumberFeatures = options.renumberFeatures;
  const DataPath& cellFeatureAmPath = options.cellFeatureAttributeMatrixPath;
  const bool pRemoveOriginalGeometry = options.removeOriginalGeometry;
  const bool pUsePhysicalBounds = options.usePhysicalBounds;
  const bool pCropXDim = options.cropX;
  const bool pCropYDim = options.cropY;
  const bool pCropZDim = options.cropZ;

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  uint64& xMin = bounds.xMin;
  uint64& xMax = bounds.xMax;
  uint64& yMax = bounds.yMax;
  uint64& yMin = bounds.yMin;
  uint64& zMax = bounds.zMax;
  uint64& zMin = bounds.zMin;

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
    const std::vector<float64>& maxCropCoord = options.maxCoordinate;
    const std::vector<float64>& minCropCoord = options.minCoordinate;

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

    auto boundingBox = srcImageGeomPtr->getBoundingBoxf();
    const Point3Df& minPoint = boundingBox.getMinPoint();
    const Point3Df& maxPoint = boundingBox.getMaxPoint();

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

  // Create the destination Cell Attribute Matrix and its arrays.
  // Add the source matrix to ignorePaths because these arrays have explicit create actions.
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

    // Create one destination array for each source Cell Attribute Matrix array.
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
  // If feature renumbering is enabled, create the destination feature arrays.
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
