#include "CropImageGeometry.hpp"

#include <sstream>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/SamplingUtils.hpp"

namespace complex
{
namespace
{
template <typename T>
void copyDataTuple(IDataArray& oldArray, IDataArray& newArray, usize oldIndex, usize newIndex)
{
  auto& oldArrayCast = static_cast<DataArray<T>&>(oldArray);
  auto& newArrayCast = static_cast<DataArray<T>&>(newArray);

  auto numComponents = oldArrayCast.getNumberOfComponents();

  for(usize i = 0; i < numComponents; i++)
  {
    usize newOffset = newIndex * numComponents;
    usize oldOffset = oldIndex * numComponents;
    newArrayCast[newOffset + i] = oldArrayCast[oldOffset + i];
  }
}

void copyDataArrayTuple(IDataArray& oldArray, IDataArray& newArray, usize oldIndex, usize newIndex)
{
  auto dataType = oldArray.getDataType();
  switch(dataType)
  {
  case DataType::boolean:
    copyDataTuple<bool>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::float32:
    copyDataTuple<float32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::float64:
    copyDataTuple<float64>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int8:
    copyDataTuple<int8>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int16:
    copyDataTuple<int16>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int32:
    copyDataTuple<int32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::int64:
    copyDataTuple<int64>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint8:
    copyDataTuple<uint8>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint16:
    copyDataTuple<uint16>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint32:
    copyDataTuple<uint32>(oldArray, newArray, oldIndex, newIndex);
    break;
  case DataType::uint64:
    copyDataTuple<uint64>(oldArray, newArray, oldIndex, newIndex);
    break;
  default:
    break;
  }
}

USizeVec3 getCurrentVolumeDataContainerDimensions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  USizeVec3 data = {0, 0, 0};

  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(image != nullptr)
  {
    data[0] = image->getNumXPoints();
    data[1] = image->getNumYPoints();
    data[2] = image->getNumZPoints();
  }
  return data;
}

FloatVec3 getCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
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
std::string CropImageGeometry::name() const
{
  return FilterTraits<CropImageGeometry>::name;
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::className() const
{
  return FilterTraits<CropImageGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CropImageGeometry::uuid() const
{
  return FilterTraits<CropImageGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometry::defaultTags() const
{
  return {"#Core", "#Crop Image Geometry"};
}

//------------------------------------------------------------------------------
Parameters CropImageGeometry::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geom", "DataPath to the target ImageGeom", DataPath(), std::set{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeom_Key, "New Image Geom", "DataPath to create the new ImageGeom at", DataPath()));

  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinVoxel_Key, "Min Voxel", "", std::vector<uint64>{0, 0, 0}, std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxVoxel_Key, "Max Voxel [Inclusive]", "", std::vector<uint64>{0, 0, 0}, std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));

  params.insert(std::make_unique<BoolParameter>(k_UpdateOrigin_Key, "Update Origin", "Specifies if the origin should be updated", false));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_VoxelArrays_Key, "Voxel Arrays", "DataPaths to related DataArrays", std::vector<DataPath>(), complex::GetAllDataTypes()));

  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature IDs", "DataPath to Feature IDs array", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, true));

  params.insert(std::make_unique<StringParameter>(k_NewFeaturesName_Key, "New Cell Features Group Name", "Name of the new DataGroup containing updated updated Voxel Arrays", "Cell Features"));
  return params;
}

IFilter::UniquePointer CropImageGeometry::clone() const
{
  return std::make_unique<CropImageGeometry>();
}

