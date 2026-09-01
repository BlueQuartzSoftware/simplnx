#include "ComputeEuclideanDistMap.hpp"

#include "ComputeEuclideanDistMapScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <optional>

using namespace nx::core;

namespace
{
/**
 * @brief Tests for non-positive Feature IDs with bounded reads.
 * @param featureIds Supplies cell Feature IDs.
 * @param bufferSize Requests the scan buffer size.
 * @param shouldCancel Signals cancellation between scan blocks.
 * @return True if a blocked cell exists. Returns false if none exists. Returns
 * nullopt after cancellation.
 *
 * Concrete DataStore input uses a contiguous view. Other stores use bulk reads.
 * Current bulk-I/O Result values are not inspected.
 */
std::optional<bool> ContainsBlockedCells(const Int32AbstractDataStore& featureIds, usize bufferSize, const std::atomic_bool& shouldCancel)
{
  const usize scanBlockSize = std::max<usize>(bufferSize, 1);
  if(const auto* contiguousStore = dynamic_cast<const DataStore<int32>*>(&featureIds); contiguousStore != nullptr)
  {
    const nonstd::span<const int32> featureIdSpan = contiguousStore->createSpan();
    for(usize offset = 0; offset < featureIdSpan.size(); offset += scanBlockSize)
    {
      if(shouldCancel)
      {
        return std::nullopt;
      }

      const usize count = std::min(scanBlockSize, featureIdSpan.size() - offset);
      if(std::any_of(featureIdSpan.begin() + offset, featureIdSpan.begin() + offset + count, [](int32 featureId) { return featureId <= 0; }))
      {
        return true;
      }
    }
    return false;
  }

  std::vector<int32> buffer(scanBlockSize);
  for(usize offset = 0; offset < featureIds.getSize(); offset += buffer.size())
  {
    if(shouldCancel)
    {
      return std::nullopt;
    }

    const usize count = std::min(buffer.size(), featureIds.getSize() - offset);
    featureIds.copyIntoBuffer(offset, nonstd::span<int32>(buffer.data(), count));
    if(std::any_of(buffer.begin(), buffer.begin() + count, [](int32 featureId) { return featureId <= 0; }))
    {
      return true;
    }
  }
  return false;
}

/**
 * @class ComputeDistanceMapImpl
 * @brief Computes one direct distance map from resident buffers.
 * @tparam T Specifies the output distance type.
 * @tparam MapType Selects the seed map category.
 *
 * The worker uses local source and output buffers during propagation. Tasks write distinct map
 * components in nearest-neighbor storage. This specialization does not establish generic DataArray
 * or DataStore thread safety. The final bulk write does not inspect its Result value.
 */
template <typename T, ComputeEuclideanDistMap::MapType MapType = ComputeEuclideanDistMap::MapType::FeatureBoundary>
class ComputeDistanceMapImpl
{
  const ComputeEuclideanDistMapInputValues& m_InputValues;
  std::vector<int64>& m_NearestNeighbors;
  const int32* m_FeatureIds = nullptr;
  T* m_DistBuf = nullptr;
  AbstractDataStore<T>* m_OutputStore = nullptr;
  usize m_TotalVoxels = 0;
  SizeVec3 m_Dims = {};
  FloatVec3 m_Spacing = {};

public:
  /**
   * @brief Initializes a direct distance-map worker.
   * @param inputValues Preserves the direct worker constructor signature.
   * @param nearestNeighbors Stores nearest seeds for all map categories.
   * @param featureIds Supplies resident Feature IDs.
   * @param distBuf Supplies and receives resident output distances.
   * @param outputStore Receives the final bulk distance write.
   * @param totalVoxels Identifies the number of image cells.
   * @param dims Supplies image dimensions.
   * @param spacing Supplies image spacing.
   * @pre All pointers reference storage for totalVoxels values.
   * @pre All arguments outlive the worker execution.
   */
  ComputeDistanceMapImpl(const ComputeEuclideanDistMapInputValues& inputValues, std::vector<int64>& nearestNeighbors, const int32* featureIds, T* distBuf, AbstractDataStore<T>* outputStore,
                         usize totalVoxels, SizeVec3 dims, FloatVec3 spacing)
  : m_InputValues(inputValues)
  , m_NearestNeighbors(nearestNeighbors)
  , m_FeatureIds(featureIds)
  , m_DistBuf(distBuf)
  , m_OutputStore(outputStore)
  , m_TotalVoxels(totalVoxels)
  , m_Dims(dims)
  , m_Spacing(spacing)
  {
  }

