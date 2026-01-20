#include "ComputeEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief The ComputeDistanceMapImpl class implements a threaded algorithm that computes the  distance map
 * for each point in the supplied volume
 */
template <typename T, ComputeEuclideanDistMap::MapType MapType = ComputeEuclideanDistMap::MapType::FeatureBoundary>
class ComputeDistanceMapImpl
{
  DataStructure& m_DataStructure;
  const ComputeEuclideanDistMapInputValues& m_InputValues;
  DataPath m_OutputArrayPath = {};

public:
  ComputeDistanceMapImpl(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues& inputValues, const DataPath& outputArrayPath)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_OutputArrayPath(outputArrayPath)
  {
  }

  virtual ~ComputeDistanceMapImpl() = default;

  void operator()() const
  {
    using DataArrayType = DataArray<T>;
    using DataStoreType = AbstractDataStore<T>;

    const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues.InputImageGeometry);

    DataStoreType& outputArray = m_DataStructure.getDataRefAs<DataArrayType>(m_OutputArrayPath).getDataStoreRef();

    SizeVec3 udims = selectedImageGeom.getDimensions();

    size_t totalVoxels = selectedImageGeom.getNumberOfCells();
    double Distance = 0.0;
    // size_t count = 1;
    // size_t changed = 1;
    size_t neighpoint = 0;
    int64_t neighbors[6] = {0, 0, 0, 0, 0, 0};
    auto xpoints = static_cast<int64_t>(udims[0]);
    auto ypoints = static_cast<int64_t>(udims[1]);
    auto zpoints = static_cast<int64_t>(udims[2]);
    FloatVec3 spacing = selectedImageGeom.getSpacing();

    neighbors[0] = -xpoints * ypoints;
    neighbors[1] = -xpoints;
    neighbors[2] = -1;
    neighbors[3] = 1;
    neighbors[4] = xpoints;
    neighbors[5] = xpoints * ypoints;

    std::vector<int32_t> voxel_NearestNeighbor(totalVoxels, 0);
    std::vector<double> voxel_Distance(totalVoxels, 0.0);

