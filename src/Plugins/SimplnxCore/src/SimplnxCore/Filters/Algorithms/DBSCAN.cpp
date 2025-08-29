#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
struct GridBitMap
{
  std::vector<uint8> gridTable = {};
  usize numPositions = 0;

  // This value represents the number of bytes allocated
  // to each row in the map
  // Reason: stored to speed up indexing and access
  usize rowLength = 0;
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

    gridBitMap.numPositions = numPositons;
    gridBitMap.rowLength = bitPackSize;

    gridBitMap.gridTable.resize(bitPackSize * numPositons);

    return gridBitMap;
  }
};

class HyperGridBitMap
{
public:
  // Grid Cells
  std::vector<std::vector<usize>> gridVoxels = {};

protected:
  HyperGridBitMap() = default;
};

class HyperGridBitMap3D : public HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 3;

  GridBitMap xTable;
  GridBitMap yTable;
  GridBitMap zTable;

  HyperGridBitMap3D() = delete;

  template <typename T>
  HyperGridBitMap3D(const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    // Load array bounds
    std::array<float32, 6> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      if(!mask->isTrue(i))
      {
        continue;
      }

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

    float32 buffer = sideLength;
    std::array<float32, 3> origin = {};
    origin[0] = static_cast<float32>(bounds[0]) - buffer;
    origin[1] = static_cast<float32>(bounds[1]) - buffer;
    origin[2] = static_cast<float32>(bounds[2]) - buffer;

    std::array<usize, 3> dims = {};
    dims[0] = static_cast<usize>(((bounds[3] + buffer) - origin[0]) / spacing[0]) + 2;
    dims[1] = static_cast<usize>(((bounds[4] + buffer) - origin[1]) / spacing[1]) + 2;
    dims[2] = static_cast<usize>(((bounds[5] + buffer) - origin[2]) / spacing[2]) + 2;

    // Fill the BitMap
    {
      std::vector<std::array<usize, 3>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<std::vector<usize>> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()));
        // Load grid cells
        for(usize tup = 0; tup < inputArray.getNumberOfTuples(); tup++)
        {
          if(!mask->isTrue(tup))
          {
            continue;
          }
          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfComponents();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);
          usize zPos = std::floor((inputArray.getValue(pointIdx + 2) - origin[2]) / spacing[2]);

          usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;

          grids[bin].push_back(tup);
        }

        usize zSize = dims[1] * dims[0];
        usize ySize = dims[0];
        for(usize i = 0; i < grids.size(); i++)
        {
          if(!grids[i].empty())
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
        xTable.gridTable[xBytePos] = xMask | xTable.gridTable[xBytePos];

        usize yBytePos = yPos + relativeGridBytePos;
        uint8 yMask = 1;
        yMask <<= bitGridOffset;
        yTable.gridTable[yBytePos] = yMask | yTable.gridTable[yBytePos];

        usize zBytePos = zPos + relativeGridBytePos;
        uint8 zMask = 1;
        zMask <<= bitGridOffset;
        zTable.gridTable[zBytePos] = zMask | zTable.gridTable[zBytePos];
      }
    }
  }
};

