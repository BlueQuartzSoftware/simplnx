#include "ComputeEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @brief The ComputeDistanceMapImpl class implements a threaded algorithm that computes the distance map
 * for each point in the supplied volume.
 *
 * Accepts pre-buffered local arrays instead of DataStore references to avoid
 * per-element virtual dispatch overhead (important for OOC stores).
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

  virtual ~ComputeDistanceMapImpl() = default;

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
    // This loop initializes the `voxel_NearestNeighbor` and `voxel_Distance` temp arrays with values
    for(usize voxelTupleIdx = 0; voxelTupleIdx < m_TotalVoxels; ++voxelTupleIdx)
    {
      // For the given `mapType`, get the value that was stored for the nearestNeighbor,
      // essentially, as long as the value is **NOT** -1.
      if(m_NearestNeighbors[voxelTupleIdx * 3 + static_cast<usize>(MapType)] >= 0)
      {
        // if voxel is boundary voxel, then use itself as the nearest boundary voxel
        voxel_NearestNeighbor[voxelTupleIdx] = static_cast<int64>(voxelTupleIdx);
      }
      else
      {
        // If a default value was stored into the NearestNeighbor then set that into the voxel_NearestNeighbor vector at the current voxel index
        voxel_NearestNeighbor[voxelTupleIdx] = -1;
      }
      voxel_Distance[voxelTupleIdx] = static_cast<float64>(m_DistBuf[voxelTupleIdx]);
    }

    // ------------- Calculate the Manhattan Distance ----------------
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
            // If the nearestNeighbor value == -1 (invalid value?) and the featureId
            // of the current voxel is valid. Does this mean we are on a border
            // voxel like the border between the overscan and sample of an EBSD data set?
            if(voxel_NearestNeighbor[i] == -1 && m_FeatureIds[i] > 0)
            {
              count++; // increment the count?
              // Loop over all neighbors (6 face neighbors)
              for(int32 j = 0; j < 6; j++)
              {
                neighpoint = i + neighbors[j];
                if(mask[j] == 1)
                {
                  // if the mask for this voxel is true and the voxel_distance != -1, i.e.,
                  // meaning that we are on a boundary voxel of some type, then set
                  // the voxel_nearestNeighbor of the current voxel to that of
                  // its neighbor? This value could get overwritten by the next
                  // neighbor value? I wonder if this has any ramifications.?
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

      // Now run back over all voxels to increment "changed" and voxel_Distance.
      for(usize voxelIdx = 0; voxelIdx < m_TotalVoxels; ++voxelIdx)
      {
        if(voxel_NearestNeighbor[voxelIdx] != -1 && voxel_Distance[voxelIdx] == -1.0 && m_FeatureIds[voxelIdx] > 0)
        {
          changed++;
          voxel_Distance[voxelIdx] = Distance;
        }
      }
    }

    // ------------- Calculate the Euclidian Distance ----------------
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

    // Write results back to the nearestNeighbors vector and output distance buffer
    for(usize a = 0; a < m_TotalVoxels; ++a)
    {
      m_NearestNeighbors[a * 3 + static_cast<usize>(MapType)] = voxel_NearestNeighbor[a];
      m_DistBuf[a] = static_cast<T>(voxel_Distance[a]);
    }

    // Bulk-write the distance buffer back to the output DataStore
    m_OutputStore->copyFromBuffer(0, nonstd::span<const T>(m_DistBuf, m_TotalVoxels));
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeEuclideanDistMap::ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeEuclideanDistMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeEuclideanDistMap::~ComputeEuclideanDistMap() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Core distance map computation, templated on the output type (int32 for
 * Manhattan distance, float32 for Euclidean distance).
 *
 * OOC optimization strategy:
 *   The original implementation accessed FeatureIds and distance DataStores through
 *   per-element virtual dispatch in three passes: boundary identification, iterative
 *   Manhattan propagation, and Euclidean distance correction. Each pass iterated
 *   over all voxels, causing O(totalVoxels * passes) chunk operations for OOC data.
 *
 *   The optimized implementation front-loads all DataStore I/O:
 *   1. Bulk-read the entire FeatureIds array into featureIdsBuf.
 *   2. Fill distance stores with -1, then bulk-read into local buffers (gbDistBuf, etc.).
 *   3. Boundary identification runs entirely on local buffers.
 *   4. Each ComputeDistanceMapImpl worker receives raw pointers (not DataStore refs),
 *      so propagation and distance correction use plain memory access.
 *   5. Workers write results back via a single copyFromBuffer() at the end.
 *
 *   This reduces OOC round-trips from O(totalVoxels * passes) to O(1) per map.
 */
// -----------------------------------------------------------------------------
template <typename T>
void FindDistanceMap(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  const auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(inputValues->FeatureIdsArrayPath).getDataStoreRef();
  usize totalVoxels = featureIdsStoreRef.getNumberOfTuples();

  // Bulk-read the entire FeatureIds array into a local buffer. This is a
  // full-volume read but is done once, vs. the original per-element access
  // that triggered a chunk operation per voxel per pass.
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

  // Bulk-read distance stores into local buffers after the fill(-1) call
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

  // Create a temporary nearest neighbors vector (3 components per voxel)
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

  // This entire loop finds all 3 kinds of grain boundaries,
  // Feature Boundaries, Triple Junctions, QuadPoints
  for(int64 voxelIndex = 0; voxelIndex < static_cast<int64>(totalVoxels); ++voxelIndex)
  {
    if(shouldCancel)
    {
      return;
    }
    feature = featureIdsBuf[voxelIndex];
    if(feature > 0) // Ignore FeatureId = 0
    {
      int64 xIdx = voxelIndex % dims[0];
      int64 yIdx = (voxelIndex / dims[0]) % dims[1];
      int64 zIdx = voxelIndex / (dims[0] * dims[1]);

      // Loop over the 6 face neighbors of the voxel
      std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }

        neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

        // If we are a proper neighbor voxel, i.e., have not stepped out of the virtual volume,
        // and the featureId of the neighbor is NOT the currentFeatureId AND the
        // neighborFeatureId is valid (greater than 0), then drop into this conditional
        if(featureIdsBuf[neighborPoint] != feature && featureIdsBuf[neighborPoint] >= 0)
        {
          add = true; // Default to always adding this neighbor to the coordination vector
          // Loop over the current vector of coordination values
          for(const auto& coordination_value : coordination)
          {
            // If the featureId of the neighbor voxel == the current coordination_value
            // then we set the boolean to ignore this neighbor by setting `add = false`
            if(featureIdsBuf[neighborPoint] == coordination_value)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            coordination.push_back(featureIdsBuf[neighborPoint]); // Push back the first neighbor found
          }
        }
      }

      // now that the neighbors are found and the coordination size is found
      // If no values were pushed into the coordination vector, then just initialize
      // all 3 components of the nearestNeighbors to -1
      if(coordination.empty())
      {
        nearestNeighbors[voxelIndex * 3 + 0] = -1;
        nearestNeighbors[voxelIndex * 3 + 1] = -1;
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }
      // If ANY values were pushed back into the coordination vector, then this voxel
      // is a grain boundary. Initialize the first component of the nearestNeighbor to the
      // first value of the coordination vector.
      // Initialize the GB output array to 0
      if(!coordination.empty() && inputValues->DoBoundaries)
      {
        gbDistBuf[voxelIndex] = 0;
        nearestNeighbors[voxelIndex * 3 + 0] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 1] = -1;
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }

      // Triple lines are defined as a line that separates 3, and only 3, grains.
      // Initialize the nearestNeighbor components 0 and 1 to the first value in the coordination vector
      // Initializes the TJ output array to 0;
      if(coordination.size() >= 2 && inputValues->DoTripleLines)
      {
        tjDistBuf[voxelIndex] = 0;
        nearestNeighbors[voxelIndex * 3 + 0] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 1] = coordination[0];
        nearestNeighbors[voxelIndex * 3 + 2] = -1;
      }

      // All other boundaries between 4 or more grains are Quadruple Points.
      // Initialize the nearestNeighbor components 0, 1, 2 to the first value in the coordination vector
      // Initializes the QP output array to 0.
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

  // Now that we have all the necessary values, use TBB to initiate a task to compute
  // the output for each kind of selected output.
  // Each task gets its own distance buffer and the shared (read-only) featureIds buffer.
  // The template parameter T already encodes the distance type (int32 for Manhattan, float32 for Euclidean).
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
  // Wait for tasks to complete
  taskRunner.wait();
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeEuclideanDistMap::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeEuclideanDistMap::operator()()
{
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
