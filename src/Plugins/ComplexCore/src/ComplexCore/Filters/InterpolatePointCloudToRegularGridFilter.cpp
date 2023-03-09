#include "InterpolatePointCloudToRegularGridFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <cmath>

namespace complex
{
namespace
{
constexpr int64 k_MissingVertexGeom = -24500;
constexpr int64 k_MissingImageGeom = -24501;

struct MapPointCloudDataByKernelFunctor
{
  template <typename T>
  void operator()(IDataArray* source, INeighborList* dynamic, std::vector<float>& kernelVals, const int64 kernel[3], const usize dims[3], usize curX, usize curY, usize curZ, usize vertIdx)
  {
    auto* inputDataArray = dynamic_cast<DataArray<T>*>(source);
    auto& inputData = inputDataArray->getDataStoreRef();
    auto* interpolatedDataPtr = dynamic_cast<NeighborList<T>*>(dynamic);

    usize index = 0;
    int64 startKernel[3] = {0, 0, 0};
    int64 endKernel[3] = {0, 0, 0};
    usize counter = 0;

    kernel[0] > int64(curX) ? startKernel[0] = 0 : startKernel[0] = curX - kernel[0];
    kernel[1] > int64(curY) ? startKernel[1] = 0 : startKernel[1] = curY - kernel[1];
    kernel[2] > int64(curZ) ? startKernel[2] = 0 : startKernel[2] = curZ - kernel[2];

    int64(curX) + kernel[0] >= int64(dims[0]) ? endKernel[0] = dims[0] - 1 : endKernel[0] = curX + kernel[0];
    int64(curY) + kernel[1] >= int64(dims[1]) ? endKernel[1] = dims[1] - 1 : endKernel[1] = curY + kernel[1];
    endKernel[2] = int64(curZ);

    for(int64 z = startKernel[2]; z <= endKernel[2]; z++)
    {
      for(int64 y = startKernel[1]; y <= endKernel[1]; y++)
      {
        for(int64 x = startKernel[0]; x <= endKernel[0]; x++)
        {
          if(kernelVals[counter] == 0.0f)
          {
            continue;
          }
          index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
          interpolatedDataPtr->addEntry(index, kernelVals[counter] * inputData[vertIdx]);
          counter++;
        }
      }
    }
  }
};

void determineKernel(uint64 interpolationTechnique, const FloatVec3& sigmas, std::vector<float32>& kernel, const int64 kernelNumVoxels[3])
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        if(interpolationTechnique == 0)
        {
          kernel[counter] = 1.0f;
        }
        else if(interpolationTechnique == 1)
        {
          kernel[counter] = static_cast<float32>(std::exp(-((x * x) / (2 * sigmas[0] * sigmas[0]) + (y * y) / (2 * sigmas[1] * sigmas[1]) + (z * z) / (2 * sigmas[2] * sigmas[2]))));
        }
        counter++;
      }
    }
  }
}

void determineKernelDistances(std::vector<float32>& kernelValDistances, int64 kernelNumVoxels[3], FloatVec3 res)
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        kernelValDistances[counter] = (x * x * res[0] * res[0]) + (y * y * res[1] * res[1]) + (z * z * res[2] * res[2]);
        kernelValDistances[counter] = static_cast<float32>(std::sqrt(kernelValDistances[counter]));
        counter++;
      }
    }
  }
}

