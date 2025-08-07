#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
struct GridBitMapFactory;

struct GridBitMap
{
  friend GridBitMapFactory;

  std::vector<uint8> gridTable = {};
  usize numGrids = 0;
  usize numPositions = 0;

  // This value represents the number of bytes allocated
  // to each row in the map
  // Reason: stored to speed up indexing and access
  usize rowLength = 0;

private:
  GridBitMap() = default;
};

struct GridBitMapFactory
{
  /**
   * Note here we can pack it slightly tighter by not adding buffers at the end of each row (for grid counts not divisible by 8)
   * but this will make calculations more difficult and costly during neighbor search
   * At most this saves 7/8s of a byte per position worth of space for significant calculation
   * and parse cost incursion
   */
  static GridBitMap createGridBitMap(usize numGrids, usize numPositons)
  {
    GridBitMap gridBitMap = {};

    usize bitPackSize = numGrids / 8;
    bitPackSize += static_cast<usize>((numGrids % 8 > 0)); // Cast to avoid if/else branch

    gridBitMap.numGrids = numGrids;
    gridBitMap.numPositions = numPositons;
    gridBitMap.rowLength = bitPackSize;

    gridBitMap.gridTable.resize(bitPackSize * numPositons);

    return gridBitMap;
  }
};

class HyperGridBitMap
{
public:
  struct GridCell
  {
    std::vector<usize> pointIndices = {};
  };

  // Grid Cells
  std::vector<GridCell> gridVoxels = {};

protected:
  HyperGridBitMap() = default;
};

class HyperGridBitMap3D : HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 3;

  GridBitMap xTable;
  GridBitMap yTable;
  GridBitMap zTable;

  HyperGridBitMap3D() = delete;

  template <typename T>
  HyperGridBitMap3D(const AbstractDataStore<T>& inputArray, float32 epsilon)
  : HyperGridBitMap()
  {
    /*
     * TODO:
     *  - Swap input array to datastore
     *  - Validate epsilon is not negative
     */

    // Load array bounds
    std::array<float32, 6> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      float32 xVal = inputArray.getValue((i * 3) + 0);
      float32 yVal = inputArray.getValue((i * 3) + 1);
      float32 zVal = inputArray.getValue((i * 3) + 2);

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);
      bounds[2] = std::isnan(bounds[2]) ? zVal : std::min(bounds[2], zVal);

      bounds[3] = std::isnan(bounds[3]) ? xVal : std::max(bounds[3], xVal);
      bounds[4] = std::isnan(bounds[4]) ? yVal : std::max(bounds[4], yVal);
      bounds[5] = std::isnan(bounds[5]) ? zVal : std::max(bounds[5], zVal);
    }

    // Grid Info
    float32 sideLength = epsilon / std::sqrt(Dimensions);
    std::array<float32, 3> spacing = {sideLength, sideLength, sideLength};

    float32 buffer = sideLength * 0.5f;
    std::array<float32, 3> origin = {};
    origin[0] = static_cast<float32>(bounds[0]) - buffer;
    origin[1] = static_cast<float32>(bounds[1]) - buffer;
    origin[2] = static_cast<float32>(bounds[2]) - buffer;

    std::array<usize, 3> dims = {};
    dims[0] = static_cast<usize>(((bounds[3] + buffer) - origin[0]) / spacing[0]);
    dims[1] = static_cast<usize>(((bounds[4] + buffer) - origin[1]) / spacing[1]);
    dims[2] = static_cast<usize>(((bounds[5] + buffer) - origin[2]) / spacing[2]);

    // Fill the BitMap
    {
      std::vector<std::array<usize, 3>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<GridCell> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()));
        // Load grid cells
        for(usize tup = 0; tup < inputArray.getNumberOfTuples(); tup++)
        {
          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfTuples();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);
          usize zPos = std::floor((inputArray.getValue(pointIdx + 2) - origin[2]) / spacing[2]);

          usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;

          grids[bin].pointIndices.push_back(tup);
        }

        usize zSize = dims[1] * dims[0];
        usize ySize = dims[0];
        for(usize i = 0; i < grids.size(); i++)
        {
          if(!grids[i].pointIndices.empty())
          {
            gridVoxels.push_back(std::move(grids[i]));

            std::array<usize, 3> position = {}; // Trivially copyable
            position[2] = i / zSize;
            usize zRemdr = i % zSize; // Modern compilers will extract the result from previous instruction
            position[1] = zRemdr / ySize;
            position[0] = zRemdr % ySize; // Modern compilers will extract the result from previous instruction
            positions.push_back(position);
          }
        }
      } // End of filling non-empty grids and positions vector

      /**
       * This could be modified to 3 passes on the positions vector with custom predicates and ths std::sort function,
       * but we are sacrificing space for speed, because its a subset of a known predefined grid
       */
      // Make sets to bin grids
      std::set<usize> xSet = {};
      std::set<usize> ySet = {};
      std::set<usize> zSet = {};

      for(const auto& position : positions)
      {
        xSet.insert(position[0]);
        ySet.insert(position[1]);
        zSet.insert(position[2]);
      }

      // Set up hyper bit map
      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());
      zTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), zSet.size());

      // Not the most efficient fill but due to the random access nature of position
      // we can't load one consecutive mask
      for(usize gridId = 0; gridId < positions.size(); gridId++)
      {
        usize relativeGridBytePos = gridId / 8;
        uint8 bitGridOffset = gridId % 8;

        usize xPos = std::distance(xSet.begin(), xSet.find(positions[gridId][0])) * xTable.rowLength;
        usize yPos = std::distance(ySet.begin(), ySet.find(positions[gridId][1])) * yTable.rowLength;
        usize zPos = std::distance(zSet.begin(), zSet.find(positions[gridId][2])) * zTable.rowLength;

        usize xBytePos = xPos + relativeGridBytePos;
        uint8 xMask = 1;
        xMask <<= bitGridOffset;
        xTable.gridTable[xBytePos] |= xMask;

        usize yBytePos = yPos + relativeGridBytePos;
        uint8 yMask = 1;
        yMask <<= bitGridOffset;
        yTable.gridTable[yBytePos] |= yMask;

        usize zBytePos = zPos + relativeGridBytePos;
        uint8 zMask = 1;
        zMask <<= bitGridOffset;
        zTable.gridTable[zBytePos] |= zMask;
      }
    }
  }
};

