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
#include "complex/Utilities/SamplingUtils.hpp"

namespace complex
{
namespace
{

IntVec3 getCurrentVolumeDataContainerDimensions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  IntVec3 data = {0, 0, 0};

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

std::string getOldBoxDimensions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  std::string desc = "Please select an Image Geometry ";
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(nullptr != image)
  {
    auto oldOrigin = image->getOrigin();
    auto oldDimensions = image->getDimensions();
    auto oldResolution = image->getSpacing();

    std::stringstream ss;
    ss << "X Range: " << oldOrigin[0] << " to " << (oldOrigin[0] + (oldDimensions[0] * oldResolution[0])) << " (Delta: " << (oldDimensions[0] * oldResolution[0]) << ") " << 0 << "-"
       << oldDimensions[0] - 1 << " Voxels\n";
    ss << "Y Range: " << oldOrigin[1] << " to " << (oldOrigin[1] + (oldDimensions[1] * oldResolution[1])) << " (Delta: " << (oldDimensions[1] * oldResolution[1]) << ") " << 0 << "-"
       << oldDimensions[1] - 1 << " Voxels\n";
    ss << "Z Range: " << oldOrigin[2] << " to " << (oldOrigin[2] + (oldDimensions[2] * oldResolution[2])) << " (Delta: " << (oldDimensions[2] * oldResolution[2]) << ") " << 0 << "-"
       << oldDimensions[2] - 1 << " Voxels\n";
    return ss.str();
  }
  return desc;
}

std::string getNewBoxDimensions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  std::string desc = std::string("Please select a Data Container that has an Image Geometry");
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(nullptr != image)
  {
    auto newOrigin = image->getOrigin();
    auto newDimensions = image->getDimensions();
    auto newResolution = image->getSpacing();

    std::stringstream ss;
    ss << "X Range: " << newOrigin[0] << " to " << (newOrigin[0] + (newDimensions[0] * newResolution[0])) << " (Delta: " << (newDimensions[0] * newResolution[0]) << ") " << 0 << "-"
       << newDimensions[0] - 1 << " Voxels\n";
    ss << "Y Range: " << newOrigin[1] << " to " << (newOrigin[1] + (newDimensions[1] * newResolution[1])) << " (Delta: " << (newDimensions[1] * newResolution[1]) << ") " << 0 << "-"
       << newDimensions[1] - 1 << " Voxels\n";
    ss << "Z Range: " << newOrigin[2] << " to " << (newOrigin[2] + (newDimensions[2] * newResolution[2])) << " (Delta: " << (newDimensions[2] * newResolution[2]) << ") " << 0 << "-"
       << newDimensions[2] - 1 << " Voxels\n";
    return ss.str();
  }
  return desc;
}
} // namespace

std::string CropImageGeometry::name() const
{
  return FilterTraits<CropImageGeometry>::name;
}

std::string CropImageGeometry::className() const
{
  return FilterTraits<CropImageGeometry>::className;
}

Uuid CropImageGeometry::uuid() const
{
  return FilterTraits<CropImageGeometry>::uuid;
}

std::string CropImageGeometry::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometry::defaultTags() const
{
  return {"#Core", "#Crop Image Geometry"};
}

