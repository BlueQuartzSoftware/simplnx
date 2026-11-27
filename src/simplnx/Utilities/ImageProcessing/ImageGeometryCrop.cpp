#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <fmt/ranges.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".cropped_image_geometry";

FloatVec3Type GetCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  const auto* imagePtr = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  return imagePtr == nullptr ? FloatVec3Type{0, 0, 0} : imagePtr->getSpacing();
}
} // namespace

IFilter::PreflightResult nx::core::PreflightImageGeometryCrop(const DataStructure& dataStructure, const ImageGeometryCropOptions& options, ImageGeometryCropBounds& bounds)
{
  const DataPath& srcImagePath = options.input_image_geometry_path;
  DataPath destImagePath = options.output_image_geometry_path;
  const std::vector<uint64>& minVoxels = options.min_voxel;
  const std::vector<uint64>& maxVoxels = options.max_voxel;
  const bool shouldRenumberFeatures = options.renumber_features;
  const DataPath& cellFeatureAmPath = options.cell_feature_attribute_matrix_path;
  const bool pRemoveOriginalGeometry = options.remove_original_geometry;
  const bool pUsePhysicalBounds = options.use_physical_bounds;
  const bool pCropXDim = options.crop_x;
  const bool pCropYDim = options.crop_y;
  const bool pCropZDim = options.crop_z;

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  uint64& xMin = bounds.x_min;
  uint64& xMax = bounds.x_max;
  uint64& yMax = bounds.y_max;
  uint64& yMin = bounds.y_min;
  uint64& zMax = bounds.z_max;
  uint64& zMin = bounds.z_min;

  const auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  {
    // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
    std::string cropOptionsStr = "This filter will crop the image in the following dimension(s):  ";
    cropOptionsStr.append(pCropXDim ? "X" : "");
    cropOptionsStr.append(pCropYDim ? "Y" : "");
    cropOptionsStr.append(pCropZDim ? "Z" : "");
    preflightUpdatedValues.push_back({"Crop Dimensions", cropOptionsStr});

    preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::geometry_helpers::description::GenerateGeometryInfo(srcImageGeom.getDimensions(), srcImageGeom.getSpacing(),
                                                                                                                           srcImageGeom.getOrigin(), srcImageGeom.getUnits())});
  }

  if(!pCropXDim && !pCropYDim && !pCropZDim)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-4010, "At least one dimension must be selected to crop!"), .output_values = preflightUpdatedValues};
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
      return {.output_actions = MakeErrorResult<OutputActions>(-4011, errMsg), .output_values = preflightUpdatedValues};
    }
    if(pCropYDim && yMax < yMin)
    {
      const std::string errMsg = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
      return {.output_actions = MakeErrorResult<OutputActions>(-4012, errMsg), .output_values = preflightUpdatedValues};
    }
    if(pCropZDim && zMax < zMin)
    {
      const std::string errMsg = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
      return {.output_actions = MakeErrorResult<OutputActions>(-4013, errMsg), .output_values = preflightUpdatedValues};
    }
  }

  // Validate the incoming DataContainer, Geometry, and AttributeMatrix.
  const auto spacing = GetCurrentVolumeDataContainerResolutions(dataStructure, srcImagePath);

  const auto* srcImageGeomPtr = dataStructure.getDataAs<ImageGeom>(srcImagePath);
  auto srcOrigin = srcImageGeomPtr->getOrigin();

  if(pUsePhysicalBounds)
  {
    const std::vector<float64>& maxCropCoord = options.max_coordinate;
    const std::vector<float64>& minCropCoord = options.min_coordinate;

    const std::array<std::string, 3> errLabels = {"X", "Y", "Z"};
    const std::array<bool, 3> dimEnabled = {pCropXDim, pCropYDim, pCropZDim};
    for(uint8 i = 0; i < 3; i++)
    {
      if(dimEnabled[i] && (std::isnan(minCropCoord[i]) || std::isnan(maxCropCoord[i])))
      {
        return {.output_actions = MakeErrorResult<OutputActions>(
                    -50561, fmt::format("The {} crop values [{} to {}] contain a NaN value. Set both {} crop values to finite numbers.", errLabels[i], minCropCoord[i], maxCropCoord[i], errLabels[i])),
                .output_values = preflightUpdatedValues};
      }
    }

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
      return {.output_actions = MakeErrorResult<OutputActions>(-50556, errMsg), .output_values = preflightUpdatedValues};
    }

    auto boundingBox = srcImageGeomPtr->getBoundingBoxf();
    const FloatVec3Type& minPoint = boundingBox.getMinPoint();
    const FloatVec3Type& maxPoint = boundingBox.getMaxPoint();

    for(uint8 i = 0; i < 3; i++)
    {
      if(dimEnabled[i] && maxCropCoord[i] < minCropCoord[i])
      {
        const std::string errMsg = fmt::format("The max value {} ({}) is lower then the min value {} ({}). Please ensure the maximum value is greater than the minimum value.", errLabels[i],
                                               maxCropCoord[i], errLabels[i], minCropCoord[i]);
        return {.output_actions = MakeErrorResult<OutputActions>(-50559, errMsg), .output_values = preflightUpdatedValues};
      }

      if(dimEnabled[i] && maxCropCoord[i] < minPoint[i] && minCropCoord[i] < minPoint[i])
      {
        const std::string errMsg = fmt::format(
            "Both the Minimum and Maximum {} crop values are less than the minimum {} bounds ({}). Please ensure at least part of the crop is within the bounding box of min=[{}] and max=[{}]",
            errLabels[i], errLabels[i], maxPoint[i], fmt::join(minPoint.begin(), minPoint.end(), ","), fmt::join(maxPoint.begin(), maxPoint.end(), ","));
        return {.output_actions = MakeErrorResult<OutputActions>(-50560, errMsg), .output_values = preflightUpdatedValues};
      }

      if(dimEnabled[i] && maxCropCoord[i] > maxPoint[i] && minCropCoord[i] > maxPoint[i])
      {
        const std::string errMsg = fmt::format(
            "Both the Minimum and Maximum {} crop values are greater than the maximum {} bounds ({}). Please ensure at least part of the crop is within the bounding box of min=[{}] and max=[{}]",
            errLabels[i], errLabels[i], maxPoint[i], fmt::join(minPoint.begin(), minPoint.end(), ","), fmt::join(maxPoint.begin(), maxPoint.end(), ","));
        return {.output_actions = MakeErrorResult<OutputActions>(-50560, errMsg), .output_values = preflightUpdatedValues};
      }

      if(dimEnabled[i] && minCropCoord[i] < minPoint[i])
      {
        resultOutputActions.m_warnings.push_back(
            Warning({.code = -50503,
                     .message = fmt::format("The {} minimum crop value {} is less than the {} minimum bounds value of {}. The filter will use the minimum bounds value instead.", errLabels[i],
                                            minCropCoord[i], errLabels[i], minPoint[i])}));
      }
      if(dimEnabled[i] && maxCropCoord[i] > maxPoint[i])
      {
        resultOutputActions.m_warnings.push_back(
            Warning({.code = -50503,
                     .message = fmt::format("The {} maximum crop value {} is greater than the {} maximum bounds value of {}. The filter will use the maximum bounds value instead.", errLabels[i],
                                            maxCropCoord[i], errLabels[i], maxPoint[i])}));
      }
    }

    // if we have made it here the coordinate bounds are valid so figure out and assign index values to xMax, xMin, ...
    auto srcSpacing = srcImageGeomPtr->getSpacing();
    xMin = (pCropXDim && minCropCoord[0] >= srcOrigin[0]) ? static_cast<uint64>(std::floor((minCropCoord[0] - srcOrigin[0]) / spacing[0])) : 0;
    yMin = (pCropYDim && minCropCoord[1] >= srcOrigin[1]) ? static_cast<uint64>(std::floor((minCropCoord[1] - srcOrigin[1]) / spacing[1])) : 0;
    zMin = (pCropZDim && minCropCoord[2] >= srcOrigin[2]) ? static_cast<uint64>(std::floor((minCropCoord[2] - srcOrigin[2]) / spacing[2])) : 0;

    xMax = (pCropXDim && maxCropCoord[0] <= maxPoint[0]) ? std::min<uint64>(static_cast<uint64>(std::floor((maxCropCoord[0] - srcOrigin[0]) / spacing[0])), srcImageGeomPtr->getNumXCells() - 1) :
                                                           srcImageGeomPtr->getNumXCells() - 1;
    yMax = (pCropYDim && maxCropCoord[1] <= maxPoint[1]) ? std::min<uint64>(static_cast<uint64>(std::floor((maxCropCoord[1] - srcOrigin[1]) / spacing[1])), srcImageGeomPtr->getNumYCells() - 1) :
                                                           srcImageGeomPtr->getNumYCells() - 1;
    zMax = (pCropZDim && maxCropCoord[2] <= maxPoint[2]) ? std::min<uint64>(static_cast<uint64>(std::floor((maxCropCoord[2] - srcOrigin[2]) / spacing[2])), srcImageGeomPtr->getNumZCells() - 1) :
                                                           srcImageGeomPtr->getNumZCells() - 1;
  }

  if(pCropXDim && xMax > srcImageGeomPtr->getNumXCells() - 1)
  {
    const std::string errMsg = fmt::format("The X Max ({}) is greater than the Image Geometry X extent ({})", xMax, srcImageGeomPtr->getNumXCells() - 1);
    return {.output_actions = MakeErrorResult<OutputActions>(-5553, errMsg), .output_values = preflightUpdatedValues};
  }

  if(pCropYDim && yMax > srcImageGeomPtr->getNumYCells() - 1)
  {
    const std::string errMsg = fmt::format("The Y Max ({}) is greater than the Image Geometry Y extent ({})", yMax, srcImageGeomPtr->getNumYCells() - 1);
    return {.output_actions = MakeErrorResult<OutputActions>(-5554, errMsg), .output_values = preflightUpdatedValues};
  }

  if(pCropZDim && zMax > srcImageGeomPtr->getNumZCells() - 1)
  {
    const std::string errMsg = fmt::format("The Z Max ({}) is greater than the Image Geometry Z extent ({})", zMax, srcImageGeomPtr->getNumZCells() - 1);
    return {.output_actions = MakeErrorResult<OutputActions>(-5555, errMsg), .output_values = preflightUpdatedValues};
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
    const AttributeMatrix* selectedCellDataPtr = srcImageGeomPtr->getCellData();
    if(selectedCellDataPtr == nullptr)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-4014, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString())), .output_values = preflightUpdatedValues};
    }
    std::string cellDataName = selectedCellDataPtr->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(
        destImagePath, geomDims, targetOrigin, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, cellDataName, srcImageGeomPtr->getUnits()));

    // Create one destination array for each source Cell Attribute Matrix array.
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellDataPtr)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }

    preflightUpdatedValues.push_back(
        {"Cropped Image Geometry Info", nx::core::geometry_helpers::description::GenerateGeometryInfo(geomDims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]},
                                                                                                      targetOrigin, srcImageGeomPtr->getUnits())});
  }
  // The cleared Feature AM keeps its source shape until optional renumbering.
  if(options.clear_feature_attribute_matrix)
  {
    const auto* srcCellFeatureDataPtr = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAmPath);
    auto featureAmResult = ValidateFeatureAttributeMatrixInGeometry(dataStructure, srcImagePath, cellFeatureAmPath, -50563, "Feature Attribute Matrix");
    if(featureAmResult.invalid())
    {
      featureAmResult.errors().front().message += " Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.";
      return {.output_actions = ConvertInvalidResult<OutputActions>(std::move(featureAmResult)), .output_values = preflightUpdatedValues};
    }
    auto elementDataResult = ValidateFeatureAttributeMatrixNotElementData(dataStructure, cellFeatureAmPath);
    if(elementDataResult.invalid())
    {
      return {.output_actions = ConvertInvalidResult<OutputActions>(std::move(elementDataResult))};
    }
    if(shouldRenumberFeatures && !IsChildOfAttributeMatrix(dataStructure, options.feature_ids_path, *srcImageGeomPtr->getCellData()))
    {
      return {
          .output_actions = MakeErrorResult<OutputActions>(
              -50564, fmt::format("**Cell Feature Ids** DataArray '{}' must be in the Cell Attribute Matrix '{}' of the selected Image Geometry '{}'. Select the DataArray from that Attribute Matrix.",
                                  options.feature_ids_path.toString(), srcImagePath.createChildPath(srcImageGeomPtr->getCellData()->getName()).toString(), srcImagePath.toString())),
          .output_values = preflightUpdatedValues};
    }
    ignorePaths.push_back(cellFeatureAmPath);
    const DataPath destCellFeatureAmPath = destImagePath.createChildPath(cellFeatureAmPath.getTargetName());
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAmPath, srcCellFeatureDataPtr->getShape()));

    std::string clearedList;
    for(const auto& [identifier, object] : *srcCellFeatureDataPtr)
    {
      clearedList += "\n" + cellFeatureAmPath.createChildPath(object->getName()).toString();
    }
    std::string message = fmt::format("The Feature Attribute Matrix '{}' is recreated in the cropped geometry with no arrays.", cellFeatureAmPath.toString());
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
  else if(options.report_uncleared_feature_data)
  {
    preflightUpdatedValues.push_back({"Feature Data Not Cleared", "Attribute Matrices other than the Cell Attribute Matrix are copied to the cropped geometry unchanged. If one of them holds "
                                                                  "Feature data, its values describe the uncropped geometry and are not valid for the cropped geometry."});
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry
  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    AppendUnchangedAttributeMatricesWarning(dataStructure, childPaths.value(), ignorePaths, "cropped geometry", resultOutputActions);
    for(const auto& childPath : childPaths.value())
    {
      const DataPath copiedChildPath = childPath.rebase(srcImagePath, destImagePath).value();
      AppendCopyDataObjectAction(dataStructure, resultOutputActions.value(), childPath, copiedChildPath, options.clear_feature_attribute_matrix ? cellFeatureAmPath : DataPath{});
    }
  }

  if(pRemoveOriginalGeometry)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}