class HyperGridBitMap2D : HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 2;

  GridBitMap xTable;
  GridBitMap yTable;

  HyperGridBitMap2D() = delete;

  template <typename T>
  HyperGridBitMap2D(const AbstractDataStore<T>& inputArray, float32 epsilon)
  : HyperGridBitMap()
  {
    /*
     * TODO:
     *  - Swap input array to datastore
     *  - Validate epsilon is not negative
     */

    // Load array bounds
    std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      // Determine the voxel
      usize pointIdx = i * inputArray.getNumberOfTuples();
      float32 xVal = inputArray.getValue((pointIdx * 2) + 0);
      float32 yVal = inputArray.getValue((pointIdx * 2) + 1);

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

      bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
      bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
    }

    // Grid Info
    float32 sideLength = epsilon / std::sqrt(Dimensions);
    std::array<float32, 2> spacing = {sideLength, sideLength};

    float32 buffer = sideLength * 0.5f;
    std::array<float32, 2> origin = {};
    origin[0] = static_cast<float32>(bounds[0]) - buffer;
    origin[1] = static_cast<float32>(bounds[1]) - buffer;

    std::array<usize, 2> dims = {};
    dims[0] = static_cast<usize>(((bounds[2] + buffer) - origin[0]) / spacing[0]);
    dims[1] = static_cast<usize>(((bounds[3] + buffer) - origin[1]) / spacing[1]);

    // Fill the BitMap
    {
      std::vector<std::array<usize, 2>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<GridCell> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()));
        // Load grid cells
        for(usize tup = 0; tup < inputArray.getNumberOfTuples(); tup++)
        {
          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfTuples();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;

          grids[bin].pointIndices.push_back(tup);
        }

        usize ySize = dims[0];
        for(usize i = 0; i < grids.size(); i++)
        {
          if(!grids[i].pointIndices.empty())
          {
            gridVoxels.push_back(std::move(grids[i]));

            std::array<usize, 2> position = {}; // Trivially copyable
            position[1] = i / ySize;
            position[0] = i % ySize; // Modern compilers will extract the result from previous instruction
            positions.push_back(position);
          }
        }
      } // End of filling non-empty grids and positions vector

      /**
       * This could be modified to 3 passes on the positions vector with custom predicates and ths std::sort function,
       * but we are sacrificing space for speed, because its a subset of a known predefined grid
       */
      // Make sets to bin grids
      std::set<usize> xSet = {};
      std::set<usize> ySet = {};

      for(const auto& position : positions)
      {
        xSet.insert(position[0]);
        ySet.insert(position[1]);
      }

      // Set up hyper bit map
      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());

      // Not the most efficient fill but due to the random access nature of position
      // we can't load one consecutive mask
      for(usize gridId = 0; gridId < positions.size(); gridId++)
      {
        usize relativeGridBytePos = gridId / 8;
        uint8 bitGridOffset = gridId % 8;

        usize xPos = std::distance(xSet.begin(), xSet.find(positions[gridId][0])) * xTable.rowLength;
        usize yPos = std::distance(ySet.begin(), ySet.find(positions[gridId][1])) * yTable.rowLength;

        usize xBytePos = xPos + relativeGridBytePos;
        uint8 xMask = 1;
        xMask <<= bitGridOffset;
        xTable.gridTable[xBytePos] |= xMask;

        usize yBytePos = yPos + relativeGridBytePos;
        uint8 yMask = 1;
        yMask <<= bitGridOffset;
        yTable.gridTable[yBytePos] |= yMask;
      }
    }
  }
};

