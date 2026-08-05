#include "InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
struct VoxelAccumulator
{
  float64 weightedSum = 0.0;
  float64 weightSum = 0.0;
  uint64 count = 0;
  float64 min = std::numeric_limits<float64>::max();
  float64 max = std::numeric_limits<float64>::lowest();
  float64 welfordMean = 0.0;
  float64 welfordM2 = 0.0;
};

struct ExtractAsFloat64Functor
{
  template <typename T>
  std::vector<float64> operator()(IDataArray* sourceArray)
  {
    auto& store = sourceArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numTuples = store.getNumberOfTuples();
    std::vector<float64> result(numTuples);
    for(usize i = 0; i < numTuples; i++)
    {
      result[i] = static_cast<float64>(store[i]);
    }
    return result;
  }
};

struct WriteWeightedAverageFunctor
{
  template <typename T>
  void operator()(IDataArray* outputArray, const std::vector<float64>& weightedSums, const std::vector<float64>& weightSums, usize numVoxels)
  {
    auto& store = outputArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    for(usize i = 0; i < numVoxels; i++)
    {
      if(weightSums[i] > 0.0)
      {
        store[i] = static_cast<T>(weightedSums[i] / weightSums[i]);
      }
    }
  }
};

void computeKernel(uint64 interpolationTechnique, const std::vector<float32>& sigmas, std::vector<float32>& kernel, const int64 kernelNumVoxels[3])
{
  const auto kDimX = static_cast<usize>(2 * kernelNumVoxels[0] + 1);
  const auto kDimY = static_cast<usize>(2 * kernelNumVoxels[1] + 1);

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        const auto kx = static_cast<usize>(x + kernelNumVoxels[0]);
        const auto ky = static_cast<usize>(y + kernelNumVoxels[1]);
        const auto kz = static_cast<usize>(z + kernelNumVoxels[2]);
        const usize idx = kz * kDimY * kDimX + ky * kDimX + kx;

        if(interpolationTechnique == InterpolatePointCloudToRegularGrid::k_Uniform)
        {
          kernel[idx] = 1.0f;
        }
        else if(interpolationTechnique == InterpolatePointCloudToRegularGrid::k_Gaussian)
        {
          kernel[idx] = std::exp(-((static_cast<float32>(x * x) / (2.0f * sigmas[0] * sigmas[0])) + (static_cast<float32>(y * y) / (2.0f * sigmas[1] * sigmas[1])) +
                                   (static_cast<float32>(z * z) / (2.0f * sigmas[2] * sigmas[2]))));
        }
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
  const auto* vertices = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->vertexGeomPath);
  const auto* image = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->imageGeomPath);
  const DataPath interpolatedGroupPath = image->getCellDataPath();
  const SizeVec3 dims = image->getDimensions();
  const FloatVec3 res = image->getSpacing();

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize numVoxels = dimX * dimY * dimZ;

  const usize numVerts = vertices->getNumberOfVertices();

  // Kernel dimensions in voxels
  int64 kernelNumVoxels[3] = {0, 0, 0};
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

  const auto kDimX = static_cast<usize>(2 * kernelNumVoxels[0] + 1);
  const auto kDimY = static_cast<usize>(2 * kernelNumVoxels[1] + 1);
  const auto kDimZ = static_cast<usize>(2 * kernelNumVoxels[2] + 1);
  const usize totalKernel = kDimX * kDimY * kDimZ;

  // Compute kernel weights
  std::vector<float32> kernel(totalKernel, 0.0f);
  computeKernel(m_InputValues->interpolationTechnique, m_InputValues->sigmas, kernel, kernelNumVoxels);

  // Mask
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
  if(m_InputValues->useMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->maskDataPath);
    } catch(const std::exception& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->maskDataPath.toString());
      return MakeErrorResult(-34060, message);
    }
  }

  const bool needWelford = m_InputValues->findStdDeviation;

  // Pre-extract interpolated source array data as float64
  std::vector<std::vector<float64>> interpSourceData;
  std::vector<IDataArray*> interpSourceArrays;
  for(const auto& path : m_InputValues->interpolatedDataPaths)
  {
    auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(path);
    if(sourceArray->getDataType() == DataType::boolean)
    {
      continue;
    }
    interpSourceArrays.push_back(sourceArray);
    interpSourceData.push_back(ExecuteDataFunction(ExtractAsFloat64Functor{}, sourceArray->getDataType(), sourceArray));
  }

  // Pre-extract copy source array data as float64
  std::vector<std::vector<float64>> copySourceData;
  std::vector<IDataArray*> copySourceArrays;
  for(const auto& path : m_InputValues->copyDataPaths)
  {
    auto* sourceArray = m_DataStructure.getDataAs<IDataArray>(path);
    if(sourceArray->getDataType() == DataType::boolean)
    {
      continue;
    }
    copySourceArrays.push_back(sourceArray);
    copySourceData.push_back(ExecuteDataFunction(ExtractAsFloat64Functor{}, sourceArray->getDataType(), sourceArray));
  }

  // Allocate accumulators for interpolated arrays
  const usize numInterpArrays = interpSourceArrays.size();
  std::vector<std::vector<VoxelAccumulator>> interpAccum(numInterpArrays, std::vector<VoxelAccumulator>(numVoxels));

  // Allocate simple accumulators for copy arrays (weighted sum + weight sum)
  const usize numCopyArrays = copySourceArrays.size();
  std::vector<std::vector<float64>> copyWeightedSum(numCopyArrays, std::vector<float64>(numVoxels, 0.0));
  std::vector<std::vector<float64>> copyWeightSum(numCopyArrays, std::vector<float64>(numVoxels, 0.0));

  // Main vertex loop
  const usize progIncrement = numVerts / 100;
  usize prog = 1;

  for(usize i = 0; i < numVerts; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(m_InputValues->useMask && !maskCompare->isTrue(i)) // lazy-evaluation ensures the pointer is never accessed nullptr
    {
      continue;
    }

    const Point3D<float32> coords = vertices->getVertexCoordinate(i);
    const std::optional<usize> optIndex = image->getIndex(coords[0], coords[1], coords[2]);
    if(!optIndex.has_value())
    {
      continue;
    }
    const usize index = optIndex.value();

    const usize curX = index % dimX;
    const usize curY = (index / dimX) % dimY;
    const usize curZ = index / (dimX * dimY);

    // Compute clipped kernel bounds in grid space (symmetric in all 3 dimensions)
    const int64 startX = std::max(static_cast<int64>(0), static_cast<int64>(curX) - kernelNumVoxels[0]);
    const int64 startY = std::max(static_cast<int64>(0), static_cast<int64>(curY) - kernelNumVoxels[1]);
    const int64 startZ = std::max(static_cast<int64>(0), static_cast<int64>(curZ) - kernelNumVoxels[2]);
    const int64 endX = std::min(static_cast<int64>(dimX) - 1, static_cast<int64>(curX) + kernelNumVoxels[0]);
    const int64 endY = std::min(static_cast<int64>(dimY) - 1, static_cast<int64>(curY) + kernelNumVoxels[1]);
    const int64 endZ = std::min(static_cast<int64>(dimZ) - 1, static_cast<int64>(curZ) + kernelNumVoxels[2]);

    // Traverse kernel
    for(int64 gz = startZ; gz <= endZ; gz++)
    {
      for(int64 gy = startY; gy <= endY; gy++)
      {
        for(int64 gx = startX; gx <= endX; gx++)
        {
          // Compute kernel index using 3D offset
          const auto kx = static_cast<usize>(gx - static_cast<int64>(curX) + kernelNumVoxels[0]);
          const auto ky = static_cast<usize>(gy - static_cast<int64>(curY) + kernelNumVoxels[1]);
          const auto kz = static_cast<usize>(gz - static_cast<int64>(curZ) + kernelNumVoxels[2]);
          const usize kernelIdx = kz * kDimY * kDimX + ky * kDimX + kx;

          const float32 weight = kernel[kernelIdx];
          const usize voxelIdx = static_cast<usize>(gz) * dimX * dimY + static_cast<usize>(gy) * dimX + static_cast<usize>(gx);

          // Update interpolated array accumulators (using actual kernel weight)
          if(weight != 0.0f)
          {
            const auto w = static_cast<float64>(weight);
            for(usize a = 0; a < numInterpArrays; a++)
            {
              const float64 sourceVal = interpSourceData[a][i];
              const float64 weightedVal = w * sourceVal;

              auto& accum = interpAccum[a][voxelIdx];
              accum.count++;
              accum.weightedSum += weightedVal;
              accum.weightSum += w;
              accum.min = std::min(accum.min, weightedVal);
              accum.max = std::max(accum.max, weightedVal);

              if(needWelford)
              {
                const float64 delta = weightedVal - accum.welfordMean;
                accum.welfordMean += delta / static_cast<float64>(accum.count);
                const float64 delta2 = weightedVal - accum.welfordMean;
                accum.welfordM2 += delta * delta2;
              }
            }
          }

          // Update copy array accumulators (always uniform weight = 1.0)
          for(usize a = 0; a < numCopyArrays; a++)
          {
            const float64 sourceVal = copySourceData[a][i];
            copyWeightedSum[a][voxelIdx] += sourceVal;
            copyWeightSum[a][voxelIdx] += 1.0;
          }
        }
      }
    }

    if(i > prog)
    {
      const auto progressInt = static_cast<usize>((static_cast<float64>(i) / static_cast<float64>(numVerts)) * 100.0);
      m_MessageHandler.sendInfoMessage(fmt::format("Interpolating Point Cloud || {}% Completed", progressInt));
      prog += progIncrement;
    }
  }

  // Finalization pass - write outputs
  m_MessageHandler.sendInfoMessage("Writing interpolated results...");

  for(usize a = 0; a < numInterpArrays; a++)
  {
    const std::string& arrayName = interpSourceArrays[a]->getName();

    // Write interpolated (weighted average) output
    auto& interpOutput = m_DataStructure.getDataRefAs<Float64Array>(interpolatedGroupPath.createChildPath(arrayName));
    for(usize v = 0; v < numVoxels; v++)
    {
      const auto& accum = interpAccum[a][v];
      if(accum.weightSum > 0.0)
      {
        interpOutput[v] = accum.weightedSum / accum.weightSum;
      }
    }

    // Write statistics arrays
    if(m_InputValues->findLength)
    {
      auto& lengthOutput = m_DataStructure.getDataRefAs<UInt64Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->lengthSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        lengthOutput[v] = interpAccum[a][v].count;
      }
    }
    if(m_InputValues->findMin)
    {
      auto& minOutput = m_DataStructure.getDataRefAs<Float32Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->minSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        const auto& accum = interpAccum[a][v];
        minOutput[v] = accum.count > 0 ? static_cast<float32>(accum.min) : 0.0f;
      }
    }
    if(m_InputValues->findMax)
    {
      auto& maxOutput = m_DataStructure.getDataRefAs<Float32Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->maxSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        const auto& accum = interpAccum[a][v];
        maxOutput[v] = accum.count > 0 ? static_cast<float32>(accum.max) : 0.0f;
      }
    }
    if(m_InputValues->findMean)
    {
      auto& meanOutput = m_DataStructure.getDataRefAs<Float32Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->meanSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        const auto& accum = interpAccum[a][v];
        meanOutput[v] = accum.count > 0 ? static_cast<float32>(accum.weightedSum / static_cast<float64>(accum.count)) : 0.0f;
      }
    }
    if(m_InputValues->findStdDeviation)
    {
      auto& stdDevOutput = m_DataStructure.getDataRefAs<Float32Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->stdDeviationSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        const auto& accum = interpAccum[a][v];
        stdDevOutput[v] = accum.count > 0 ? static_cast<float32>(std::sqrt(accum.welfordM2 / static_cast<float64>(accum.count))) : 0.0f;
      }
    }
    if(m_InputValues->findSummation)
    {
      auto& sumOutput = m_DataStructure.getDataRefAs<Float32Array>(interpolatedGroupPath.createChildPath(arrayName + m_InputValues->summationSuffix));
      for(usize v = 0; v < numVoxels; v++)
      {
        sumOutput[v] = static_cast<float32>(interpAccum[a][v].weightedSum);
      }
    }
  }

  // Write copy array outputs
  for(usize a = 0; a < numCopyArrays; a++)
  {
    auto* outputArray = m_DataStructure.getDataAs<IDataArray>(interpolatedGroupPath.createChildPath(copySourceArrays[a]->getName()));
    ExecuteDataFunction(WriteWeightedAverageFunctor{}, outputArray->getDataType(), outputArray, copyWeightedSum[a], copyWeightSum[a], numVoxels);
  }

  return {};
}
