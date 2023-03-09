#include "MapPointCloudToRegularGridFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <cmath>

namespace complex
{
namespace
{
constexpr int64 k_MissingVertexGeom = -2600;
constexpr int64 k_BadGridDimensions = -2601;
constexpr int64 k_MissingVoxelIndicesArray = -2602;
constexpr int64 k_MissingMaskArray = -2603;

void createRegularGrid(DataStructure& data, const Arguments& args)
{
  auto samplingGridType = args.value<uint64>(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key);
  auto gridDimensions = args.value<std::vector<int32>>(MapPointCloudToRegularGridFilter::k_GridDimensions_Key);
  auto vertexGeomPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key);
  auto newImageGeomPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key);
  auto existingImageGeomPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_ExistingImageGeometry_Key);
  auto arraysToMap = args.value<std::vector<DataPath>>(MapPointCloudToRegularGridFilter::k_ArraysToMap_Key);
  auto useMask = args.value<bool>(MapPointCloudToRegularGridFilter::k_UseMask_Key);
  auto maskArrayPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_MaskPath_Key);
  auto voxelIndicesPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key);

  std::string ss = fmt::format("Creating Regular Grid");
  // notifyStatusMessage(ss);

  VertexGeom* pointCloud = data.getDataAs<VertexGeom>(vertexGeomPath);
  ImageGeom* image = nullptr;
  if(samplingGridType == 0)
  {
    image = data.getDataAs<ImageGeom>(newImageGeomPath);
  }
  else if(samplingGridType == 1)
  {
    image = data.getDataAs<ImageGeom>(existingImageGeomPath);
  }

  int64 numVerts = pointCloud->getNumberOfVertices();
  auto* vertex = pointCloud->getVertices();

  auto mask = data.getDataAs<BoolArray>(maskArrayPath);

  // Find the largest/smallest (x,y,z) dimensions of the incoming data to be used to define the maximum dimensions for the regular grid
  std::vector<float32> meshMaxExtents;
  std::vector<float32> meshMinExtents;
  for(size_t i = 0; i < 3; i++)
  {
    meshMaxExtents.push_back(std::numeric_limits<float>::lowest());
    meshMinExtents.push_back(std::numeric_limits<float>::max());
  }

  for(int64_t i = 0; i < numVerts; i++)
  {
    if(!useMask || (useMask && (*mask)[i]))
    {
      if((*vertex)[3 * i] > meshMaxExtents[0])
      {
        meshMaxExtents[0] = (*vertex)[3 * i];
      }
      if((*vertex)[3 * i + 1] > meshMaxExtents[1])
      {
        meshMaxExtents[1] = (*vertex)[3 * i + 1];
      }
      if((*vertex)[3 * i + 2] > meshMaxExtents[2])
      {
        meshMaxExtents[2] = (*vertex)[3 * i + 2];
      }
      if((*vertex)[3 * i] < meshMinExtents[0])
      {
        meshMinExtents[0] = (*vertex)[3 * i];
      }
      if((*vertex)[3 * i + 1] < meshMinExtents[1])
      {
        meshMinExtents[1] = (*vertex)[3 * i + 1];
      }
      if((*vertex)[3 * i + 2] < meshMinExtents[2])
      {
        meshMinExtents[2] = (*vertex)[3 * i + 2];
      }
    }
  }

  SizeVec3 iDims = image->getDimensions();

  std::vector<float32> iRes(3, 0.0f);
  std::vector<float32> iOrigin(3, 0.0f);

  if(iDims[0] > 1)
  {
    iRes[0] = (meshMaxExtents[0] - meshMinExtents[0]) / (static_cast<float32>(iDims[0]));
    if(iRes[0] == 0.0)
    {
      iRes[0] = 1.0f;
      iDims[0] = 1;
    }
    else
    {
      iDims[0] += 1;
    }
    iOrigin[0] = meshMinExtents[0] - (iRes[0] / 2.0f);
  }
  else
  {
    iRes[0] = 1.25f * (meshMaxExtents[0] - meshMinExtents[0]) / (static_cast<float>(iDims[0]));
    if(iRes[0] == 0.0)
    {
      iRes[0] = 1.0f;
      iOrigin[0] = -0.5f;
    }
    else
    {
      iOrigin[0] = meshMinExtents[0] - (iRes[0] * 0.1f);
    }
  }

  if(iDims[1] > 1)
  {
    iRes[1] = (meshMaxExtents[1] - meshMinExtents[1]) / (static_cast<float32>(iDims[1]));
    if(iRes[1] == 0.0)
    {
      iRes[1] = 1.0f;
      iDims[1] = 1;
    }
    else
    {
      iDims[1] += 1;
    }
    iOrigin[1] = meshMinExtents[1] - (iRes[1] / 2.0f);
  }
  else
  {
    iRes[1] = 1.25f * (meshMaxExtents[1] - meshMinExtents[1]) / (static_cast<float32>(iDims[1]));
    if(iRes[1] == 0.0)
    {
      iRes[1] = 1.0f;
      iOrigin[1] = -0.5f;
    }
    else
    {
      iOrigin[1] = meshMinExtents[1] - (iRes[1] * 0.1f);
    }
  }

  if(iDims[2] > 1)
  {
    iRes[2] = (meshMaxExtents[2] - meshMinExtents[2]) / (static_cast<float32>(iDims[2]));
    if(iRes[2] == 0.0)
    {
      iRes[2] = 1.0f;
      iDims[2] = 1;
    }
    else
    {
      iDims[2] += 1;
    }
    iOrigin[2] = meshMinExtents[2] - (iRes[2] / 2.0f);
  }
  else
  {
    iRes[2] = 1.25f * (meshMaxExtents[2] - meshMinExtents[2]) / (static_cast<float32>(iDims[2]));
    if(iRes[2] == 0.0)
    {
      iRes[2] = 1.0f;
      iOrigin[2] = -0.5f;
    }
    else
    {
      iOrigin[2] = meshMinExtents[1] - (iRes[2] * 0.1f);
    }
  }

  image->setDimensions(iDims);
  image->setSpacing(iRes[0], iRes[1], iRes[2]);
  image->setOrigin(iOrigin[0], iOrigin[1], iOrigin[2]);
}
} // namespace

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::name() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::className() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MapPointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::humanName() const
{
  return "Map Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> MapPointCloudToRegularGridFilter::defaultTags() const
{
  return {"Alignment", "Point Cloud", "Grid", "Sampling", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters MapPointCloudToRegularGridFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplingGridType_Key, "Sampling Grid Type", "Specifies how data is saved or accessed", 0,
                                                                    std::vector<std::string>{"Manual", "Use Existing Image Geometry"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_GridDimensions_Key, "Grid Dimensions", "Target grid size", std::vector<int32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_ExistingImageGeometry_Key, "Image Geometry", "Path to the target Image Geometry", DataPath()));

  params.insert(std::make_unique<DataPathSelectionParameter>(k_VertexGeometry_Key, "Vertex Geometry", "Path to the target Vertex Geometry", DataPath()));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeometry_Key, "Image Geometry", "Path to create the Image Geometry", DataPath()));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_ArraysToMap_Key, "Arrays to Map", "Paths to map to the grid geometry", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies if a mask array should be used", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskPath_Key, "Mask", "Path to the target mask array", DataPath(), ArraySelectionParameter::AllowedTypes{DataType::boolean},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndices_Key, "Voxel Indices", "Path to the Voxel Indices array", DataPath(), ArraySelectionParameter::AllowedTypes{DataType::uint64}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the cell data attribute matrix to be created within the created Image Geometry",
                                                          ImageGeom::k_CellDataName));

  params.linkParameters(k_UseMask_Key, k_MaskPath_Key, std::make_any<bool>(true));
  params.linkParameters(k_SamplingGridType_Key, k_GridDimensions_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_SamplingGridType_Key, k_ExistingImageGeometry_Key, std::make_any<ChoicesParameter::ValueType>(1));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MapPointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<MapPointCloudToRegularGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MapPointCloudToRegularGridFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto samplingGridType = args.value<uint64>(k_SamplingGridType_Key);
  auto gridDimensions = args.value<std::vector<int32>>(k_GridDimensions_Key);
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeometry_Key);
  auto newImageGeomPath = args.value<DataPath>(k_NewImageGeometry_Key);
  auto existingImageGeomPath = args.value<DataPath>(k_ExistingImageGeometry_Key);
  auto arraysToMap = args.value<std::vector<DataPath>>(k_ArraysToMap_Key);
  auto useMask = args.value<bool>(k_UseMask_Key);
  auto maskArrayPath = args.value<DataPath>(k_MaskPath_Key);
  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndices_Key);
  auto cellDataName = args.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);

  OutputActions actions;

  std::vector<DataPath> dataArrays;

  // VertexGeom::Pointer vertex = getDataContainerArray()->getPrereqGeometryFromDataContainer<VertexGeom>(this, getDataContainerName());
  auto* vertexGeom = data.getDataAs<VertexGeom>(vertexGeomPath);
  if(vertexGeom == nullptr)
  {
    std::string ss = fmt::format("Could not find Vertex geometry at {}", vertexGeomPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertexGeom, ss}})};
  }
  else
  {
    // dataArrays.push_back(vertex->getVertices());
  }

  if(samplingGridType == 0)
  {
    if(gridDimensions[0] <= 0 || gridDimensions[1] <= 0 || gridDimensions[2] <= 0)
    {
      std::string ss = fmt::format("All grid dimensions must be positive.\n "
                                   "Current grid dimensions:\n x = {}\n y = {}\n z = {}\n",
                                   gridDimensions[0], gridDimensions[1], gridDimensions[2]);
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadGridDimensions, ss}})};
    }

    CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(gridDimensions[0]), static_cast<usize>(gridDimensions[1]), static_cast<usize>(gridDimensions[2])};
    CreateImageGeometryAction::OriginType origin = {0, 0, 0};
    CreateImageGeometryAction::SpacingType spacing = {1, 1, 1};
    auto imageAction = std::make_unique<CreateImageGeometryAction>(newImageGeomPath, dims, origin, spacing, cellDataName);
    actions.actions.push_back(std::move(imageAction));
  }

  if(samplingGridType == 1)
  {
    auto* vertex = data.getDataAs<ImageGeom>(existingImageGeomPath);
    if(vertex == nullptr)
    {
      std::string ss = fmt::format("Could not find Image geometry at {}", existingImageGeomPath.toString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertexGeom, ss}})};
    }
  }

  auto* voxelIndicesPtr = data.getDataAs<UInt64Array>(voxelIndicesPath);
  if(nullptr == voxelIndicesPtr)
  {
    std::string ss = fmt::format("Could not find Voxel Indices array at {}", voxelIndicesPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVoxelIndicesArray, ss}})};
  }
  else
  {
    dataArrays.push_back(voxelIndicesPath);
  }

  if(useMask)
  {
    auto maskPtr = data.getDataAs<BoolArray>(maskArrayPath);
    if(nullptr == maskPtr)
    {
      std::string ss = fmt::format("Could not find Mask array at {}", maskArrayPath.toString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingMaskArray, ss}})};
    }
    else
    {
      dataArrays.push_back(maskArrayPath);
    }
  }

  data.validateNumberOfTuples(dataArrays);

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> MapPointCloudToRegularGridFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto samplingGridType = args.value<uint64>(k_SamplingGridType_Key);
  auto gridDimensions = args.value<std::vector<int32>>(k_GridDimensions_Key);
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeometry_Key);
  auto newImageGeomPath = args.value<DataPath>(k_NewImageGeometry_Key);
  auto existingImageGeomPath = args.value<DataPath>(k_ExistingImageGeometry_Key);
  auto arraysToMap = args.value<std::vector<DataPath>>(k_ArraysToMap_Key);
  auto useMask = args.value<bool>(k_UseMask_Key);
  auto maskArrayPath = args.value<DataPath>(k_MaskPath_Key);
  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndices_Key);

  ImageGeom* image = nullptr;
  if(samplingGridType == 0)
  {
    // Create the regular grid
    createRegularGrid(data, args);
    image = data.getDataAs<ImageGeom>(newImageGeomPath);
  }
  else if(samplingGridType == 1)
  {
    image = data.getDataAs<ImageGeom>(existingImageGeomPath);
  }

  auto* vertices = data.getDataAs<VertexGeom>(vertexGeomPath);

  int64 numVerts = vertices->getNumberOfVertices();
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  FloatVec3 origin = image->getOrigin();
  usize idxs[3] = {0, 0, 0};
  int64 progIncrement = numVerts / 100;
  int64 prog = 1;
  int64 progressInt = 0;
  int64 counter = 0;

  auto maskPtr = data.getDataAs<BoolArray>(maskArrayPath);
  auto* voxelIndicesPtr = data.getDataAs<UInt64Array>(voxelIndicesPath);

  for(int64 i = 0; i < numVerts; i++)
  {
    if(!useMask || (useMask && (*maskPtr)[i]))
    {
      auto coords = vertices->getVertexCoordinate(i);

      for(usize j = 0; j < 3; j++)
      {
        /*if((coords[j] - origin[j]) < 0)
        {
          std::string ss = fmt::format("Found negative value for index computation of vertex {}, which may result in unsigned underflow", i);
          setWarningCondition(-1000, ss);
        }*/
        idxs[j] = int64(std::floor((coords[j] - origin[j]) / res[j]));
      }

      for(usize j = 0; j < 3; j++)
      {
        if(idxs[j] >= dims[j])
        {
          idxs[j] = (dims[j] - 1);
        }
      }

      (*voxelIndicesPtr)[i] = (idxs[2] * dims[1] * dims[0]) + (idxs[1] * dims[0]) + idxs[0];

      if(counter > prog)
      {
        progressInt = static_cast<int64>((static_cast<float>(counter) / numVerts) * 100.0f);
        // std::string ss = fmt::format("Computing Point Cloud Voxel Indices || {}% Completed", progressInt);
        // notifyStatusMessage(ss);
        prog = prog + progIncrement;
      }
      counter++;
    }
  }

  return {};
}
} // namespace complex
