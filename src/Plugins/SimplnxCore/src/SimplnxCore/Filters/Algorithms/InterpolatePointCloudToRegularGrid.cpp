#include "InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr uint64 k_Uniform = 0;
constexpr uint64 k_Gaussian = 1;

struct MapPointCloudDataByKernelFunctor
{
  template <typename T>
  void operator()(IDataArray* source, INeighborList* dynamic, std::vector<float>& kernelVals, const int64 kernel[3], const usize dims[3], usize curX, usize curY, usize curZ, usize vertIdx)
  {
    auto& inputData = source->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto* interpolatedDataPtr = dynamic_cast<NeighborList<T>*>(dynamic);

    usize index = 0;
    int64 startKernel[3] = {0, 0, 0};
    int64 endKernel[3] = {0, 0, 0};
    usize counter = 0;

    kernel[0] > static_cast<int64>(curX) ? startKernel[0] = 0 : startKernel[0] = static_cast<int64>(curX) - kernel[0];
    kernel[1] > static_cast<int64>(curY) ? startKernel[1] = 0 : startKernel[1] = static_cast<int64>(curY) - kernel[1];
    kernel[2] > static_cast<int64>(curZ) ? startKernel[2] = 0 : startKernel[2] = static_cast<int64>(curZ) - kernel[2];

    static_cast<int64>(curX) + kernel[0] >= static_cast<int64>(dims[0]) ? endKernel[0] = static_cast<int64>(dims[0]) - 1 : endKernel[0] = static_cast<int64>(curX) + kernel[0];
    static_cast<int64>(curY) + kernel[1] >= static_cast<int64>(dims[1]) ? endKernel[1] = static_cast<int64>(dims[1]) - 1 : endKernel[1] = static_cast<int64>(curY) + kernel[1];
    endKernel[2] = static_cast<int64>(curZ);

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
          interpolatedDataPtr->addEntry(index, kernelVals[counter] * inputData.at(vertIdx));
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
        if(interpolationTechnique == k_Uniform)
        {
          kernel[counter] = 1.0f;
        }
        else if(interpolationTechnique == k_Gaussian)
        {
          kernel[counter] = std::exp(-((x * x) / (2 * sigmas[0] * sigmas[0]) + (y * y) / (2 * sigmas[1] * sigmas[1]) + (z * z) / (2 * sigmas[2] * sigmas[2])));
        }
        counter++;
      }
    }
  }
}

void determineKernelDistances(std::vector<float32>& kernelValDistances, const int64 kernelNumVoxels[3], FloatVec3 res)
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        kernelValDistances[counter] = (x * x * res[0] * res[0]) + (y * y * res[1] * res[1]) + (z * z * res[2] * res[2]);
        kernelValDistances[counter] = std::sqrt(kernelValDistances[counter]);
        counter++;
      }
    }
  }
}