void SearchTablePositions(std::vector<uint8>& outputGridMask, usize searchSpace, usize targetPosition, const GridBitMap& selectedTable)
{
  std::vector<uint8> tempGridMask(selectedTable.rowLength, 0);

  // Find indices to search space
  usize xStart = (targetPosition < searchSpace) ? 0 : targetPosition - searchSpace;
  usize xEnd = (targetPosition + searchSpace < selectedTable.numPositions) ? targetPosition + searchSpace + 1 : selectedTable.numPositions;

  // Store all grids in the positions within the dimensional search space
  for(usize pos = xStart; pos < xEnd; pos++)
  {
    for(usize i = 0; i < selectedTable.rowLength; i++)
    {
      tempGridMask[i] |= selectedTable.gridTable[(pos * selectedTable.rowLength) + i];
    }
  }

  // Narrow down search by overlaying this dimension's search space
  // onto previous dimensions search space
  for(usize i = 0; selectedTable.rowLength; i++)
  {
    outputGridMask[i] &= tempGridMask[i];
  }
}

template <class HGBMT>
concept IsHGBP = std::is_base_of_v<HyperGridBitMap, HGBMT>;

template <IsHGBP HGBPT>
std::vector<usize> NeighborGridQuery(usize targetGridId, const HGBPT& hyperGridBitMap)
{
  usize searchSpace = std::ceil(std::sqrt(HGBPT::Dimensions));

  std::vector<usize> neighborGridIds = {};

  // check adjacent positions in the table by sqrt(Dimensions) for grid ids
  std::vector<uint8> finalGridMask(hyperGridBitMap.gridVoxels.size(), std::numeric_limits<uint8>::max());

  // The search loops to find xyzPos can be cut if we opt to store the
  // positions for each grid cell within each cell or in a separate vector
  usize relativeGridBytePos = targetGridId / 8;
  uint8 bitGridOffset = targetGridId % 8;

  usize xPos = 0;
  for(usize i = 0; i < hyperGridBitMap.xTable.numPositions; i++)
  {
    usize gridPos = (i * hyperGridBitMap.xTable.rowLength) + relativeGridBytePos;
    uint8 mask = 1;
    mask <<= bitGridOffset;
    uint8 result = hyperGridBitMap.xTable.gridTable[gridPos] & mask;
    if(result > 0)
    {
      xPos = i;
      break;
    }
  }
  SearchTablePositions(finalGridMask, searchSpace, xPos, hyperGridBitMap.xTable);

  usize yPos = 0;
  for(usize i = 0; i < hyperGridBitMap.yTable.numPositions; i++)
  {
    usize gridPos = (i * hyperGridBitMap.yTable.rowLength) + relativeGridBytePos;
    uint8 mask = 1;
    mask <<= bitGridOffset;
    uint8 result = hyperGridBitMap.yTable.gridTable[gridPos] & mask;
    if(result > 0)
    {
      yPos = i;
      break;
    }
  }
  SearchTablePositions(finalGridMask, searchSpace, yPos, hyperGridBitMap.yTable);

  if constexpr(HGBPT::Dimensions == 3)
  {
    usize zPos = 0;
    for(usize i = 0; i < hyperGridBitMap.zTable.numPositions; i++)
    {
      usize gridPos = (i * hyperGridBitMap.zTable.rowLength) + relativeGridBytePos;
      uint8 mask = 1;
      mask <<= bitGridOffset;
      uint8 result = hyperGridBitMap.zTable.gridTable[gridPos] & mask;
      if(result > 0)
      {
        zPos = i;
        break;
      }
    }
    SearchTablePositions(finalGridMask, searchSpace, zPos, hyperGridBitMap.zTable);
  }

  for(usize i = 0; i < finalGridMask.size(); i++)
  {
    if(finalGridMask[i] > 0)
    {
      for(uint8 bit = 0; bit < 8; bit++)
      {
        if((finalGridMask[i] & (1 << bit)) != 0)
        {
          neighborGridIds.push_back((i * 8) + bit);
        }
      }
    }
  }

  return neighborGridIds;
}