class HyperGridBitMap2D : public HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 2;

  GridBitMap xTable;
  GridBitMap yTable;

  HyperGridBitMap2D() = delete;

  template <typename T>
  HyperGridBitMap2D(const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    // Load array bounds
    std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      if(!mask->isTrue(i))
      {
        continue;
      }

      // Determine the voxel
      float32 xVal = inputArray.getValue((i * 2) + 0);
      float32 yVal = inputArray.getValue((i * 2) + 1);

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

      bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
      bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
    }

    // Grid Info
    float32 sideLength = epsilon / std::sqrt(Dimensions);
    std::array<float32, 2> spacing = {sideLength, sideLength};

    float32 buffer = sideLength;
    std::array<float32, 2> origin = {};
    origin[0] = static_cast<float32>(bounds[0]) - buffer;
    origin[1] = static_cast<float32>(bounds[1]) - buffer;

    std::array<usize, 2> dims = {};
    dims[0] = static_cast<usize>(((bounds[2] + buffer) - origin[0]) / spacing[0]) + 2;
    dims[1] = static_cast<usize>(((bounds[3] + buffer) - origin[1]) / spacing[1]) + 2;

    // Fill the BitMap
    {
      std::vector<std::array<usize, 2>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<std::vector<usize>> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()));
        // Load grid cells
        for(usize tup = 0; tup < inputArray.getNumberOfTuples(); tup++)
        {
          if(!mask->isTrue(tup))
          {
            continue;
          }

          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfComponents();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;

          grids[bin].push_back(tup);
        }

        usize ySize = dims[0];
        for(usize i = 0; i < grids.size(); i++)
        {
          if(!grids[i].empty())
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
        xTable.gridTable[xBytePos] = xMask | xTable.gridTable[xBytePos];

        usize yBytePos = yPos + relativeGridBytePos;
        uint8 yMask = 1;
        yMask <<= bitGridOffset;
        yTable.gridTable[yBytePos] = yMask | yTable.gridTable[yBytePos];
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
      tempGridMask[i] = tempGridMask[i] | selectedTable.gridTable[(pos * selectedTable.rowLength) + i];
    }
  }

  // Narrow down search by overlaying this dimension's search space
  // onto previous dimensions search space
  for(usize i = 0; i < selectedTable.rowLength; i++)
  {
    outputGridMask[i] = tempGridMask[i] & outputGridMask[i];
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
  std::vector<uint8> finalGridMask(hyperGridBitMap.xTable.rowLength, std::numeric_limits<uint8>::max());

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

struct ClusterNode
{
  int32 clusterId;
  usize parent;
};

struct ClusterForest
{
  std::vector<ClusterNode> clusterForestNodes = {};
  std::vector<usize> coreGridIds = {};

  void initialize(usize numGrids)
  {
    clusterForestNodes.resize(numGrids);

    for(usize i = 0; i < clusterForestNodes.size(); i++)
    {
      clusterForestNodes[i].parent = i;
      clusterForestNodes[i].clusterId = static_cast<int32>(i + 1);
    }
  }

  usize findClusterRoot(usize gridId)
  {
    if(clusterForestNodes[gridId].parent == gridId)
    {
      return gridId;
    }

    return findClusterRoot(clusterForestNodes[gridId].parent);
  }

  bool infer(usize pGridId, usize qGridId)
  {
    return findClusterRoot(pGridId) == findClusterRoot(qGridId);
  }

  void mergeLRC(const std::vector<usize>& gridIds)
  {
    if(gridIds.size() < 2)
    {
      return;
    }

    std::vector<usize> rootClusterIdx = {};

    usize lowestClusterIdx = findClusterRoot(gridIds[0]);
    rootClusterIdx.push_back(lowestClusterIdx);
    for(usize i = 1; i < gridIds.size(); i++)
    {
      usize clusterIndex = findClusterRoot(gridIds[i]);
      rootClusterIdx.push_back(clusterIndex);
      if(clusterForestNodes[clusterIndex].clusterId < clusterForestNodes[lowestClusterIdx].clusterId)
      {
        lowestClusterIdx = clusterIndex;
      }
    }

    for(const usize clusterIdx : rootClusterIdx)
    {
      if(lowestClusterIdx != clusterIdx)
      {
        clusterForestNodes[clusterIdx].parent = clusterForestNodes[lowestClusterIdx].parent;
      }
    }
  }
};

template <IsHGBP HGBPT, typename T>
class GDCF
{
public:
  GDCF() = delete;
  GDCF(const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, ClusterUtilities::DistanceMetric distMetric)
  : hyperGridBitMap(HGBPT(inputArray, epsilon, mask))
  , m_InputDataStore(inputArray)
  , m_Epsilon(epsilon)
  , m_DistMetric(distMetric)
  {
  }

  void cluster(usize minPoints, DBSCAN::ParseOrder parseOrder, std::mt19937_64::result_type seed = std::mt19937_64::default_seed)
  {
    // Identify Core Grids
    for(usize i = 0; i < hyperGridBitMap.gridVoxels.size(); i++)
    {
      if(hyperGridBitMap.gridVoxels[i].size() >= minPoints)
      {
        clusterForest.coreGridIds.push_back(i);
      }
    }
    if(clusterForest.coreGridIds.empty())
    {
      return;
    }

    // Sort Grids to reduce bias
    switch(parseOrder)
    {
    case DBSCAN::ParseOrder::LowDensityFirst: {
      QuickSortGrids(clusterForest.coreGridIds, 0, clusterForest.coreGridIds.size() - 1);
      break;
    }
    case DBSCAN::ParseOrder::Random: {
      std::mt19937_64 gen(seed);
      std::uniform_real_distribution<float64> dist(0, 1);

      auto maxIdx = static_cast<float64>(clusterForest.coreGridIds.size() - 1);

      //--- Shuffle elements by randomly exchanging each with one other.
      for(usize i = 1; i < clusterForest.coreGridIds.size(); i++)
      {
        auto r = static_cast<usize>(std::floor(dist(gen) * maxIdx)); // Random remaining position.

        std::swap(clusterForest.coreGridIds[i], clusterForest.coreGridIds[r]);
      }

      break;
    }
    case DBSCAN::SeededRandom: {
      std::mt19937_64 gen(seed);
      std::uniform_real_distribution<float64> dist(0, 1);

      auto maxIdx = static_cast<float64>(clusterForest.coreGridIds.size() - 1);

      //--- Shuffle elements by randomly exchanging each with one other.
      for(usize i = 1; i < clusterForest.coreGridIds.size(); i++)
      {
        auto r = static_cast<usize>(std::floor(dist(gen) * maxIdx)); // Random remaining position.

        std::swap(clusterForest.coreGridIds[i], clusterForest.coreGridIds[r]);
      }

      break;
    }
    }

    /** This is to validate NN Query for debugging
     * REMOVE LATER
     */
    //        clusterForest.initialize(hyperGridBitMap.gridVoxels.size());
    //        usize targetCoreGrid = 0;
    //        std::vector<usize> neighborGrids = NeighborGridQuery(clusterForest.coreGridIds[targetCoreGrid], hyperGridBitMap);
    //        for(const usize gridId : neighborGrids)
    //        {
    //          if(canMerge(clusterForest.coreGridIds[targetCoreGrid], gridId))
    //          {
    //            clusterForest.clusterForestNodes[gridId].parent = clusterForest.coreGridIds[targetCoreGrid];
    //          }
    //        }
    //
    //        clusterForest.clusterForestNodes[clusterForest.coreGridIds[targetCoreGrid]].clusterId = 1;
    //
    //        for(usize i = 0; i < clusterForest.clusterForestNodes.size(); i++)
    //        {
    //          if(i == clusterForest.coreGridIds[targetCoreGrid])
    //          {
    //            continue;
    //          }
    //          clusterForest.clusterForestNodes[i].clusterId = 0;
    //        }

    clusterForest.initialize(hyperGridBitMap.gridVoxels.size());
    for(usize i = 0; i < clusterForest.coreGridIds.size(); i++)
    {
      std::vector<usize> neighborGrids = NeighborGridQuery(clusterForest.coreGridIds[i], hyperGridBitMap);

      std::vector<usize> cluster = {};
      cluster.push_back(clusterForest.coreGridIds[i]);
      for(const usize gridId : neighborGrids)
      {
        if(clusterForest.infer(clusterForest.coreGridIds[i], gridId))
        {
          continue;
        }

        if(canMerge(clusterForest.coreGridIds[i], gridId))
        {
          // Check if it's a border grid and check if its unvisited
          if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && clusterForest.clusterForestNodes[gridId].parent == gridId)
          {
            // Border grids can not be their own cluster, which means this
            // is unvisited currently so merge it into the current cluster
            clusterForest.clusterForestNodes[gridId].parent = clusterForest.coreGridIds[i];
          }
          else
          {
            // Either this is a density-reachable core grid
            // OR
            // This border grid belongs to another cluster, but the fact it is
            // reachable here means that the two clusters are one and need to
            // be merged
            cluster.push_back(gridId);
          }
        }
      }

      clusterForest.mergeLRC(cluster);
    }

    /**
     * Re-evaluate core point labeling to account for cluster expansion.
     * Cluster Expansion:
     * Starting from an unvisited core point, a new cluster is initiated.
     * All density-reachable points from this core point (including other core points and border points) are added to this cluster.
     * This process recursively expands the cluster by considering newly added core points and their density-reachable points.
     */

    // Now determine if non-core grids are close enough to a cluster to be border else noise
    usize operations = 0;
    do
    {
      operations = 0;
      for(usize i = 0; i < hyperGridBitMap.gridVoxels.size(); i++)
      {
        if(hyperGridBitMap.gridVoxels[i].size() < minPoints)
        {
          std::vector<usize> neighborGrids = NeighborGridQuery(i, hyperGridBitMap);

          for(const usize gridId : neighborGrids)
          {
            if(clusterForest.infer(i, gridId))
            {
              continue;
            }

            if(canMerge(i, gridId))
            {
              usize activeParent = clusterForest.findClusterRoot(i);
              usize neighborGridParent = clusterForest.findClusterRoot(gridId);
              // Check if search grid has been visited
              if(activeParent == i)
              {
                if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && neighborGridParent == gridId)
                {
                  // Border grids can not be their own cluster, which means this
                  // is unvisited currently;
                  continue;
                }
                clusterForest.clusterForestNodes[i].parent = neighborGridParent;
              }
              else
              {
                if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && neighborGridParent == gridId)
                {
                  clusterForest.clusterForestNodes[gridId].parent = activeParent;
                }
                else
                {
                  // Infer returning false means that they can't have the same cluster id so else must be greater than
                  if(clusterForest.clusterForestNodes[activeParent].clusterId < clusterForest.clusterForestNodes[neighborGridParent].clusterId)
                  {
                    clusterForest.clusterForestNodes[neighborGridParent].parent = activeParent;
                  }
                  else
                  {
                    clusterForest.clusterForestNodes[activeParent].parent = neighborGridParent;
                  }
                }
              }
              operations++;
            }
          }
        }
      }
    } while(operations > 0);

    // clean up cluster forest
    std::vector<usize> clusters = {};
    for(usize i = 0; i < clusterForest.clusterForestNodes.size(); i++)
    {
      if(clusterForest.clusterForestNodes[i].parent == i)
      {
        if(hyperGridBitMap.gridVoxels[i].size() >= minPoints)
        {
          // Only core nodes can be their own parent
          clusters.push_back(i);
        }
        else
        {
          // grid unreachable, label noise
          clusterForest.clusterForestNodes[i].clusterId = 0;
        }
      }
    }

    for(usize i = 0; i < clusters.size(); i++)
    {
      clusterForest.clusterForestNodes[clusters[i]].clusterId = static_cast<int32>(i + 1);
    }
  }

  void label(AbstractDataStore<int32>& fIdsDataStore)
  {
    if(clusterForest.clusterForestNodes.empty())
    {
      return;
    }

    // label
    fIdsDataStore.fill(0);
    for(usize gridIdx = 0; gridIdx < hyperGridBitMap.gridVoxels.size(); gridIdx++)
    {
      int32 featureId = clusterForest.clusterForestNodes[clusterForest.findClusterRoot(gridIdx)].clusterId;
      for(usize pointIdx : hyperGridBitMap.gridVoxels[gridIdx])
      {
        fIdsDataStore.setValue(pointIdx, featureId);
      }
    }
  }