void mapKernelDistances(NeighborList<float32>* kernelDistances, std::vector<float32>& kernelValDistances, std::vector<float32>& kernel, const int64 kernelNumVoxels[3], const usize dims[3], usize curX,
                        usize curY, usize curZ)
{
  usize index;
  int64 startKernel[3] = {0, 0, 0};
  int64 endKernel[3] = {0, 0, 0};
  usize counter = 0;

  kernelNumVoxels[0] > static_cast<int64>(curX) ? startKernel[0] = 0 : startKernel[0] = static_cast<int64>(curX) - kernelNumVoxels[0];
  kernelNumVoxels[1] > static_cast<int64>(curY) ? startKernel[1] = 0 : startKernel[1] = static_cast<int64>(curY) - kernelNumVoxels[1];
  kernelNumVoxels[2] > static_cast<int64>(curZ) ? startKernel[2] = 0 : startKernel[2] = static_cast<int64>(curZ) - kernelNumVoxels[2];

  static_cast<int64>(curX) + kernelNumVoxels[0] >= static_cast<int64>(dims[0]) ? endKernel[0] = static_cast<int64>(dims[0]) - 1 : endKernel[0] = static_cast<int64>(curX) + kernelNumVoxels[0];
  static_cast<int64>(curY) + kernelNumVoxels[1] >= static_cast<int64>(dims[1]) ? endKernel[1] = static_cast<int64>(dims[1]) - 1 : endKernel[1] = static_cast<int64>(curY) + kernelNumVoxels[1];
  endKernel[2] = static_cast<int64>(curZ);

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

// -----------------------------------------------------------------------------
InterpolatePointCloudToRegularGrid::InterpolatePointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       InterpolatePointCloudToRegularGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InterpolatePointCloudToRegularGrid::~InterpolatePointCloudToRegularGrid() noexcept = default;

// -----------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGrid::operator()()
{
  auto useMask = m_InputValues->UseMask;
  auto storeKernelDistances = m_InputValues->StoreKernelDistances;
  auto interpolationTechnique = m_InputValues->InterpolationIndex;
  auto vertexGeomPath = m_InputValues->InputVertexGeometryPath;
  auto imageGeomPath = m_InputValues->InputImageGeometryPath;
  auto interpolatedGroupName = m_InputValues->InterpolatedGroupName;
  auto interpolatedDataPaths = m_InputValues->InterpolateArrays;
  auto copyDataPaths = m_InputValues->CopyArrays;
  auto voxelIndicesPath = m_InputValues->VoxelIndicesPath;
  auto kernelSize = m_InputValues->KernelSize;

  const DataPath interpolatedGroupPath = imageGeomPath.createChildPath(interpolatedGroupName);
  const auto sigmas = m_InputValues->GaussianSigmas;

  auto vertices = m_DataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = m_DataStructure.getDataAs<ImageGeom>(imageGeomPath);
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  int64 kernelNumVoxels[3] = {0, 0, 0};

  auto numVerts = vertices->getNumberOfVertices();
  usize index = 0;
  usize x = 0;
  usize y = 0;
  usize z = 0;

  std::vector<float32> kernel;

  BoolArray::store_type* mask = nullptr;
  if(useMask)
  {
    mask = m_DataStructure.getDataAs<BoolArray>(m_InputValues->InputMaskPath)->getDataStore();
  }

  auto& voxelIndices = m_DataStructure.getDataRefAs<UInt64Array>(voxelIndicesPath);

  // Make sure the NeighborList's outermost vector is resized to the number of tuples and initialized to non-null values (empty vectors)
  for(const auto& interpolatedDataPath : interpolatedDataPaths)
  {
    InitializeNeighborList(m_DataStructure, interpolatedGroupPath.createChildPath(interpolatedDataPath.getTargetName()));
  }
  for(const auto& copyDataPath : copyDataPaths)
  {
    InitializeNeighborList(m_DataStructure, interpolatedGroupPath.createChildPath(copyDataPath.getTargetName()));
  }

  usize maxImageIndex = ((dims[2] - 1) * dims[0] * dims[1]) + ((dims[1] - 1) * dims[0]) + (dims[0] - 1);

  kernelNumVoxels[0] = static_cast<int64>(std::ceil((kernelSize[0] / res[0]) * 0.5f));
  kernelNumVoxels[1] = static_cast<int64>(std::ceil((kernelSize[1] / res[1]) * 0.5f));
  kernelNumVoxels[2] = static_cast<int64>(std::ceil((kernelSize[2] / res[2]) * 0.5f));

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
  for(usize i = 0; i < 3; i++)
  {
    tmpKernelSize[i] *= (kernelNumVoxels[i] * 2) + 1;
  }

  int64 totalKernel = tmpKernelSize[0] * tmpKernelSize[1] * tmpKernelSize[2];

  kernel.resize(totalKernel);
  std::fill(kernel.begin(), kernel.end(), 0.0f);
  determineKernel(interpolationTechnique, sigmas, kernel, kernelNumVoxels);

  std::vector<float32> uniformKernel(totalKernel, 1.0f);

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
    index = voxelIndices[i];
    if(index > maxImageIndex)
    {
      return MakeErrorResult(-11004,
                             fmt::format("Index present in the selected Voxel Indices array that falls outside the selected Image Geometry for interpolation.\n Index = %1\n Max Image Index = %2\n",
                                         index, maxImageIndex));
    }
    x = index % dims[0];
    y = (index / dims[0]) % dims[1];
    z = index / (dims[0] * dims[1]);

    for(const auto& interpolatedDataPathItem : interpolatedDataPaths)
    {
      const auto dynamicArrayPath = interpolatedGroupPath.createChildPath(interpolatedDataPathItem.getTargetName());
      auto* dynamicArrayToInterpolate = m_DataStructure.getDataAs<INeighborList>(dynamicArrayPath);
      auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(interpolatedDataPathItem);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToInterpolate, kernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    for(const auto& copyDataPath : copyDataPaths)
    {
      auto dynamicArrayPath = interpolatedGroupPath.createChildPath(copyDataPath.getTargetName());
      auto* dynamicArrayToCopy = m_DataStructure.getDataAs<INeighborList>(dynamicArrayPath);
      auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(copyDataPath);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToCopy, uniformKernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    if(storeKernelDistances)
    {
      const DataPath kernelDistPath = interpolatedGroupPath.createChildPath(m_InputValues->KernelDistancesArrayName);
      InitializeNeighborList(m_DataStructure, kernelDistPath);
      auto* kernelDistances = m_DataStructure.getDataAs<Float32NeighborList>(kernelDistPath);
      mapKernelDistances(kernelDistances, kernelValDistances, kernel, kernelNumVoxels, dims.data(), x, y, z);
    }

    if(i > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(i) / numVerts) * 100.0f);
      m_MessageHandler(fmt::format("Interpolating Point Cloud || {}% Completed", progressInt));
      prog = prog + progIncrement;
    }
  }

  return {};
}