  /**
   * @brief Destroys the direct distance-map worker.
   */
  virtual ~ComputeDistanceMapImpl() = default;

  /**
   * @brief Propagates one map and writes its final distances.
   *
   * This worker does not inspect cancellation. It runs to completion after its
   * task starts.
   */
  void operator()() const
  {
    auto xpoints = static_cast<int64_t>(m_Dims[0]);
    auto ypoints = static_cast<int64_t>(m_Dims[1]);
    auto zpoints = static_cast<int64_t>(m_Dims[2]);

    float64 Distance = 0.0;
    usize count = 1;
    usize changed = 1;
    usize neighpoint = 0;
    int64_t neighbors[6] = {0, 0, 0, 0, 0, 0};

    neighbors[0] = -xpoints * ypoints;
    neighbors[1] = -xpoints;
    neighbors[2] = -1;
    neighbors[3] = 1;
    neighbors[4] = xpoints;
    neighbors[5] = xpoints * ypoints;

    std::vector<int64> voxel_NearestNeighbor(m_TotalVoxels, 0);
    std::vector<float64> voxel_Distance(m_TotalVoxels, 0.0);

    Distance = 0;
    // Initialize each voxel with its nearest seed or blocked state.
    for(usize voxelTupleIdx = 0; voxelTupleIdx < m_TotalVoxels; ++voxelTupleIdx)
    {
      if(m_NearestNeighbors[voxelTupleIdx * 3 + static_cast<usize>(MapType)] >= 0)
      {
        voxel_NearestNeighbor[voxelTupleIdx] = static_cast<int64>(voxelTupleIdx);
      }
      else
      {
        voxel_NearestNeighbor[voxelTupleIdx] = -1;
      }
      voxel_Distance[voxelTupleIdx] = static_cast<float64>(m_DistBuf[voxelTupleIdx]);
    }

    // Propagate city-block distances until no voxel changes.
    count = 1;
    changed = 1;
    int64_t i = 0;
    int64_t zBlock = xpoints * ypoints;
    int64_t zStride = 0, yStride = 0;
    char mask[6] = {0, 0, 0, 0, 0, 0};
    while(count > 0 && changed > 0)
    {
      count = 0;
      changed = 0;
      Distance++;

      for(int64_t z = 0; z < zpoints; ++z)
      {
        zStride = z * zBlock;
        mask[0] = mask[5] = 1;
        if(z == 0)
        {
          mask[0] = 0;
        }
        if(z == zpoints - 1)
        {
          mask[5] = 0;
        }

        for(int64_t y = 0; y < ypoints; ++y)
        {
          yStride = y * xpoints;
          mask[1] = mask[4] = 1;
          if(y == 0)
          {
            mask[1] = 0;
          }
          if(y == ypoints - 1)
          {
            mask[4] = 0;
          }

          for(int64_t x = 0; x < xpoints; ++x)
          {
            mask[2] = mask[3] = 1;
            if(x == 0)
            {
              mask[2] = 0;
            }
            if(x == xpoints - 1)
            {
              mask[3] = 0;
            }

            i = zStride + yStride + x;
            if(voxel_NearestNeighbor[i] == -1 && m_FeatureIds[i] > 0)
            {
              count++;
              for(int32 j = 0; j < 6; j++)
              {
                neighpoint = i + neighbors[j];
                if(mask[j] == 1)
                {
                  if(voxel_Distance[neighpoint] != -1.0)
                  {
                    voxel_NearestNeighbor[i] = voxel_NearestNeighbor[neighpoint];
                  }
                }
              }
            }
          }
        }
      }

      for(usize voxelIdx = 0; voxelIdx < m_TotalVoxels; ++voxelIdx)
      {
        if(voxel_NearestNeighbor[voxelIdx] != -1 && voxel_Distance[voxelIdx] == -1.0 && m_FeatureIds[voxelIdx] > 0)
        {
          changed++;
          voxel_Distance[voxelIdx] = Distance;
        }
      }
    }

    // Float output converts nearest-seed positions to Euclidean distances.
    if constexpr(std::is_same_v<T, float32>)
    {
      float64 x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0, z1 = 0.0, z2 = 0.0;
      float64 dist = 0.0;
      float64 oneOverzBlock = 1.0 / static_cast<float64>(zBlock);
      float64 oneOverxpoints = 1.0 / static_cast<float64>(xpoints);
      for(int64_t m = 0; m < zpoints; m++)
      {
        zStride = m * zBlock;
        for(int64_t n = 0; n < ypoints; n++)
        {
          yStride = n * xpoints;
          for(int64_t p = 0; p < xpoints; p++)
          {
            x1 = static_cast<float64>(p) * m_Spacing[0];
            y1 = static_cast<float64>(n) * m_Spacing[1];
            z1 = static_cast<float64>(m) * m_Spacing[2];
            if(int64_t nearestNeighbor = voxel_NearestNeighbor[zStride + yStride + p]; nearestNeighbor >= 0)
            {
              x2 = m_Spacing[0] * static_cast<float64>(nearestNeighbor % xpoints);                                        // find_xcoord(nearestneighbor);
              y2 = m_Spacing[1] * static_cast<float64>(static_cast<int64_t>(nearestNeighbor * oneOverxpoints) % ypoints); // find_ycoord(nearestneighbor);
              z2 = m_Spacing[2] * floor(nearestNeighbor * oneOverzBlock);                                                 // find_zcoord(nearestneighbor);
              dist = ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) + ((z1 - z2) * (z1 - z2));
              dist = sqrt(dist);
              voxel_Distance[zStride + yStride + p] = dist;
            }
          }
        }
      }
    }