private:
  HGBPT hyperGridBitMap;

  ClusterForest clusterForest = {};

  float32 m_Epsilon;
  const AbstractDataStore<T>& m_InputDataStore;
  ClusterUtilities::DistanceMetric m_DistMetric;

  // Uses Hoare's method for speed
  usize ProcessSection(std::vector<usize>& sorted, usize begin, usize end) const
  {
    const usize threshold = hyperGridBitMap.gridVoxels[sorted[begin]].size();

    usize front = begin;
    usize back = end;

    while(true)
    {
      while(hyperGridBitMap.gridVoxels[sorted[front]].size() < threshold)
      {
        front++;
      }

      while(hyperGridBitMap.gridVoxels[sorted[back]].size() > threshold)
      {
        back--;
      }

      if(front >= back)
      {
        return back;
      }

      std::swap(sorted[front], sorted[back]);
      front++;
      back--;
    }
  }

  void QuickSortGrids(std::vector<usize>& sorted, usize begin, usize end) const
  {
    if(begin >= end)
    {
      return;
    }

    usize next = ProcessSection(sorted, begin, end);

    // Recurse
    QuickSortGrids(sorted, begin, next);
    QuickSortGrids(sorted, next + 1, end);
  }

  bool canMerge(usize pGridId, usize qGridId)
  {
    for(usize pPointId : hyperGridBitMap.gridVoxels[pGridId])
    {
      for(usize qPointId : hyperGridBitMap.gridVoxels[qGridId])
      {
        float64 dist = ClusterUtilities::GetDistance(m_InputDataStore, (HGBPT::Dimensions * pPointId), m_InputDataStore, (HGBPT::Dimensions * qPointId), HGBPT::Dimensions, m_DistMetric);
        if(dist < m_Epsilon)
        {
          return true;
        }
      }
    }

    return false;
  }
};