Parameters CropImageGeometry::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geom", "DataPath to the target ImageGeom", DataPath(), std::set{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeom_Key, "New Image Geom", "DataPath to create the new ImageGeom at", DataPath()));

  params.insert(std::make_unique<Int32Parameter>(k_MinX_Key, "X Min Voxel (Column)", "Minimum X Voxel", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MinY_Key, "Y Min Voxel (Row)", "Minimum Y Voxel", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MinZ_Key, "Z Min Voxel (Plane)", "Minimum Z Voxel", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxX_Key, "X Max Voxel (Column) [Inclusive]", "Maximum X Voxel", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxY_Key, "Y Max Voxel (Row) [Inclusive]", "Maximum Y Voxel", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxZ_Key, "Z Max Voxel (Plane) [Inclusive]", "Maximum Z Voxel", 0));

  params.insert(std::make_unique<BoolParameter>(k_UpdateOrigin_Key, "Update Origin", "Specifies if the origin should be updated", false));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_VoxelArrays_Key, "Voxel Arrays", "DataPaths to related DataArrays", std::vector<DataPath>()));

  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature IDs", "DataPath to the target Feature IDs array", DataPath()));

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
  auto xMax = args.value<int32>(k_MinX_Key);
  auto xMin = args.value<int32>(k_MaxX_Key);
  auto yMax = args.value<int32>(k_MaxY_Key);
  auto yMin = args.value<int32>(k_MinY_Key);
  auto zMax = args.value<int32>(k_MaxZ_Key);
  auto zMin = args.value<int32>(k_MinZ_Key);
  auto shouldUpdateOrigin = args.value<bool>(k_UpdateOrigin_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto voxelArrayPaths = args.value<std::vector<DataPath>>(k_VoxelArrays_Key);
  auto newCellFeaturesName = args.value<std::string>(k_NewFeaturesName_Key);

  OutputActions actions;

  if(xMax < xMin)
  {
    std::string ss = fmt::format("X Max (%1) less than X Min (%2)", xMax, xMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(yMax < yMin)
  {
    std::string ss = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(zMax < zMin)
  {
    std::string ss = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(xMin < 0)
  {
    std::string ss = fmt::format("X Min ({}) less than 0", xMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(yMin < 0)
  {
    std::string ss = fmt::format("Y Min ({}) less than 0", yMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(zMin < 0)
  {
    std::string ss = fmt::format("Z Min ({}) less than 0", zMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }

  // Validate the incoming DataContainer, Geometry, and AttributeMatrix.
  // Provides {0, 0, 0} or {1, 1, 1} respectfully if the geometry could not be found.
  auto oldDimensions = getCurrentVolumeDataContainerDimensions(data, srcImagePath);
  auto oldResolution = getCurrentVolumeDataContainerResolutions(data, srcImagePath);

  auto srcImageGeom = data.getDataAs<ImageGeom>(srcImagePath);
  auto oldOrigin = srcImageGeom->getOrigin();

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

  IntVec3 newDimensions;
  newDimensions[0] = tDims[0];
  newDimensions[1] = tDims[1];
  newDimensions[2] = tDims[2];

  FloatVec3 newResolution;
  newResolution = oldResolution;

  std::vector<float32> newOrigin(3);
  if(shouldUpdateOrigin)
  {
    newOrigin[0] = xMin * newResolution[0] + oldOrigin[0];
    newOrigin[1] = yMin * newResolution[1] + oldOrigin[1];
    newOrigin[2] = zMin * newResolution[2] + oldOrigin[2];
  }
  else
  {
    newOrigin[0] = oldOrigin[0];
    newOrigin[1] = oldOrigin[1];
    newOrigin[2] = oldOrigin[2];
  }

  // saveAsNewImage
  {
    int64 XP = ((xMax - xMin) + 1);
    int64 YP = ((yMax - yMin) + 1);
    int64 ZP = ((zMax - zMin) + 1);

    std::vector<usize> tDims(3, 0);
    tDims[0] = XP;
    tDims[1] = YP;
    tDims[2] = ZP;

    auto spacing = srcImageGeom->getSpacing();
    std::vector<float32> spacingVec(3);
    for(usize i = 0; i < 3; i++)
    {
      spacingVec[i] = spacing[i];
    }
    auto action = std::make_unique<CreateImageGeometryAction>(destImagePath, tDims, newOrigin, spacingVec);
    actions.actions.push_back(std::move(action));

    auto newCellFeaturesPath = destImagePath.createChildPath(newCellFeaturesName);
    {
      auto action = std::make_unique<CreateDataGroupAction>(newCellFeaturesPath);
      actions.actions.push_back(std::move(action));
    }

    for(const auto& srcArrayPath : voxelArrayPaths)
    {
      auto* srcArray = data.getDataAs<IDataArray>(srcArrayPath);
      if(srcArray == nullptr)
      {
        std::string ss = fmt::format("Could not find the DataArray at path '{}'", srcArrayPath.toString());
        return {MakeErrorResult<OutputActions>(-5551, ss)};
      }

      auto numericType = static_cast<NumericType>(srcArray->getDataType());
      auto numTuples = srcArray->getNumberOfTuples();
      auto components = srcArray->getNumberOfComponents();
      auto dataArrayPath = newCellFeaturesPath.createChildPath(srcArrayPath.getTargetName());
      auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{numTuples}, std::vector<usize>{components}, dataArrayPath);
      actions.actions.push_back(std::move(action));
    }
  }

  usize totalPoints = srcImageGeom->getNumberOfElements();

  if(shouldRenumberFeatures)
  {
    std::vector<usize> cDims = {1};
    auto* featureIdsPtr = data.getDataAs<DataArray<int32>>(featureIdsArrayPath);
    if(nullptr == featureIdsPtr)
    {
      std::string ss = fmt::format("The DataArray '{}' which defines the Feature Ids to renumber is invalid. Does it exist? Is it the correct type?", featureIdsArrayPath.toString());
      return {MakeErrorResult<OutputActions>(-55500, ss)};
    }
    if(featureIdsPtr->getNumberOfComponents() != 1)
    {
      std::string ss = fmt::format("The Feature IDs array does not have the correct component dimensions. 1 component required. Array has {}", featureIdsPtr->getNumberOfComponents());
      return {MakeErrorResult<OutputActions>(-55501, ss)};
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
  auto xMax = args.value<int32>(k_MaxX_Key);
  auto xMin = args.value<int32>(k_MinX_Key);
  auto yMax = args.value<int32>(k_MaxY_Key);
  auto yMin = args.value<int32>(k_MinY_Key);
  auto zMax = args.value<int32>(k_MaxZ_Key);
  auto zMin = args.value<int32>(k_MinZ_Key);
  auto shouldUpdateOrigin = args.value<bool>(k_UpdateOrigin_Key);
  auto shouldRenumberFeatures = args.value<bool>(k_RenumberFeatures_Key);
  auto newFeaturesName = args.value<std::string>(k_NewFeaturesName_Key);
  auto voxelArrayPaths = args.value<std::vector<DataPath>>(k_VoxelArrays_Key);
  auto featureIdsArrayPath = args.value<DataPath>(k_FeatureIds_Key);

  auto& srcImageGeom = data.getDataRefAs<ImageGeom>(srcImagePath);
  auto& destImageGeom = data.getDataRefAs<ImageGeom>(destImagePath);

  if(xMax > (static_cast<int64>(destImageGeom.getNumXPoints()) - 1))
  {
    std::string ss = fmt::format("The X Max ({}) is greater than the Image Geometry X extent ({})", xMax, static_cast<int64>(destImageGeom.getNumXPoints()) - 1);
    return MakeErrorResult(-5550, ss);
  }

  if(yMax > (static_cast<int64>(destImageGeom.getNumYPoints()) - 1))
  {
    std::string ss = fmt::format("The Y Max ({}) is greater than the Image Geometry Y extent ({})", yMax, static_cast<int64>(destImageGeom.getNumYPoints()) - 1);
    return MakeErrorResult(-5550, ss);
  }

  if(zMax > (static_cast<int64>(destImageGeom.getNumZPoints()) - 1))
  {
    std::string ss = fmt::format("The Z Max ({}) is greater than the Image Geometry Z extent ({})", zMax, static_cast<int64>(destImageGeom.getNumZPoints()) - 1);
    return MakeErrorResult(-5550, ss);
  }

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to crop
  int64 totalPoints = srcImageGeom.getNumberOfElements();

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
    std::string ss = fmt::format("The Max X value ({}) is greater than the Image Geometry X entent ({})."
                                 " This may lead to junk data being filled into the extra space.",
                                 xMax, dims[0]);
    return MakeErrorResult(-950, ss);
  }
  if(dims[1] <= yMax)
  {
    std::string ss = fmt::format("The Max Y value ({}) is greater than the Image Geometry Y entent ({})."
                                 " This may lead to junk data being filled into the extra space.",
                                 yMax, dims[1]);
    return MakeErrorResult(-951, ss);
  }
  if(dims[2] <= zMax)
  {
    std::string ss = fmt::format("The Max Z value ({}) is greater than the Image Geometry Z entent ({})."
                                 " This may lead to junk data being filled into the extra space.",
                                 zMax, dims[2]);
    return MakeErrorResult(-952, ss);
  }

  int64 XP = ((xMax - xMin) + 1);
  int64 YP = ((yMax - yMin) + 1);
  int64 ZP = ((zMax - zMin) + 1);

  int64 col = 0, row = 0, plane = 0;
  int64 colold = 0, rowold = 0, planeold = 0;
  int64 index = 0;
  int64 index_old = 0;
  for(int64 i = 0; i < ZP; i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    // std::string ss = fmt::format("Cropping Volume || Slice {} of {} Complete", i, ZP);
    // notifyStatusMessage(ss);
    planeold = (i + zMin) * (srcImageGeom.getNumXPoints() * srcImageGeom.getNumYPoints());
    plane = (i * XP * YP);
    for(int64 j = 0; j < YP; j++)
    {
      rowold = (j + yMin) * srcImageGeom.getNumXPoints();
      row = (j * XP);
      for(int64 k = 0; k < XP; k++)
      {
        colold = (k + xMin);
        col = k;
        index_old = planeold + rowold + colold;
        index = plane + row + col;
        for(const auto& voxelPath : voxelArrayPaths)
        {
          auto& dataArray = data.getDataRefAs<IDataArray>(voxelPath);
          dataArray.copyTuple(index_old, index);
        }
      }
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  totalPoints = destImageGeom.getNumberOfElements();
  std::vector<usize> tDims(3, 0);
  tDims[0] = XP;
  tDims[1] = YP;
  tDims[2] = ZP;

  if(shouldRenumberFeatures)
  {
    DataPath destCellFeaturesPath = destImagePath.createChildPath(newFeaturesName);
    auto result = Sampling::RenumberFeatures(data, destImagePath, destCellFeaturesPath, featureIdsArrayPath, shouldCancel);
    if(result.invalid())
    {
      return result;
    }
  }

  if(shouldUpdateOrigin)
  {
    FloatVec3 resolution = destImageGeom.getSpacing();
    FloatVec3 origin = destImageGeom.getOrigin();

    origin[0] = xMin * resolution[0] + oldOrigin[0];
    origin[1] = yMin * resolution[1] + oldOrigin[1];
    origin[2] = zMin * resolution[2] + oldOrigin[2];

    destImageGeom.setOrigin(origin);
  }

  return {};
}
} // namespace complex