    // Publish local results after the worker completes propagation.
    for(usize a = 0; a < m_TotalVoxels; ++a)
    {
      m_NearestNeighbors[a * 3 + static_cast<usize>(MapType)] = voxel_NearestNeighbor[a];
      m_DistBuf[a] = static_cast<T>(voxel_Distance[a]);
    }

    m_OutputStore->copyFromBuffer(0, nonstd::span<const T>(m_DistBuf, m_TotalVoxels));
  }
};
} // namespace

ComputeEuclideanDistMap::ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeEuclideanDistMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeEuclideanDistMap::~ComputeEuclideanDistMap() noexcept = default;

/**
 * @brief Computes direct distance maps with full-volume resident buffers.
 * @tparam T Specifies int32 city-block or float32 Euclidean output.
 * @param dataStructure Contains the ImageGeom, Feature IDs, and output maps.
 * @param inputValues Selects map types and identifies required objects.
 * @param shouldCancel Signals cancellation during seed discovery.
 * @param messageHandler Preserves the common algorithm call signature.
 * @pre Requested output maps have the Feature ID tuple count.
 *
 * The function bulk-copies complete source and output arrays before worker tasks run. Workers then
 * use local buffers and publish each map with one final bulk write. The normal dispatcher reserves
 * this memory-heavy path for its measured resident exception.
 *
 * This function ignores bulk-I/O Result values. Cancellation during seed discovery returns without
 * starting worker tasks. Output maps are filled with -1 before seed discovery. Cancellation can
 * leave those initialized maps. Worker tasks do not inspect cancellation. This function does not use messageHandler.
 */