template <typename T>
void MakeEdgeGeom(DataStructure& dataStructure, float32 epsilon, const AbstractDataStore<T>& inputArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, std::string&& name)
{
  // Load array bounds
  std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                   std::numeric_limits<float32>::quiet_NaN()};
  for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
  {
    if(!mask->isTrue(i))
    {
      continue;
    }

    // Determine the voxel
    float32 xVal = inputArray.getValue((i * 2) + 0);
    float32 yVal = inputArray.getValue((i * 2) + 1);

    bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
    bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

    bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
    bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
  }

  // Grid Info
  float32 sideLength = epsilon / std::sqrt(2);
  std::array<float32, 2> spacing = {sideLength, sideLength};

  float32 buffer = sideLength;
  std::array<float32, 2> origin = {};
  origin[0] = static_cast<float32>(bounds[0]) - buffer;
  origin[1] = static_cast<float32>(bounds[1]) - buffer;

  std::array<usize, 2> dims = {};
  dims[0] = static_cast<usize>(((bounds[2] + buffer) - origin[0]) / spacing[0]) + 2;
  dims[1] = static_cast<usize>(((bounds[3] + buffer) - origin[1]) / spacing[1]) + 2;

  auto* edgeGeom = EdgeGeom::Create(dataStructure, name);
  std::string sharedVertListName = "SharedVertList";
  size_t numVerts = dims[0] * dims[1];
  size_t numComps = 3;
  IGeometry::SharedVertexList* vertices = Float32Array::CreateWithStore<DataStore<float>>(dataStructure, sharedVertListName, {numVerts}, {numComps}, edgeGeom->getId());
  edgeGeom->setVertices(*vertices);

  std::string sharedEdgeListName = "SharedEdgeList";
  size_t numEdges = ((dims[0] - 1) * dims[1]) + ((dims[1] - 1) * dims[0]);
  size_t numEdgeComps = 2;
  IGeometry::SharedEdgeList* edges = IGeometry::SharedEdgeList::CreateWithStore<DataStore<EdgeGeom::MeshIndexType>>(dataStructure, sharedEdgeListName, {numEdges}, {numEdgeComps}, edgeGeom->getId());
  edgeGeom->setEdgeList(*edges);

  auto& vertList = edgeGeom->getVerticesRef();
  auto& edgeList = edgeGeom->getEdgesRef();
  for(uint64 j = 0; j < dims[1]; j++)
  {
    usize yStride = j * dims[0];
    for(uint64 k = 0; k < dims[0]; k++)
    {
      vertList.setValue(((yStride + k) * 3) + 0, k * spacing[0] + origin[0]);
      vertList.setValue(((yStride + k) * 3) + 1, j * spacing[1] + origin[1]);
      vertList.setValue(((yStride + k) * 3) + 2, 0);
    }
  }

  for(uint64 j = 0; j < dims[1]; j++)
  {
    usize yStride = j * dims[0];
    for(uint64 k = 0; k < dims[0] - 1; k++)
    {
      edgeList.setValue(((yStride + k) * 2) + 0, yStride + k);
      edgeList.setValue(((yStride + k) * 2) + 1, yStride + k + 1);
    }
  }

  usize offset = dims[1] * (dims[0] - 1);
  for(uint64 j = 0; j < dims[1] - 1; j++)
  {
    usize yStride = j * dims[0];
    for(uint64 k = 0; k < dims[0]; k++)
    {
      edgeList.setValue(((offset + yStride + k) * 2) + 0, yStride + k);
      edgeList.setValue(((offset + yStride + k) * 2) + 1, ((j + 1) * dims[0]) + k);
    }
  }
}