void mapKernelDistances(NeighborList<float32>* kernelDistances, std::vector<float32>& kernelValDistances, int64 kernel[3], usize dims[3], usize curX, usize curY, usize curZ)
{
  usize index = 0;
  int64 startKernel[3] = {0, 0, 0};
  int64 endKernel[3] = {0, 0, 0};
  usize counter = 0;

  kernel[0] > int64(curX) ? startKernel[0] = 0 : startKernel[0] = curX - kernel[0];
  kernel[1] > int64(curY) ? startKernel[1] = 0 : startKernel[1] = curY - kernel[1];
  kernel[2] > int64(curZ) ? startKernel[2] = 0 : startKernel[2] = curZ - kernel[2];

  int64(curX) + kernel[0] >= int64(dims[0]) ? endKernel[0] = dims[0] - 1 : endKernel[0] = curX + kernel[0];
  int64(curY) + kernel[1] >= int64(dims[1]) ? endKernel[1] = dims[1] - 1 : endKernel[1] = curY + kernel[1];
  endKernel[2] = int64(curZ);

  for(int64 z = startKernel[2]; z <= endKernel[2]; z++)
  {
    for(int64 y = startKernel[1]; y <= endKernel[1]; y++)
    {
      for(int64 x = startKernel[0]; x <= endKernel[0]; x++)
      {
        if(kernel[counter] == 0.0f)
        {
          continue;
        }
        index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
        kernelDistances->addEntry(index, kernelValDistances[counter]);
        counter++;
      }
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::name() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::className() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InterpolatePointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::humanName() const
{
  return "Interpolate Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> InterpolatePointCloudToRegularGridFilter::defaultTags() const
{
  return {"Geometry", "Gaussian", "Kernel", "Interpolation", "Point Cloud", "Vertex Geometry"};
}

//------------------------------------------------------------------------------
Parameters InterpolatePointCloudToRegularGridFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies whether or not to use a mask array", true));
  params.insert(std::make_unique<BoolParameter>(k_StoreKernelDistances_Key, "Store Kernel Distances", "Specifies whether or not to store kernel distances", true));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationTechnique_Key, "Interpolation Technique", "Selected Interpolation Technique", 0, std::vector<std::string>{"Uniform", "Gaussian"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_KernelSize_Key, "Kernel Size", "Specifies the kernel size", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_GaussianSigmas_Key, "Gaussian Sigmas", "Specifies the Gaussian sigmas", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_VertexGeom_Key, "Vertex Geometry to Interpolate", "DataPath to geometry to interpolate", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_ImageGeom_Key, "Interpolated Image Geometry", "DataPath to interpolated geometry", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndices_Key, "Voxel Indices", "DataPath to voxel indices", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_Mask_Key, "Mask", "DataPath to mask array", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InterpolateArrays_Key, "Attribute Arrays to Interpolate", "DataPaths to interpolate", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllNumericTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyArrays_Key, "Attribute Arrays to Copy", "DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_InterpolatedGroup_Key, "Interpolated Group", "DataPath to created DataGroup for interpolated data", DataPath()));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_KernelDistancesGroup_Key, "Kernel Distances Group", "DataPath to created DataGroup for kernel distances data", DataPath()));

  params.linkParameters(k_UseMask_Key, k_Mask_Key, std::make_any<bool>(true));

  return params;
}

IFilter::UniquePointer InterpolatePointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<InterpolatePointCloudToRegularGridFilter>();
}

IFilter::PreflightResult InterpolatePointCloudToRegularGridFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto interpolatedGroupPath = args.value<DataPath>(k_InterpolatedGroup_Key);
  auto kernelDistancesGroupPath = args.value<DataPath>(k_KernelDistancesGroup_Key);

  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndices_Key);

  auto interpolatedDataPaths = args.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = args.value<std::vector<DataPath>>(k_CopyArrays_Key);

  auto useMask = args.value<bool>(k_UseMask_Key);
  auto storeKernelDistances = args.value<bool>(k_StoreKernelDistances_Key);

  auto maskArrayPath = args.value<DataPath>(k_Mask_Key);

  auto interpolationTechnique = args.value<uint64>(k_InterpolationTechnique_Key);
  auto kernelSize = args.value<std::vector<float32>>(k_KernelSize_Key);
  auto sigmas = args.value<std::vector<float32>>(k_GaussianSigmas_Key);

  std::vector<const IDataArray*> dataArrays;
  OutputActions actions;

  auto vertexGeom = data.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = data.getDataAs<ImageGeom>(imageGeomPath);

  if(vertexGeom == nullptr)
  {
    std::string ss = fmt::format("Vertex Geometry cannot be found");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertexGeom, ss}})};
  }
  if(image == nullptr)
  {
    std::string ss = fmt::format("Image Geometry cannot be found");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingImageGeom, ss}})};
  }

  dataArrays.push_back(vertexGeom->getVertices());

  if(interpolationTechnique < 0 || interpolationTechnique > 1)
  {
    std::string ss = fmt::format("Interpolation Technique must be 0 [Uniform] or 1 [Gaussian] ");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  if(kernelSize[0] < 0 || kernelSize[1] < 0 || kernelSize[2] < 0)
  {
    std::string ss = fmt::format("All kernel dimensions must be positive.\n "
                                 "Current kernel dimensions:\n x = %1\n y = %2\n z = %3\n",
                                 kernelSize[0], kernelSize[1], kernelSize[2]);
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  if(sigmas[0] <= 0 || sigmas[1] <= 0 || sigmas[2] <= 0)
  {
    std::string ss = fmt::format("All sigmas must be positive.\n "
                                 "Current sigmas:\n x = %1\n y = %2\n z = %3\n",
                                 sigmas[0], sigmas[1], sigmas[2]);
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  // Create Output Groups
  {
    auto createGroupAction = std::make_unique<CreateDataGroupAction>(interpolatedGroupPath);
    actions.actions.push_back(std::move(createGroupAction));
  }
  {
    auto createGroupAction = std::make_unique<CreateDataGroupAction>(kernelDistancesGroupPath);
    actions.actions.push_back(std::move(createGroupAction));
  }

  // Get the list of all attribute matrices in the input data container and add them to the regular grid data container
  // auto m = data.getDataAs<DataGroup>(dataGroupPath);
  // auto interpolatedDC = data.getDataAs<DataGroup>(interpolatedDataPath);
  // m_AttrMatList = m->getAttributeMatrixNames();
  // SizeVec3 dims = image->getDimensions();
  // std::vector<usize> tDims = {dims[0], dims[1], dims[2]};
  // std::list<std::string> tempDataArrayList;
  // DataPath tempPath;
  // std::string tempAttrMatName;

  // All vertex data in the original point cloud will be interpolated to the regular grid's cells
  // Feature/Ensemble attribute matrices will remain unchanged and are deep copied to the new data container below
  // Create the attribute matrix where all the interpolated data will be stored

  // Loop through all the attribute matrices in the original data container
  // If we are in a vertex attribute matrix, create data arrays for all in the new interpolated data attribute matrix
  // Else, we are in a feature/ensemble attribute matrix, and just deep copy it into the new data container

  const usize cDims = 1;

  for(const auto& interpolatePath : interpolatedDataPaths)
  {
    auto targetArray = data.getDataAs<IDataArray>(interpolatePath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    auto copyAction = std::make_unique<CopyArrayInstanceAction>(interpolatePath, targetPath);
    actions.actions.push_back(std::move(copyAction));

    dataArrays.push_back(targetArray);
    auto tmpDims = targetArray->getNumberOfComponents();
    if(tmpDims != 1)
    {
      std::string ss = fmt::format("Attribute Arrays selected for copying must be scalar arrays");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-11002, ss}})};
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName() + " Neighbors");
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, targetArray->getNumberOfTuples(), neighborPath);
      actions.actions.push_back(std::move(neighborAction));
    }
  }

  for(const auto& copyPath : copyDataPaths)
  {
    auto targetArray = data.getDataAs<IDataArray>(copyPath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    auto copyAction = std::make_unique<CopyArrayInstanceAction>(copyPath, targetPath);
    actions.actions.push_back(std::move(copyAction));

    dataArrays.push_back(targetArray);
    auto tmpDims = targetArray->getNumberOfComponents();
    if(tmpDims != 1)
    {
      std::string ss = fmt::format("Attribute Arrays selected for copying must be scalar arrays");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-11002, ss}})};
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName() + " Neighbors");
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, targetArray->getNumberOfTuples(), neighborPath);
      actions.actions.push_back(std::move(neighborAction));
    }
  }

  auto voxelIndicesPtr = data.getDataAs<UInt64Array>(voxelIndicesPath);
  if(nullptr != voxelIndicesPtr)
  {
    // voxelIndices = voxelIndicesPtr->getDataStore();
    /* Now assign the raw pointer to data from the DataArray<T> object */
    dataArrays.push_back(voxelIndicesPtr);
  }

  if(useMask)
  {
    auto maskPtr = data.getDataAs<BoolArray>(maskArrayPath);
    if(nullptr != maskPtr)
    {
      // mask = maskPtr->getDataStore();
      /* Now assign the raw pointer to data from the DataArray<T> object */
      dataArrays.push_back(maskPtr);
    }
  }

  if(storeKernelDistances)
  {
    auto kernelDistancesDataPath = kernelDistancesGroupPath.createChildPath("Neighbor List");
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, 0, kernelDistancesDataPath);
    actions.actions.push_back(std::move(action));
  }

  // data.validateNumberOfTuples(dataArrays);

  return {std::move(actions)};
}

Result<> InterpolatePointCloudToRegularGridFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto interpolatedDataPath = args.value<DataPath>(k_InterpolatedGroup_Key);
  auto kernelDistancesDataPath = args.value<DataPath>(k_KernelDistancesGroup_Key);

  auto interpolatedDataPaths = args.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = args.value<std::vector<DataPath>>(k_CopyArrays_Key);

  auto useMask = args.value<bool>(k_UseMask_Key);
  auto storeKernelDistances = args.value<bool>(k_StoreKernelDistances_Key);
  auto maskPath = args.value<DataPath>(k_Mask_Key);
  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndices_Key);

  auto interpolationTechnique = args.value<uint64>(k_InterpolationTechnique_Key);
  auto kernelSize = args.value<std::vector<float32>>(k_KernelSize_Key);
  auto sigmas = args.value<std::vector<float32>>(k_GaussianSigmas_Key);

  auto interpolatedTechnique = args.value<uint64>(k_InterpolationTechnique_Key);

  //
  auto vertices = data.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = data.getDataAs<ImageGeom>(imageGeomPath);
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  int64 kernelNumVoxels[3] = {0, 0, 0};

  auto numVerts = vertices->getNumberOfVertices();
  usize index = 0;
  usize x = 0;
  usize y = 0;
  usize z = 0;

  std::vector<float32> kernel;

  auto maskArray = data.getDataAs<BoolArray>(maskPath);
  auto mask = maskArray->getDataStore();

  auto voxelIndicesArray = data.getDataAs<DataArray<uint64>>(voxelIndicesPath);
  auto voxelIndices = voxelIndicesArray->getDataStore();

  usize maxImageIndex = ((dims[2] - 1) * dims[0] * dims[1]) + ((dims[1] - 1) * dims[0]) + (dims[0] - 1);

  kernelNumVoxels[0] = int64(std::ceil((kernelSize[0] / res[0]) * 0.5f));
  kernelNumVoxels[1] = int64(std::ceil((kernelSize[1] / res[1]) * 0.5f));
  kernelNumVoxels[2] = int64(std::ceil((kernelSize[2] / res[2]) * 0.5f));

  if(kernelSize[0] < res[0])
  {
    kernelNumVoxels[0] = 0;
  }
  if(kernelSize[1] < res[1])
  {
    kernelNumVoxels[1] = 0;
  }
  if(kernelSize[2] < res[2])
  {
    kernelNumVoxels[2] = 0;
  }

  int64 tmpKernelSize[3] = {1, 1, 1};
  int64 totalKernel = 0;

  for(usize i = 0; i < 3; i++)
  {
    tmpKernelSize[i] *= (kernelNumVoxels[i] * 2) + 1;
  }

  totalKernel = tmpKernelSize[0] * tmpKernelSize[1] * tmpKernelSize[2];

  kernel.resize(totalKernel);
  std::fill(kernel.begin(), kernel.end(), 0.0f);
  determineKernel(interpolatedTechnique, sigmas, kernel, kernelNumVoxels);

  std::vector<float> uniformKernel(totalKernel, 1.0f);

  std::vector<float32> kernelValDistances;
  if(storeKernelDistances)
  {
    kernelValDistances.resize(totalKernel);
    std::fill(kernelValDistances.begin(), kernelValDistances.end(), 0.0f);
    determineKernelDistances(kernelValDistances, kernelNumVoxels, res);
  }

  usize progIncrement = numVerts / 100;
  usize prog = 1;
  usize progressInt = 0;

  for(usize i = 0; i < numVerts; i++)
  {
    if(useMask)
    {
      if(!mask->getValue(i))
      {
        continue;
      }
    }
    index = voxelIndices->getValue(i);
    if(index > maxImageIndex)
    {
      std::string ss = fmt::format("Index present in the selected Voxel Indices array that falls outside the selected Image Geometry for interpolation.\n Index = %1\n Max Image Index = %2\n", index,
                                   maxImageIndex);
      return {nonstd::make_unexpected(std::vector<Error>{Error{-1, ss}})};
    }
    x = index % dims[0];
    y = (index / dims[0]) % dims[1];
    z = index / (dims[0] * dims[1]);

    if(!interpolatedDataPaths.empty())
    {
      for(const auto& interpolatedDataPathItem : interpolatedDataPaths)
      {
        auto parentPath = interpolatedDataPathItem.getParent();
        auto dynamicArrayPath = parentPath.createChildPath(interpolatedDataPathItem.getTargetName() + "Neighbors");
        auto* dynamicArrayToInterpolate = data.getDataAs<INeighborList>(dynamicArrayPath);
        auto* interpolatedArray = data.getDataAs<IDataArray>(interpolatedDataPathItem);

        const auto& type = interpolatedArray->getDataType();
        if(type == DataType::boolean) // Cant be executed will throw error
        {
          continue;
        }

        // NO BOOL
        ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, interpolatedArray, dynamicArrayToInterpolate, kernel, kernelNumVoxels, dims.data(), x, y, z, i);
      }
    }

    if(copyDataPaths.size() > 0)
    {
      for(const auto& copyDataPath : copyDataPaths)
      {
        auto dynamicArrayPath = interpolatedDataPath.createChildPath(copyDataPath.getTargetName() + " Neighbors");
        auto* dynamicArrayToCopy = data.getDataAs<INeighborList>(dynamicArrayPath);
        auto* copyArray = data.getDataAs<IDataArray>(copyDataPath);

        const auto& type = copyArray->getDataType();
        if(type == DataType::boolean) // Cant be executed will throw error
        {
          continue;
        }

        // NO BOOL
        ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, copyArray, dynamicArrayToCopy, uniformKernel, kernelNumVoxels, dims.data(), x, y, z, i);
      }
    }

    if(storeKernelDistances)
    {
      auto kernelDistancesNeighborsPath = kernelDistancesDataPath.createChildPath("Neighbor List");
      auto kernelDistances = data.getDataAs<Float32NeighborList>(kernelDistancesNeighborsPath);
      mapKernelDistances(kernelDistances, kernelValDistances, kernelNumVoxels, dims.data(), x, y, z);
    }

    if(i > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(i) / numVerts) * 100.0f);
      std::string ss = fmt::format("Interpolating Point Cloud || {}% Completed", progressInt);
      // notifyStatusMessage(ss);
      prog = prog + progIncrement;
    }
  }

  return {};
}
} // namespace complex