IFilter::PreflightResult CropImageGeometry::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto srcImagePath = args.value<DataPath>(k_ImageGeom_Key);
  auto destImagePath = args.value<DataPath>(k_NewImageGeom_Key);
  auto featureIdsArrayPath = args.value<DataPath>(k_FeatureIds_Key);
  auto minVoxels = args.value<std::vector<uint64>>(k_MinVoxel_Key);
  auto maxVoxels = args.value<std::vector<uint64>>(k_MaxVoxel_Key);
  auto shouldUpdateOrigin = args.value<bool>(k_UpdateOrigin_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto voxelArrayPaths = args.value<std::vector<DataPath>>(k_VoxelArrays_Key);
  auto newCellFeaturesName = args.value<std::string>(k_NewFeaturesName_Key);

  auto xMin = minVoxels[0];
  auto xMax = maxVoxels[0];
  auto yMax = maxVoxels[1];
  auto yMin = minVoxels[1];
  auto zMax = maxVoxels[2];
  auto zMin = minVoxels[2];

  OutputActions actions;

  if(xMax < xMin)
  {
    std::string errMsg = fmt::format("X Max (%1) less than X Min (%2)", xMax, xMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(yMax < yMin)
  {
    std::string errMsg = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(zMax < zMin)
  {
    std::string errMsg = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(xMin < 0)
  {
    std::string errMsg = fmt::format("X Min ({}) less than 0", xMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(yMin < 0)
  {
    std::string errMsg = fmt::format("Y Min ({}) less than 0", yMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }
  if(zMin < 0)
  {
    std::string errMsg = fmt::format("Z Min ({}) less than 0", zMin);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  // Validate the incoming DataContainer, Geometry, and AttributeMatrix.
  // Provides {0, 0, 0} or {1, 1, 1} respectfully if the geometry could not be found.
  auto srcDimensions = getCurrentVolumeDataContainerDimensions(data, srcImagePath);
  auto srcSpacing = getCurrentVolumeDataContainerResolutions(data, srcImagePath);

  const auto* srcImageGeom = data.getDataAs<ImageGeom>(srcImagePath);
  auto srcOrigin = srcImageGeom->getOrigin();

  if(xMax > srcImageGeom->getNumXPoints() - 1)
  {
    std::string errMsg = fmt::format("The X Max ({}) is greater than the Image Geometry X extent ({})", xMax, srcImageGeom->getNumXPoints() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  if(yMax > srcImageGeom->getNumYPoints() - 1)
  {
    std::string errMsg = fmt::format("The Y Max ({}) is greater than the Image Geometry Y extent ({})", yMax, srcImageGeom->getNumYPoints() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  if(zMax > srcImageGeom->getNumZPoints() - 1)
  {
    std::string errMsg = fmt::format("The Z Max ({}) is greater than the Image Geometry Z extent ({})", zMax, srcImageGeom->getNumZPoints() - 1);
    return {MakeErrorResult<OutputActions>(-5550, errMsg)};
  }

  const usize requiredTupleCount = srcDimensions[0] * srcDimensions[1] * srcDimensions[2];

  std::vector<usize> tDims(3, 0);
  if(xMax - xMin < 0)
  {
    xMax = xMin + 1;
  }
  if(yMax - yMin < 0)
  {
    yMax = yMin + 1;
  }
  if(zMax - zMin < 0)
  {
    zMax = zMin + 1;
  }

  tDims[0] = (xMax - xMin) + 1;
  tDims[1] = (yMax - yMin) + 1;
  tDims[2] = (zMax - zMin) + 1;

  FloatVec3 targetSpacing;
  targetSpacing = srcSpacing;

  std::vector<float32> targetOrigin(3);
  if(shouldUpdateOrigin)
  {
    targetOrigin[0] = static_cast<float>(xMin) * targetSpacing[0] + srcOrigin[0];
    targetOrigin[1] = static_cast<float>(yMin) * targetSpacing[1] + srcOrigin[1];
    targetOrigin[2] = static_cast<float>(zMin) * targetSpacing[2] + srcOrigin[2];
  }
  else
  {
    targetOrigin[0] = srcOrigin[0];
    targetOrigin[1] = srcOrigin[1];
    targetOrigin[2] = srcOrigin[2];
  }

  // saveAsNewImage
  {
    uint64 XP = ((xMax - xMin) + 1);
    uint64 YP = ((yMax - yMin) + 1);
    uint64 ZP = ((zMax - zMin) + 1);

    std::vector<usize> tDims = {XP, YP, ZP};

    auto spacing = srcImageGeom->getSpacing();
    std::vector<float32> spacingVec(3);
    for(usize i = 0; i < 3; i++)
    {
      spacingVec[i] = spacing[i];
    }
    auto action = std::make_unique<CreateImageGeometryAction>(destImagePath, tDims, targetOrigin, spacingVec);
    actions.actions.push_back(std::move(action));

    auto newCellFeaturesPath = destImagePath.createChildPath(newCellFeaturesName);
    {
      auto action = std::make_unique<CreateDataGroupAction>(newCellFeaturesPath);
      actions.actions.push_back(std::move(action));
    }

    for(const auto& srcArrayPath : voxelArrayPaths)
    {
      const auto* srcArray = data.getDataAs<IDataArray>(srcArrayPath);
      if(srcArray == nullptr)
      {
        std::string errMsg = fmt::format("Could not find the DataArray at path '{}'", srcArrayPath.toString());
        return {MakeErrorResult<OutputActions>(-5551, errMsg)};
      }
      if(srcArray->getNumberOfTuples() != requiredTupleCount)
      {
        std::string errMsg = fmt::format("DataArray at path '{}' does not match the required tuple count of '{}'", srcArrayPath.toString(), requiredTupleCount);
        return {MakeErrorResult<OutputActions>(-5551, errMsg)};
      }

      DataType dataType = srcArray->getDataType();
      auto components = srcArray->getNumberOfComponents();
      auto dataArrayPath = newCellFeaturesPath.createChildPath(srcArrayPath.getTargetName());
      auto action = std::make_unique<CreateArrayAction>(dataType, tDims, std::vector<usize>{components}, dataArrayPath);
      actions.actions.push_back(std::move(action));
    }
  }

  if(shouldRenumberFeatures)
  {
    std::vector<usize> cDims = {1};
    const auto* featureIdsPtr = data.getDataAs<DataArray<int32>>(featureIdsArrayPath);
    if(nullptr == featureIdsPtr)
    {
      std::string errMsg = fmt::format("The DataArray '{}' which defines the Feature Ids to renumber is invalid. Does it exist? Is it the correct type?", featureIdsArrayPath.toString());
      return {MakeErrorResult<OutputActions>(-55500, errMsg)};
    }
    if(featureIdsPtr->getNumberOfComponents() != 1)
    {
      std::string errMsg = fmt::format("The Feature IDs array does not have the correct component dimensions. 1 component required. Array has {}", featureIdsPtr->getNumberOfComponents());
      return {MakeErrorResult<OutputActions>(-55501, errMsg)};
    }
  }

  // auto action = std::make_unique<CropImageGeometryAction>(DataPath);
  // actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CropImageGeometry::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto srcImagePath = args.value<DataPath>(k_ImageGeom_Key);
  auto destImagePath = args.value<DataPath>(k_NewImageGeom_Key);
  auto minVoxels = args.value<std::vector<uint64>>(k_MinVoxel_Key);
  auto maxVoxels = args.value<std::vector<uint64>>(k_MaxVoxel_Key);
  // auto shouldUpdateOrigin = args.value<bool>(k_UpdateOrigin_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto newFeaturesName = args.value<std::string>(k_NewFeaturesName_Key);
  auto voxelArrayPaths = args.value<std::vector<DataPath>>(k_VoxelArrays_Key);
  auto featureIdsArrayPath = args.value<DataPath>(k_FeatureIds_Key);

  uint64 xMin = minVoxels[0];
  uint64 xMax = maxVoxels[0];
  uint64 yMax = maxVoxels[1];
  uint64 yMin = minVoxels[1];
  uint64 zMax = maxVoxels[2];
  uint64 zMin = minVoxels[2];

  auto& srcImageGeom = data.getDataRefAs<ImageGeom>(srcImagePath);
  auto& destImageGeom = data.getDataRefAs<ImageGeom>(destImagePath);

  DataPath newVoxelParentPath = destImagePath.createChildPath(newFeaturesName);

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to crop

  SizeVec3 udims = srcImageGeom.getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Check to see if the dims have actually changed.
  if(dims[0] == (xMax - xMin) && dims[1] == (yMax - yMin) && dims[2] == (zMax - zMin))
  {
    return {};
  }

  // Get current origin
  FloatVec3 oldOrigin = destImageGeom.getOrigin();

  // Check to make sure the new dimensions are not "out of bounds" and warn the user if they are
  if(dims[0] <= xMax)
  {
    std::string errMsg = fmt::format("The Max X value ({}) is greater than the Image Geometry X entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     xMax, dims[0]);
    return MakeErrorResult(-950, errMsg);
  }
  if(dims[1] <= yMax)
  {
    std::string errMsg = fmt::format("The Max Y value ({}) is greater than the Image Geometry Y entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     yMax, dims[1]);
    return MakeErrorResult(-951, errMsg);
  }
  if(dims[2] <= zMax)
  {
    std::string errMsg = fmt::format("The Max Z value ({}) is greater than the Image Geometry Z entent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     zMax, dims[2]);
    return MakeErrorResult(-952, errMsg);
  }

  uint64 xExtent = ((xMax - xMin) + 1);
  uint64 yExtent = ((yMax - yMin) + 1);
  uint64 zExtent = ((zMax - zMin) + 1);

  for(const auto& voxelPath : voxelArrayPaths)
  {
    auto& oldDataArray = data.getDataRefAs<IDataArray>(voxelPath);
    auto& newDataArray = data.getDataRefAs<IDataArray>(newVoxelParentPath.createChildPath(voxelPath.getTargetName()));

    std::string progMsg = fmt::format("Cropping Volume || Copying Data Array {}", voxelPath.getTargetName());
    messageHandler.operator()(progMsg);

    uint64 col = 0;
    uint64 row = 0;
    uint64 plane = 0;

    uint64 colold = 0;
    uint64 rowold = 0;
    uint64 planeold = 0;
    uint64 index = 0;
    uint64 index_old = 0;

    // Loop over every tuple in this array
    for(int64 i = 0; i < zExtent; i++)
    {
      if(shouldCancel)
      {
        return {};
      }

      planeold = (i + zMin) * (srcImageGeom.getNumXPoints() * srcImageGeom.getNumYPoints());
      plane = (i * xExtent * yExtent);
      for(int64 j = 0; j < yExtent; j++)
      {
        rowold = (j + yMin) * srcImageGeom.getNumXPoints();
        row = (j * xExtent);
        for(int64 k = 0; k < xExtent; k++)
        {
          colold = (k + xMin);
          col = k;
          index_old = planeold + rowold + colold;
          index = plane + row + col;

          copyDataArrayTuple(oldDataArray, newDataArray, index_old, index);
        }
      }
    }
  }

  if(shouldCancel)
  {
    return {};
  }
  std::vector<usize> tDims(3, 0);
  tDims[0] = xExtent;
  tDims[1] = yExtent;
  tDims[2] = zExtent;

  if(shouldRenumberFeatures)
  {
    DataPath destCellFeaturesPath = destImagePath.createChildPath(newFeaturesName);
    auto result = Sampling::RenumberFeatures(data, destImagePath, destCellFeaturesPath, featureIdsArrayPath, shouldCancel);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
} // namespace complex