template <typename T>
void FindDistanceMap(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  const auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(inputValues->FeatureIdsArrayPath).getDataStoreRef();
  usize totalVoxels = featureIdsStoreRef.getNumberOfTuples();

  // Direct propagation needs contiguous resident Feature IDs.
  std::vector<int32> featureIdsBuf(totalVoxels);
  featureIdsStoreRef.copyIntoBuffer(0, nonstd::span<int32>(featureIdsBuf.data(), totalVoxels));

  DataStoreType* gbManhattanDistancesStore = nullptr;
  if(inputValues->DoBoundaries)
  {
    gbManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->GBDistancesArrayPath)->getDataStore();
    gbManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  DataStoreType* tjManhattanDistancesStore = nullptr;
  if(inputValues->DoTripleLines)
  {
    tjManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->TJDistancesArrayPath)->getDataStore();
    tjManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  DataStoreType* qpManhattanDistancesStore = nullptr;
  if(inputValues->DoQuadPoints)
  {
    qpManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->QPDistancesArrayPath)->getDataStore();
    qpManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  // Each selected map starts at -1 before its local propagation buffer is filled.
  std::vector<T> gbDistBuf;
  if(inputValues->DoBoundaries)
  {
    gbDistBuf.resize(totalVoxels);
    gbManhattanDistancesStore->copyIntoBuffer(0, nonstd::span<T>(gbDistBuf.data(), totalVoxels));
  }
  std::vector<T> tjDistBuf;
  if(inputValues->DoTripleLines)
  {
    tjDistBuf.resize(totalVoxels);
    tjManhattanDistancesStore->copyIntoBuffer(0, nonstd::span<T>(tjDistBuf.data(), totalVoxels));
  }
  std::vector<T> qpDistBuf;
  if(inputValues->DoQuadPoints)
  {
    qpDistBuf.resize(totalVoxels);
    qpManhattanDistancesStore->copyIntoBuffer(0, nonstd::span<T>(qpDistBuf.data(), totalVoxels));
  }

  // Each map category stores one nearest-seed index per voxel.
  std::vector<int64> nearestNeighbors(totalVoxels * 3, -1);

  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  bool add = true;
  int32 feature = 0;
  std::vector<int32> coordination;

  int64_t neighborPoint = 0;
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // The seed pass records distinct neighboring Feature IDs for each valid voxel.
  for(int64 voxelIndex = 0; voxelIndex < static_cast<int64>(totalVoxels); ++voxelIndex)
  {
    if(shouldCancel)
    {
      return;
    }
    feature = featureIdsBuf[voxelIndex];
    if(feature > 0)
    {
      int64 xIdx = voxelIndex % dims[0];
      int64 yIdx = (voxelIndex / dims[0]) % dims[1];
      int64 zIdx = voxelIndex / (dims[0] * dims[1]);

      std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }

        neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

        if(featureIdsBuf[neighborPoint] != feature && featureIdsBuf[neighborPoint] >= 0)
        {
          add = true;
          for(const auto& coordination_value : coordination)
          {
            if(featureIdsBuf[neighborPoint] == coordination_value)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            coordination.push_back(featureIdsBuf[neighborPoint]);
          }
        }
      }

      if(coordination.empty())
      {
        nearestNeighbors[voxelIndex * 3 + 0] = -1;
        nearestNeighbors[voxelIndex * 3 + 1] = -1;
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }
      if(!coordination.empty() && inputValues->DoBoundaries)
      {
        gbDistBuf[voxelIndex] = 0;
        nearestNeighbors[voxelIndex * 3 + 0] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 1] = -1;
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }

      if(coordination.size() >= 2 && inputValues->DoTripleLines)
      {
        tjDistBuf[voxelIndex] = 0;
        nearestNeighbors[voxelIndex * 3 + 0] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 1] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }

      if(coordination.size() > 2 && inputValues->DoQuadPoints)
      {
        qpDistBuf[voxelIndex] = 0;
        nearestNeighbors[voxelIndex * 3 + 0] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 1] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 2] = coordination[0];
      }
      coordination.resize(0);
    }
  }

  FloatVec3 spacing = selectedImageGeom.getSpacing();

  // Each task owns one output buffer and reads the shared Feature ID buffer.
  ParallelTaskAlgorithm taskRunner;
  if(inputValues->DoBoundaries)
  {
    taskRunner.execute(ComputeDistanceMapImpl<T, ComputeEuclideanDistMap::MapType::FeatureBoundary>(*inputValues, nearestNeighbors, featureIdsBuf.data(), gbDistBuf.data(), gbManhattanDistancesStore,
                                                                                                    totalVoxels, udims, spacing));
  }

  if(inputValues->DoTripleLines)
  {
    taskRunner.execute(ComputeDistanceMapImpl<T, ComputeEuclideanDistMap::MapType::TripleJunction>(*inputValues, nearestNeighbors, featureIdsBuf.data(), tjDistBuf.data(), tjManhattanDistancesStore,
                                                                                                   totalVoxels, udims, spacing));
  }

  if(inputValues->DoQuadPoints)
  {
    taskRunner.execute(ComputeDistanceMapImpl<T, ComputeEuclideanDistMap::MapType::QuadPoint>(*inputValues, nearestNeighbors, featureIdsBuf.data(), qpDistBuf.data(), qpManhattanDistancesStore,
                                                                                              totalVoxels, udims, spacing));
  }
  taskRunner.wait();
}