struct DBSCANFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const DBSCANInputValues* inputValues, const IDataArray& clusterArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
                      Int32Array& featureIds)
  {
    const auto& inputArray = dynamic_cast<const DataArray<T>&>(clusterArray).getDataStoreRef();
    if(inputArray.getNumberOfComponents() == 2)
    {
      MakeEdgeGeom(dataStructure, inputValues->Epsilon, inputArray, mask, (clusterArray.getName() + " grid"));

      GDCF<HyperGridBitMap2D, T> algorithm = GDCF<HyperGridBitMap2D, T>(inputArray, inputValues->Epsilon, mask, inputValues->DistanceMetric);

      algorithm.cluster(inputValues->MinPoints, static_cast<DBSCAN::ParseOrder>(inputValues->ParseOrder), inputValues->Seed);

      algorithm.label(featureIds.getDataStoreRef());
    }
    else if(inputArray.getNumberOfComponents() == 3)
    {
      GDCF<HyperGridBitMap3D, T> algorithm = GDCF<HyperGridBitMap3D, T>(inputArray, inputValues->Epsilon, mask, inputValues->DistanceMetric);

      algorithm.cluster(inputValues->MinPoints, static_cast<DBSCAN::ParseOrder>(inputValues->ParseOrder), inputValues->Seed);

      algorithm.label(featureIds.getDataStoreRef());
    }
    else
    {
      return MakeErrorResult(-54060, "Input components invalid. Only 2 or 3 accepted.");
    }

    return {};
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

  ExecuteDataFunction(DBSCANFunctor{}, clusteringArray.getDataType(), m_DataStructure, m_InputValues, clusteringArray, maskCompare, featureIds);

  messageHelper.sendMessage("Resizing Clustering Attribute Matrix...");
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(AttributeMatrix::ShapeType{static_cast<usize>(maxCluster + 1)});

  return {};
}