template <typename T>
class FindEpsilonNeighborhoodsImpl
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  FindEpsilonNeighborhoodsImpl(DBSCAN* filter, float64 epsilon, const AbstractDataStoreT& inputData, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, usize numCompDims, usize numTuples,
                               ClusterUtilities::DistanceMetric distMetric, std::vector<std::list<usize>>& neighborhoods)
  : m_Filter(filter)
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputData)
  , m_Mask(mask)
  , m_NumCompDims(numCompDims)
  , m_NumTuples(numTuples)
  , m_DistMetric(distMetric)
  , m_Neighborhoods(neighborhoods)
  {
  }

  void compute(usize start, usize end) const
  {
    for(usize i = start; i < end; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      if(m_Mask->isTrue(i))
      {
        // directly inline to try to convince compiler to construct in place
        m_Neighborhoods[i] = epsilon_neighbors(i);
      }
    }
  }

  [[nodiscard]] std::list<usize> epsilon_neighbors(usize index) const
  {
    std::list<usize> neighbors;

    for(usize i = 0; i < m_NumTuples; i++)
    {
      if(m_Mask->isTrue(i))
      {
        float64 dist = ClusterUtilities::GetDistance(m_InputDataStore, (m_NumCompDims * index), m_InputDataStore, (m_NumCompDims * i), m_NumCompDims, m_DistMetric);
        if(dist < m_Epsilon)
        {
          neighbors.push_back(i);
        }
      }
    }

    return neighbors;
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  DBSCAN* m_Filter;
  float64 m_Epsilon;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  usize m_NumCompDims;
  usize m_NumTuples;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::vector<std::list<usize>>& m_Neighborhoods;
};