const std::atomic_bool& ComputeEuclideanDistMap::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeEuclideanDistMap::operator()()
{
  const auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto* boundaryDistances = m_InputValues->DoBoundaries ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->GBDistancesArrayPath) : nullptr;
  const auto* tripleJunctionDistances = m_InputValues->DoTripleLines ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->TJDistancesArrayPath) : nullptr;
  const auto* quadPointDistances = m_InputValues->DoQuadPoints ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->QPDistancesArrayPath) : nullptr;
  const bool forceDirectAlgorithm = ForceInCoreAlgorithm();
  const bool usesOutOfCoreStore = AnyOutOfCore({featureIdsArray, boundaryDistances, tripleJunctionDistances, quadPointDistances});
  const auto executeScanline = [this, usesOutOfCoreStore]() {
    RecordAlgorithmPathExecution(AlgorithmPath::OutOfCore, usesOutOfCoreStore);
    return ComputeEuclideanDistMapScanline(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
  };
  if(!forceDirectAlgorithm && (usesOutOfCoreStore || ForceOocAlgorithm()))
  {
    return executeScanline();
  }

  if(!forceDirectAlgorithm)
  {
    // The measured direct win needs all maps and at least one blocked cell.
    // Other workloads use scanline execution to avoid full-volume temporary buffers.
    const bool calculatesAllMaps = m_InputValues->DoBoundaries && m_InputValues->DoTripleLines && m_InputValues->DoQuadPoints;
    if(!calculatesAllMaps)
    {
      return executeScanline();
    }

    const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
    const SizeVec3 dims = imageGeom.getDimensions();
    const std::optional<bool> hasBlockedCells = ContainsBlockedCells(featureIdsArray->getDataStoreRef(), dims[0] * dims[1], m_ShouldCancel);
    if(!hasBlockedCells.has_value())
    {
      return {};
    }
    if(!hasBlockedCells.value())
    {
      return executeScanline();
    }
  }

  RecordAlgorithmPathExecution(AlgorithmPath::InCore, usesOutOfCoreStore);

  if(m_InputValues->CalcManhattanDist)
  {
    FindDistanceMap<int32>(m_DataStructure, m_InputValues, m_ShouldCancel, m_MessageHandler);
  }
  else
  {
    FindDistanceMap<float32>(m_DataStructure, m_InputValues, m_ShouldCancel, m_MessageHandler);
  }

  return {};
}
