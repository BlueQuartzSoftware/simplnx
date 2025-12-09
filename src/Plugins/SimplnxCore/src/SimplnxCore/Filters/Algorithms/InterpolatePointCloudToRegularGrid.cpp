#include "InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <tuple>

using namespace nx::core;

namespace
{
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
        if(interpolationTechnique == InterpolatePointCloudToRegularGrid::k_Uniform)
        {
          kernel[counter] = 1.0f;
        }
        else if(interpolationTechnique == InterpolatePointCloudToRegularGrid::k_Gaussian)
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

void mapKernelDistances(NeighborList<float32>* kernelDistances, const std::vector<float32>& kernelValDistances, const std::vector<float32>& kernel, const int64 kernelNumVoxels[3], const usize dims[3],
                        usize curX, usize curY, usize curZ)
{
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
        usize index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
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
const std::atomic_bool& InterpolatePointCloudToRegularGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGrid::operator()()
{
  const DataPath interpolatedGroupPath = m_InputValues->imageGeomPath.createChildPath(m_InputValues->interpolatedGroupName);

  const DataPath kernelDistPath = interpolatedGroupPath.createChildPath(m_InputValues->kernelDistanceArrayName);

  Float32NeighborList* kernelDistances = nullptr;
  if(m_InputValues->storeKernelDistances)
  {
    kernelDistances = m_DataStructure.getDataAs<Float32NeighborList>(kernelDistPath);
  }

  auto vertices = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->vertexGeomPath);
  auto image = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->imageGeomPath);
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
  if(m_InputValues->useMask)
  {
    mask = m_DataStructure.getDataAs<BoolArray>(m_InputValues->maskDataPath)->getDataStore();
  }

  auto& voxelIndices = m_DataStructure.getDataRefAs<UInt64Array>(m_InputValues->voxelIndicesPath);

  // Make sure the NeighborList's outermost vector is resized to the number of tuples and initialized to non-null values (empty vectors)
  // for(const auto& interpolatedDataPath : m_InputValues->interpolatedDataPaths)
  // {
  //   InitializeNeighborList(m_DataStructure, interpolatedGroupPath.createChildPath(interpolatedDataPath.getTargetName()));
  // }
  // for(const auto& copyDataPath : m_InputValues->copyDataPaths)
  // {
  //   InitializeNeighborList(m_DataStructure, interpolatedGroupPath.createChildPath(copyDataPath.getTargetName()));
  // }

  usize maxImageIndex = ((dims[2] - 1) * dims[0] * dims[1]) + ((dims[1] - 1) * dims[0]) + (dims[0] - 1);

  kernelNumVoxels[0] = static_cast<int64>(std::ceil((m_InputValues->kernelSize[0] / res[0]) * 0.5f));
  kernelNumVoxels[1] = static_cast<int64>(std::ceil((m_InputValues->kernelSize[1] / res[1]) * 0.5f));
  kernelNumVoxels[2] = static_cast<int64>(std::ceil((m_InputValues->kernelSize[2] / res[2]) * 0.5f));

  if(m_InputValues->kernelSize[0] < res[0])
  {
    kernelNumVoxels[0] = 0;
  }
  if(m_InputValues->kernelSize[1] < res[1])
  {
    kernelNumVoxels[1] = 0;
  }
  if(m_InputValues->kernelSize[2] < res[2])
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
  determineKernel(m_InputValues->interpolationTechnique, m_InputValues->sigmas, kernel, kernelNumVoxels);

  std::vector<float32> uniformKernel(totalKernel, 1.0f);

  std::vector<float32> kernelValDistances;
  if(m_InputValues->storeKernelDistances)
  {
    kernelValDistances.resize(totalKernel);
    std::fill(kernelValDistances.begin(), kernelValDistances.end(), 0.0f);
    determineKernelDistances(kernelValDistances, kernelNumVoxels, res);
  }

  usize progIncrement = numVerts / 100;
  usize prog = 1;
  usize progressInt = 0;

  // ***************************************************************************
  // Prebuild these values outside the loop since they do not change inside the loop
  using InterpolatedTupleType = std::tuple<DataPath, INeighborList*, IDataArray*>;
  std::vector<InterpolatedTupleType> interpolatedDataTypes;
  for(const auto& interpolatedDataPathItem : m_InputValues->interpolatedDataPaths)
  {
    const auto dynamicArrayPath = interpolatedGroupPath.createChildPath(interpolatedDataPathItem.getTargetName());
    auto* dynamicArrayToInterpolate = m_DataStructure.getDataAs<INeighborList>(dynamicArrayPath);
    auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(interpolatedDataPathItem);
    interpolatedDataTypes.emplace_back(dynamicArrayPath, dynamicArrayToInterpolate, sourceArray);
  }

  std::vector<InterpolatedTupleType> copiedDataTypes;
  for(const auto& copyDataPath : m_InputValues->copyDataPaths)
  {
    auto dynamicArrayPath = interpolatedGroupPath.createChildPath(copyDataPath.getTargetName());
    auto* dynamicArrayToCopy = m_DataStructure.getDataAs<INeighborList>(dynamicArrayPath);
    auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(copyDataPath);
    copiedDataTypes.emplace_back(dynamicArrayPath, dynamicArrayToCopy, sourceArray);
  }
  // ***************************************************************************

  for(usize i = 0; i < numVerts; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(m_InputValues->useMask && nullptr != mask && !mask->getValue(i))
    {
      continue;
    }
    index = voxelIndices[i];
    if(index > maxImageIndex)
    {
      return MakeErrorResult(-11004,
                             fmt::format("Index present in the selected Voxel Indices array that falls outside the selected Image Geometry for interpolation.\n Index = {}\n Max Image Index = {}\n",
                                         index, maxImageIndex));
    }
    x = index % dims[0];
    y = (index / dims[0]) % dims[1];
    z = index / (dims[0] * dims[1]);

    for(const auto& interpolatedDataPathItem : interpolatedDataTypes)
    {
      const auto dynamicArrayPath = std::get<0>(interpolatedDataPathItem);
      auto* dynamicArrayToInterpolate = std::get<1>(interpolatedDataPathItem);
      auto* sourceArray = std::get<2>(interpolatedDataPathItem);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToInterpolate, kernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    for(const auto& copyDataPath : copiedDataTypes)
    {
      auto dynamicArrayPath = std::get<0>(copyDataPath); // interpolatedGroupPath.createChildPath(copyDataPath.getTargetName());
      auto* dynamicArrayToCopy = std::get<1>(copyDataPath);
      auto* sourceArray = std::get<2>(copyDataPath);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToCopy, uniformKernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    if(m_InputValues->storeKernelDistances && nullptr != kernelDistances)
    {
      // InitializeNeighborList(m_DataStructure, kernelDistPath);
      mapKernelDistances(kernelDistances, kernelValDistances, kernel, kernelNumVoxels, dims.data(), x, y, z);
    }

    if(i > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(i) / numVerts) * 100.0f);
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Interpolating Point Cloud || {}% Completed", progressInt));
      prog = prog + progIncrement;
    }
  }

  return {};
}