template <typename T, bool PrecacheV = true, bool RandomInitV = true>
class DBSCANTemplate
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  DBSCANTemplate(DBSCAN* filter, const AbstractDataStoreT& inputDataStore, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray, AbstractDataStore<int32>& fIdsDataStore,
                 float32 epsilon, int32 minPoints, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed, MessageHelper& messageHelper)
  : m_Filter(filter)
  , m_InputDataStore(inputDataStore)
  , m_Mask(maskDataArray)
  , m_FeatureIds(fIdsDataStore)
  , m_Epsilon(epsilon)
  , m_MinPoints(minPoints)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  , m_MessageHelper(messageHelper)
  {
  }
  ~DBSCANTemplate() = default;

  DBSCANTemplate(const DBSCANTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const DBSCANTemplate&) = delete; // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputDataStore.getNumberOfTuples();
    usize numCompDims = m_InputDataStore.getNumberOfComponents();
    std::vector<bool> visited(numTuples, false);   // Uses one bit per value for space efficiency
    std::vector<bool> clustered(numTuples, false); // Uses one bit per value for space efficiency

    auto minDist = static_cast<float64>(m_Epsilon);
    int32 cluster = 0;

    std::vector<std::list<usize>> epsilonNeighborhoods;

    if constexpr(PrecacheV)
    {
      // In-memory only with current implementation for speed with std::list
      epsilonNeighborhoods = std::vector<std::list<usize>>(numTuples);

      m_MessageHelper.sendMessage("Finding Neighborhoods in parallel...");
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0ULL, numTuples);
      dataAlg.execute(FindEpsilonNeighborhoodsImpl<T>(m_Filter, minDist, m_InputDataStore, m_Mask, numCompDims, numTuples, m_DistMetric, epsilonNeighborhoods));

      m_MessageHelper.sendMessage("Neighborhoods found.");
    }

    std::mt19937_64 gen(m_Seed);
    std::uniform_int_distribution<usize> dist(0, numTuples - 1);

    m_MessageHelper.sendMessage("Beginning clustering...");
    ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
    usize i = 0;
    uint8 misses = 0;
    while(std::find(visited.begin(), visited.end(), false) != visited.end())
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      usize index;
      if constexpr(!RandomInitV)
      {
        index = i;
        if(i >= numTuples)
        {
          break;
        }
        i++;
      }
      if constexpr(RandomInitV)
      {
        index = dist(gen);
      }

      if(visited[index])
      {
        if(misses >= 10)
        {
          auto findIter = std::find(visited.begin(), visited.end(), false);
          if(findIter == visited.end())
          {
            break;
          }
          index = std::distance(visited.begin(), findIter);

          if constexpr(RandomInitV)
          {
            dist = std::uniform_int_distribution<usize>(index, numTuples - 1);
          }
        }
        else
        {
          misses++;
          continue;
        }
      }

      misses = 0;

      if(m_Mask->isTrue(index))
      {
        visited[index] = true;

        throttledMessenger.sendThrottledMessage([&]() {
          float32 progress = (static_cast<float32>(index) / static_cast<float32>(numTuples)) * 100.0f;
          return fmt::format("Scanning Data || Visited Point {} of {} || {:.2f}% Completed", index, numTuples, progress);
        });

        std::list<usize> neighbors;
        if constexpr(PrecacheV)
        {
          neighbors = epsilonNeighborhoods[index];
        }
        if constexpr(!PrecacheV)
        {
          for(usize j = 0; j < numTuples; j++)
          {
            if(m_Mask->isTrue(j))
            {
              float64 distance = ClusterUtilities::GetDistance(m_InputDataStore, (numCompDims * index), m_InputDataStore, (numCompDims * j), numCompDims, m_DistMetric);
              if(distance < m_Epsilon)
              {
                neighbors.push_back(j);
              }
            }
          }
        }

        if(static_cast<int32>(neighbors.size()) < m_MinPoints)
        {
          m_FeatureIds[index] = 0;
          clustered[index] = true;
        }
        else
        {
          if(m_Filter->getCancel())
          {
            return;
          }
          cluster++;
          m_FeatureIds[index] = cluster;
          clustered[index] = true;

          for(auto&& idx : neighbors)
          {
            if(m_Mask->isTrue(idx))
            {
              if(!visited[idx])
              {
                visited[idx] = true;

                std::list<usize> neighbors_prime;
                if constexpr(PrecacheV)
                {
                  neighbors_prime = epsilonNeighborhoods[idx];
                }
                if constexpr(!PrecacheV)
                {
                  for(usize j = 0; j < numTuples; j++)
                  {
                    if(m_Mask->isTrue(j))
                    {
                      float64 distance = ClusterUtilities::GetDistance(m_InputDataStore, (numCompDims * idx), m_InputDataStore, (numCompDims * j), numCompDims, m_DistMetric);
                      if(distance < m_Epsilon)
                      {
                        neighbors_prime.push_back(j);
                      }
                    }
                  }
                }

                if(static_cast<int32>(neighbors_prime.size()) >= m_MinPoints)
                {
                  neighbors.splice(std::end(neighbors), neighbors_prime);
                }
              }
              if(!clustered[idx])
              {
                m_FeatureIds[idx] = cluster;
                clustered[idx] = true;
              }
            }
          }
        }
      }
      else
      {
        visited[index] = true;
      }
    }
    m_MessageHelper.sendMessage("Clustering Complete!");
  }

private:
  DBSCAN* m_Filter;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  AbstractDataStore<int32>& m_FeatureIds;
  float32 m_Epsilon;
  int32 m_MinPoints;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;
  MessageHelper& m_MessageHelper;
};

struct DBSCANFunctor
{
  template <typename T>
  void operator()(bool cache, bool useRandom, DBSCAN* filter, const IDataArray& inputIDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskCompare, Int32Array& fIds,
                  float32 epsilon, int32 minPoints, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed, MessageHelper& messageHelper)
  {
    if(cache)
    {
      if(useRandom)
      {
        DBSCANTemplate<T, true, true>(filter, inputIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed,
                                      messageHelper)();
      }
      else
      {
        DBSCANTemplate<T, true, false>(filter, inputIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed,
                                       messageHelper)();
      }
    }
    else
    {
      if(useRandom)
      {
        DBSCANTemplate<T, false, true>(filter, inputIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed,
                                       messageHelper)();
      }
      else
      {
        DBSCANTemplate<T, false, false>(filter, inputIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed,
                                        messageHelper)();
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& DBSCAN::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DBSCAN::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);

  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54060, message);
  }

  ExecuteNeighborFunction(DBSCANFunctor{}, clusteringArray.getDataType(), m_InputValues->AllowCaching, m_InputValues->UseRandom, this, clusteringArray, maskCompare, featureIds, m_InputValues->Epsilon,
                          m_InputValues->MinPoints, m_InputValues->DistanceMetric, m_InputValues->Seed, messageHelper);

  messageHelper.sendMessage("Resizing Clustering Attribute Matrix...");
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(AttributeMatrix::ShapeType{static_cast<usize>(maxCluster + 1)});

  return {};
}