    // Input Arrays
    const auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues.FeatureIdsArrayPath)->getDataStoreRef();

    auto& outputStoreRef = m_DataStructure.getDataRefAs<DataArrayType>(m_OutputArrayPath).getDataStoreRef();

    // DataStoreType* gbManhattanDistancesStore = nullptr;
    // if(m_InputValues.DoBoundaries)
    // {
    //   gbManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.GBDistancesArrayName)->getDataStore();
    // }
    // DataStoreType* tjManhattanDistancesStore = nullptr;
    // if(m_InputValues.DoTripleLines)
    // {
    //   tjManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.TJDistancesArrayName)->getDataStore();
    // }
    // DataStoreType* qpManhattanDistancesStore = nullptr;
    // if(m_InputValues.DoQuadPoints)
    // {
    //   qpManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.QPDistancesArrayName)->getDataStore();
    // }

    auto& nearestNeighborsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues.NearestNeighborsArrayPath)->getDataStoreRef();

    Distance = 0;
    // This loop initializes the `voxel_NearestNeighbor` and `voxel_Distance` temp arrays with values
    for(size_t voxelTupleIdx = 0; voxelTupleIdx < totalVoxels; ++voxelTupleIdx)
    {

      // For the given `mapType`, get the value that was stored for the nearestNeighbor,
      // essentially, as long as the value is **NOT** -1.
      if(nearestNeighborsStoreRef.getComponentValue(voxelTupleIdx, static_cast<uint64_t>(MapType)) >= 0)
      {
        // if voxel is boundary voxel, then want to use itself as nearest boundary voxel
        voxel_NearestNeighbor[voxelTupleIdx] = static_cast<int32_t>(voxelTupleIdx);
      }
      else
      {
        // If a default value was stored into the NearestNeighbor then set that into the voxel_NearestNeighbor vector at the current voxel index
        voxel_NearestNeighbor[voxelTupleIdx] = -1;
      }
      // if constexpr(MapType == ComputeEuclideanDistMap::MapType::FeatureBoundary)
      {
        voxel_Distance[voxelTupleIdx] = static_cast<double>(outputStoreRef.getValue(voxelTupleIdx));
      }

      // if(m_InputValues.DoBoundaries && m_MapType == ComputeEuclideanDistMap::MapType::FeatureBoundary)
      // {
      //   voxel_Distance[voxelTupleIdx] = static_cast<double>((*gbManhattanDistancesStore)[voxelTupleIdx]);
      // }
      // else if(m_InputValues.DoTripleLines && m_MapType == ComputeEuclideanDistMap::MapType::TripleJunction)
      // {
      //   voxel_Distance[voxelTupleIdx] = static_cast<double>((*tjManhattanDistancesStore)[voxelTupleIdx]);
      // }
      // else if(m_InputValues.DoQuadPoints && m_MapType == ComputeEuclideanDistMap::MapType::QuadPoint)
      // {
      //   voxel_Distance[voxelTupleIdx] = static_cast<double>((*qpManhattanDistancesStore)[voxelTupleIdx]);
      // }
    }

    // ------------- Calculate the Manhattan Distance ----------------
    size_t count = 1;
    size_t changed = 1;
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
            if(voxel_NearestNeighbor[i] == -1 && featureIdsStore[i] > 0)
            {
              count++; // increment the count?
              // Loop over all neighbors (6 face neighbors)
              for(int32_t j = 0; j < 6; j++)
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
      for(size_t voxelIdx = 0; voxelIdx < totalVoxels; ++voxelIdx)
      {
        if(voxel_NearestNeighbor[voxelIdx] != -1 && voxel_Distance[voxelIdx] == -1.0 && featureIdsStore[voxelIdx] > 0)
        {
          changed++;
          voxel_Distance[voxelIdx] = Distance;
        }
      }
    }

    // ------------- Calculate the Euclidian Distance ----------------
    if constexpr(std::is_same_v<T, float32>)
    {
      double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0, z1 = 0.0, z2 = 0.0;
      double dist = 0.0;
      double oneOverzBlock = 1.0 / static_cast<double>(zBlock);
      double oneOverxpoints = 1.0 / static_cast<double>(xpoints);
      for(int64_t m = 0; m < zpoints; m++)
      {
        zStride = m * zBlock;
        for(int64_t n = 0; n < ypoints; n++)
        {
          yStride = n * xpoints;
          for(int64_t p = 0; p < xpoints; p++)
          {
            x1 = static_cast<double>(p) * spacing[0];
            y1 = static_cast<double>(n) * spacing[1];
            z1 = static_cast<double>(m) * spacing[2];
            if(int64_t nearestNeighbor = voxel_NearestNeighbor[zStride + yStride + p]; nearestNeighbor >= 0)
            {
              x2 = spacing[0] * static_cast<double>(nearestNeighbor % xpoints);                                        // find_xcoord(nearestneighbor);
              y2 = spacing[1] * static_cast<double>(static_cast<int64_t>(nearestNeighbor * oneOverxpoints) % ypoints); // find_ycoord(nearestneighbor);
              z2 = spacing[2] * floor(nearestNeighbor * oneOverzBlock);                                                // find_zcoord(nearestneighbor);
              dist = ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) + ((z1 - z2) * (z1 - z2));
              dist = sqrt(dist);
              voxel_Distance[zStride + yStride + p] = dist;
            }
          }
        }
      }
    }

    for(size_t a = 0; a < totalVoxels; ++a)
    {
      nearestNeighborsStoreRef[a * 3 + static_cast<uint32_t>(MapType)] = voxel_NearestNeighbor[a];

      outputStoreRef[a] = static_cast<T>(voxel_Distance[a]);

      // if(m_InputValues.DoBoundaries && m_MapType == ComputeEuclideanDistMap::MapType::FeatureBoundary)
      // {
      //   (*gbManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      // }
      // else if(m_InputValues.DoTripleLines && m_MapType == ComputeEuclideanDistMap::MapType::TripleJunction)
      // {
      //   (*tjManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      // }
      // else if(m_InputValues.DoQuadPoints && m_MapType == ComputeEuclideanDistMap::MapType::QuadPoint)
      // {
      //   (*qpManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      // }
    }
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
template <typename T>
void findDistanceMap(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues* inputValues)
{
  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  const auto& featureIdsStore = dataStructure.getDataRefAs<Int32Array>(inputValues->FeatureIdsArrayPath).getDataStoreRef();
  size_t totalVoxels = featureIdsStore.getNumberOfTuples();

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

  auto& nearestNeighbors = dataStructure.template getDataAs<Int32Array>(inputValues->NearestNeighborsArrayPath)->getDataStoreRef();

  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  int64_t column = 0, row = 0, plane = 0;
  bool good = false;
  std::vector<int32_t> coordination;

  int64_t neighbor = 0;
  int64_t neighbors[6] = {0, 0, 0, 0, 0, 0};
  neighbors[0] = -dims[0] * dims[1];
  neighbors[1] = -dims[0];
  neighbors[2] = -1;
  neighbors[3] = 1;
  neighbors[4] = dims[0];
  neighbors[5] = dims[0] * dims[1];

  size_t xPoints = udims[0];
  size_t yPoints = udims[1];
  size_t zPoints = udims[2];

  // This entire loop finds all 3 kinds of grain boundaries,
  // Feature Boundaries, Triple Junctions, QuadPoints
  for(size_t a = 0; a < totalVoxels; ++a)
  {
    int32_t feature = featureIdsStore[a];
    if(feature > 0) // Ignore FeatureId = 0
    {
      column = static_cast<int64_t>(a % xPoints);
      row = static_cast<int64_t>((a / xPoints) % yPoints);
      plane = static_cast<int64_t>(a / (xPoints * yPoints));
      for(int32_t k = 0; k < 6; k++) // Loop over the 6 face neighbors
      {
        good = true;
        neighbor = static_cast<int64_t>(a + neighbors[k]);
        if(k == 0 && plane == 0)
        {
          good = false;
        }
        if(k == 5 && plane == static_cast<int64_t>(zPoints - 1))
        {
          good = false;
        }
        if(k == 1 && row == 0)
        {
          good = false;
        }
        if(k == 4 && row == static_cast<int64_t>(yPoints - 1))
        {
          good = false;
        }
        if(k == 2 && column == 0)
        {
          good = false;
        }
        if(k == 3 && column == static_cast<int64_t>(xPoints - 1))
        {
          good = false;
        }
        // If we are a proper neighbor voxel, i.e., have not steppd out of the virtual volume,
        // and the featureId of the neighbor is NOT the currentFeatureId AND the
        // neighborFeatureId is valid ( greater than 0), then drop into this conditional
        if(good && featureIdsStore[neighbor] != feature && featureIdsStore[neighbor] >= 0)
        {
          bool add = true; // Default to always adding this neighbor to the coordination vector
          // Loop over current vector of coordination values
          for(const auto& coordination_value : coordination)
          {
            // If the featureId of the neighbor voxel == the current coordination_value
            // then we set the boolean to ignore this neighbor by setting `add = false`
            if(featureIdsStore[neighbor] == coordination_value)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            coordination.push_back(featureIdsStore[neighbor]); // Push back the first neighbor found
          }
        }
      }

      // now that the neighbors are found and the coordination size is found
      // If no values were pushed into the coordination vector then just initialize
      // all 3 components of the nearestNeighbors to -1
      if(coordination.empty())
      {
        nearestNeighbors[a * 3 + 0] = -1;
        nearestNeighbors[a * 3 + 1] = -1;
        nearestNeighbors[a * 3 + 2] = -1;
      }
      // If ANY values were pushed back into the coordination vector then this voxel
      // is a grain boundary. Initialize the first component of the nearestNeighbor to the
      // first value of the coordination vector.
      // Initialize the GB output array to 0
      if(!coordination.empty() && inputValues->DoBoundaries && nullptr != gbManhattanDistancesStore)
      {
        (*gbManhattanDistancesStore)[a] = 0;
        nearestNeighbors[a * 3 + 0] = coordination[0];
        nearestNeighbors[a * 3 + 1] = -1;
        nearestNeighbors[a * 3 + 2] = -1;
      }

      // Triple lines are defined as a line that separates 3, and only 3, grains.
      // Initialize the nearestNeighbor components 0 and 1 to the first value in the coordination vector
      // Initializes the TJ output array to 0;
      if(coordination.size() >= 2 && inputValues->DoTripleLines && nullptr != tjManhattanDistancesStore)
      {
        (*tjManhattanDistancesStore)[a] = 0;
        nearestNeighbors[a * 3 + 0] = coordination[0];
        nearestNeighbors[a * 3 + 1] = coordination[0];
        nearestNeighbors[a * 3 + 2] = -1;
      }

      // All other boundaries between 4 or more grains are Quadruple Points. Initialize
      // Initialize the nearestNeighbor components 0, 1, 2 to the first value in the coordination vector
      // Initializes the QP output array to 0.
      if(coordination.size() > 2 && inputValues->DoQuadPoints && nullptr != qpManhattanDistancesStore)
      {
        (*qpManhattanDistancesStore)[a] = 0;
        nearestNeighbors[a * 3 + 0] = coordination[0];
        nearestNeighbors[a * 3 + 1] = coordination[0];
        nearestNeighbors[a * 3 + 2] = coordination[0];
      }
      coordination.resize(0);
    }
  }

  // Now that we have all the necessary values, use TBB to initiate a task to compute
  // the output for each kind of selected output.
  ParallelTaskAlgorithm taskRunner;
  if(inputValues->DoBoundaries)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32, ComputeEuclideanDistMap::MapType::FeatureBoundary>(dataStructure, *inputValues, inputValues->GBDistancesArrayPath));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32, ComputeEuclideanDistMap::MapType::FeatureBoundary>(dataStructure, *inputValues, inputValues->GBDistancesArrayPath));
    }
  }

  if(inputValues->DoTripleLines)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32, ComputeEuclideanDistMap::MapType::TripleJunction>(dataStructure, *inputValues, inputValues->TJDistancesArrayPath));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32, ComputeEuclideanDistMap::MapType::TripleJunction>(dataStructure, *inputValues, inputValues->TJDistancesArrayPath));
    }
  }

  if(inputValues->DoQuadPoints)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32, ComputeEuclideanDistMap::MapType::QuadPoint>(dataStructure, *inputValues, inputValues->QPDistancesArrayPath));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32, ComputeEuclideanDistMap::MapType::QuadPoint>(dataStructure, *inputValues, inputValues->QPDistancesArrayPath));
    }
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
    findDistanceMap<int32>(m_DataStructure, m_InputValues);
  }
  else
  {
    findDistanceMap<float32>(m_DataStructure, m_InputValues);
  }

  return {};
}
