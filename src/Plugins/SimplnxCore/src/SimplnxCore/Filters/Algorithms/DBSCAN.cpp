#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
/**
 * Implementation derived from: https://yliu.site/pub/GDCF_PR2019.pdf
 * Citation:
 * Thapana Boonchoo, Xiang Ao, Yang Liu, Weizhong Zhao, Fuzhen Zhuang, Qing He,
 * Grid-based DBSCAN: Indexing and inference,
 * https://doi.org/10.1016/j.patcog.2019.01.034.
 *
 * Definitions:
 * - Core Grid - A grid that contains more than the minPoints
 * - Border Grid - A grid that contains less than the minPoints,
 * but is density-reachable from an existing cluster
 * - Noise Grid - A grid with less than minPoints, and is unreachable
 * from a valid cluster
 */

/**
 * @brief This object packs a sparse matrix into a vector of uint8s. It
 * represents a singular dimension and must be used in tandem with another
 * from each dimension in the input array.
 *
 * It stores a form of adjacency matrix that is utilized as a look up
 * table for Nearest Neighbor queries.
 */
struct GridBitMap
{
  std::vector<uint8> gridTable = {};
  usize numPositions = 0;

  // This value represents the number of bytes allocated
  // to each row in the map
  // Reason: stored to speed up indexing and access
  usize rowLength = 0;
};

/**
 * @brief This object contains a function for creating GridBitMaps that handles
 * all the setup for the object. The decision to make it a Factory object comes
 * from the need to assemble multiple depending on the dimensions of input.
 */
struct GridBitMapFactory
{
  /**
   * Note here we can pack it slightly tighter by not adding buffers at the end of each row (for grid counts not divisible by 8)
   * but this will make calculations more difficult and costly during neighbor search
   * At most this saves 7/8s of a byte per dimension worth of space for significant calculation
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

/**
 * @brief HyperGridBitMap is the superclass for two specializations of 2D and 3D. These
 * read an input array to define a relevant regular grid. It bins the values in the input
 * array into cells in the grid then compresses the stored grids to just the ones containing
 * points (gridVoxels). It then builds several psuedo-adjacency maps to preserve the spatial
 * relationship between grids along each dimension.
 */
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
  HyperGridBitMap3D(const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, const AbstractDataStore<T>& inputArray, float32 epsilon,
                    const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    ThrottledMessageHandler throttle(messageHandler);

    messageHandler.sendInfoMessage(" - Determining bounds...");
    // Load array bounds
    std::array<float32, 6> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      if(shouldCancel)
      {
        return;
      }

      throttle.updatePercent(" - Finding Bounds", i, inputArray.getNumberOfTuples());

      if(!mask->isTrue(i))
      {
        continue;
      }

      auto xVal = static_cast<float32>(inputArray.getValue((i * 3) + 0));
      auto yVal = static_cast<float32>(inputArray.getValue((i * 3) + 1));
      auto zVal = static_cast<float32>(inputArray.getValue((i * 3) + 2));

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);
      bounds[2] = std::isnan(bounds[2]) ? zVal : std::min(bounds[2], zVal);

      bounds[3] = std::isnan(bounds[3]) ? xVal : std::max(bounds[3], xVal);
      bounds[4] = std::isnan(bounds[4]) ? yVal : std::max(bounds[4], yVal);
      bounds[5] = std::isnan(bounds[5]) ? zVal : std::max(bounds[5], zVal);
    }

    // Every bound is still NaN only when no point passed the mask. Bailing out here leaves
    // gridVoxels empty, which cluster() reports as the "no clusters detected" warning. Falling
    // through instead would cast NaN to usize while computing dims, which is undefined behavior.
    if(std::isnan(bounds[0]))
    {
      messageHandler.sendInfoMessage(" - No active (unmasked) points were found in the input array; there is nothing to cluster.");
      return;
    }

    // Grid Info - DO NOT MODIFY - basis for algorithm
    float32 sideLength = epsilon / std::sqrt(Dimensions);
    std::array<float32, 3> spacing = {sideLength, sideLength, sideLength};

    float32 buffer = sideLength;
    std::array<float32, 3> origin = {};
    origin[0] = bounds[0] - buffer;
    origin[1] = bounds[1] - buffer;
    origin[2] = bounds[2] - buffer;

    std::array<usize, 3> dims = {};
    dims[0] = static_cast<usize>(((bounds[3] + buffer) - origin[0]) / spacing[0]) + 2;
    dims[1] = static_cast<usize>(((bounds[4] + buffer) - origin[1]) / spacing[1]) + 2;
    dims[2] = static_cast<usize>(((bounds[5] + buffer) - origin[2]) / spacing[2]) + 2;

    messageHandler.sendInfoMessage(" - Binning values into a regular grid...");
    // Fill the BitMap
    {
      std::vector<std::array<usize, 3>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        usize numTup = inputArray.getNumberOfTuples();
        // grids and gridMap below are both sized by total cell count (occupied + empty):
        // dims[0] * dims[1] * dims[2], where each dims[i] ~ (bounding box per-axis range) / (epsilon / sqrt(D)).
        // grids is bit-packed (1 bit/cell); gridMap is usize/cell — both are live simultaneously.
        // Small epsilon or active extreme outlier points on 3D data can make this allocation needlessly expensive.
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // Find num grid cells
        for(usize tup = 0; tup < numTup; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          throttle.updatePercent(" - Binning", tup, numTup * 2);

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

          grids[bin] = true;
        }

        messageHandler.sendInfoMessage(" - Compressing regular grid...");
        usize zSize = dims[1] * dims[0];
        usize ySize = dims[0];
        usize activeGridCount = 0;
        std::vector<usize> gridMap(grids.size());
        for(usize i = 0; i < grids.size(); i++)
        {
          if(grids[i])
          {
            gridMap[i] = activeGridCount;
            activeGridCount++;

            std::array<usize, 3> position = {}; // Trivially copyable
            position[2] = i / zSize;
            usize zRemdr = i % zSize; // Modern compilers will extract the result from previous instruction
            position[1] = zRemdr / ySize;
            position[0] = zRemdr % ySize; // Modern compilers will extract the result from previous instruction
            positions.push_back(position);
          }
        }

        gridVoxels = std::vector<std::vector<usize>>(activeGridCount, std::vector<usize>(0));
        // Fill grid cells
        for(usize tup = 0; tup < numTup; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          throttle.updatePercent(" - Binning", numTup + tup, numTup * 2);

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

          gridVoxels[gridMap[bin]].push_back(tup);
        }
      } // End of filling non-empty grids and positions vector

      // Pack down memory further (run outside block to clear mem faster)
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

      messageHandler.sendInfoMessage(" - Generating adjacency matrix for search...");
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

      if(shouldCancel)
      {
        return;
      }

      // Set up hyper bit map
      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());
      zTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), zSet.size());

      if(shouldCancel)
      {
        return;
      }

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
  HyperGridBitMap2D(const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, const AbstractDataStore<T>& inputArray, float32 epsilon,
                    const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    ThrottledMessageHandler throttle(messageHandler);

    messageHandler.sendInfoMessage(" - Determining bounds...");
    // Load array bounds
    std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN()};
    for(usize i = 0; i < inputArray.getNumberOfTuples(); i++)
    {
      if(shouldCancel)
      {
        return;
      }

      throttle.updatePercent(" - Finding Bounds", i, inputArray.getNumberOfTuples());

      if(!mask->isTrue(i))
      {
        continue;
      }

      // Determine the voxel
      auto xVal = static_cast<float32>(inputArray.getValue((i * 2) + 0));
      auto yVal = static_cast<float32>(inputArray.getValue((i * 2) + 1));

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

      bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
      bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
    }

    // Every bound is still NaN only when no point passed the mask. Bailing out here leaves
    // gridVoxels empty, which cluster() reports as the "no clusters detected" warning. Falling
    // through instead would cast NaN to usize while computing dims, which is undefined behavior.
    if(std::isnan(bounds[0]))
    {
      messageHandler.sendInfoMessage(" - No active (unmasked) points were found in the input array; there is nothing to cluster.");
      return;
    }

    // Grid Info - DO NOT MODIFY - basis for algorithm
    float32 sideLength = epsilon / std::sqrt(Dimensions);
    std::array<float32, 2> spacing = {sideLength, sideLength};

    float32 buffer = sideLength;
    std::array<float32, 2> origin = {};
    origin[0] = bounds[0] - buffer;
    origin[1] = bounds[1] - buffer;

    std::array<usize, 2> dims = {};
    dims[0] = static_cast<usize>(((bounds[2] + buffer) - origin[0]) / spacing[0]) + 2;
    dims[1] = static_cast<usize>(((bounds[3] + buffer) - origin[1]) / spacing[1]) + 2;

    messageHandler.sendInfoMessage(" - Binning values into a regular grid...");
    // Fill the BitMap
    {
      std::vector<std::array<usize, 2>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        usize numTup = inputArray.getNumberOfTuples();
        // grids and gridMap below are both sized by total cell count (occupied + empty):
        // dims[0] * dims[1], where each dims[i] ~ (bounding box per-axis range) / (epsilon / sqrt(D)).
        // grids is bit-packed (1 bit/cell); gridMap is usize/cell — both are live simultaneously.
        // Small epsilon or active extreme outlier points can make this allocation needlessly expensive.
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // Find num grid cells
        for(usize tup = 0; tup < numTup; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          throttle.updatePercent(" - Binning", tup, numTup * 2);

          if(!mask->isTrue(tup))
          {
            continue;
          }

          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfComponents();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;

          grids[bin] = true;
        }

        messageHandler.sendInfoMessage(" - Compressing regular grid...");

        usize ySize = dims[0];
        usize activeGridCount = 0;
        std::vector<usize> gridMap(grids.size());
        for(usize i = 0; i < grids.size(); i++)
        {
          if(grids[i])
          {
            gridMap[i] = activeGridCount;
            activeGridCount++;

            std::array<usize, 2> position = {}; // Trivially copyable
            position[1] = i / ySize;
            position[0] = i % ySize; // Modern compilers will extract the result from previous instruction
            positions.push_back(position);
          }
        }

        gridVoxels = std::vector<std::vector<usize>>(activeGridCount, std::vector<usize>(0));
        // Fill grid cells
        for(usize tup = 0; tup < numTup; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          throttle.updatePercent(" - Binning", numTup + tup, numTup * 2);

          if(!mask->isTrue(tup))
          {
            continue;
          }
          // Determine the voxel
          usize pointIdx = tup * inputArray.getNumberOfComponents();
          usize xPos = std::floor((inputArray.getValue(pointIdx + 0) - origin[0]) / spacing[0]);
          usize yPos = std::floor((inputArray.getValue(pointIdx + 1) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;

          gridVoxels[gridMap[bin]].push_back(tup);
        }
      } // End of filling non-empty grids and positions vector

      // Pack down memory further (run outside block to clear mem faster)
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

      messageHandler.sendInfoMessage(" - Generating adjacency matrix for search...");
      /**
       * This could be modified to 2 passes on the positions vector with custom predicates and ths std::sort function,
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

      if(shouldCancel)
      {
        return;
      }

      // Set up hyper bit map
      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());

      if(shouldCancel)
      {
        return;
      }

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

  /**
   * @brief Primes the cluster forest object
   * @param numGrids the total number of gridVoxels containing points (not just core grids)
   */
  void initialize(usize numGrids)
  {
    clusterForestNodes.resize(numGrids);

    for(usize i = 0; i < clusterForestNodes.size(); i++)
    {
      clusterForestNodes[i].parent = i;
      clusterForestNodes[i].clusterId = static_cast<int32>(i + 1);
    }
  }

  usize findClusterRoot(usize gridId) const
  {
    while(clusterForestNodes[gridId].parent != gridId)
    {
      gridId = clusterForestNodes[gridId].parent;
    }
    return gridId;
  }

  /**
   * @brief Checks if grids are already in the same cluster
   * Note: NO BOUNDS CHECKING
   * @param pGridId a valid grid id
   * @param qGridId a valid grid id
   * @return bool if true they are in the same cluster
   */
  bool infer(usize pGridId, usize qGridId) const
  {
    return findClusterRoot(pGridId) == findClusterRoot(qGridId);
  }

  /**
   * @brief This function merges every supplied grid into the cluster with the
   * lowest cluster id.
   *
   * Note: DO NOT PASS IN A BORDER GRID THAT HAS ITSELF AS THE PARENT. This will
   * collapse all your clusters into unlabeled category. Ids to border grids that
   * have a valid Core Grid parent are fine.
   *
   * The best way to avoid collapse is never make a border grid with itself as
   * the parent, the parent of another border grid
   * @param gridIds - a vector of ids representing grids with valid parents to be merged
   */
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
        clusterForestNodes[clusterIdx].parent = lowestClusterIdx;
      }
    }
  }
};

template <IsHGBP HGBPT, typename T>
class GDCF
{
public:
  GDCF() = delete;
  GDCF(const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
       ClusterUtilities::DistanceMetric distMetric)
  : hyperGridBitMap(HGBPT(shouldCancel, messageHandler, inputArray, epsilon, mask))
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputArray)
  , m_DistMetric(distMetric)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> cluster(usize minPoints, DBSCAN::ParseOrder parseOrder, std::mt19937_64::result_type seed = std::mt19937_64::default_seed)
  {
    m_MessageHandler.sendInfoMessage(" - Identifying core grids...");
    // Identify Core Grids
    std::vector<usize> coreGridIds = {};
    for(usize i = 0; i < hyperGridBitMap.gridVoxels.size(); i++)
    {
      if(hyperGridBitMap.gridVoxels[i].size() >= minPoints)
      {
        coreGridIds.push_back(i);
      }
    }
    if(coreGridIds.empty())
    {
      return MakeWarningVoidResult(-85640, "No clusters detected - If a mask is applied, verify that some points are unmasked. Otherwise, consider reducing the number of required points (`Minimum "
                                           "Points`) or increasing the acceptable distance (`Epsilon`).");
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    // Sort Grids to reduce bias
    m_MessageHandler.sendInfoMessage(" - Sorting grids according to supplied parse order...");
    switch(parseOrder)
    {
    case DBSCAN::ParseOrder::LowDensityFirst: {
      QuickSortGrids(coreGridIds, 0, coreGridIds.size() - 1);
      break;
    }
    // Random and SeededRandom shuffle identically; they differ only in where the seed comes
    // from, which the filter resolves before handing the seed to this method.
    case DBSCAN::ParseOrder::Random:
    case DBSCAN::ParseOrder::SeededRandom: {
      std::mt19937_64 gen(seed);
      std::shuffle(coreGridIds.begin(), coreGridIds.end(), gen);
      break;
    }
    default:
      return MakeErrorResult(-85642, fmt::format("Unrecognized ParseOrder value: {}.", to_underlying(parseOrder)));
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler.sendInfoMessage("Identifying Qualifying Independent Clusters:");
    ThrottledMessageHandler throttle(m_MessageHandler);
    clusterForest.initialize(hyperGridBitMap.gridVoxels.size());
    for(usize i = 0; i < coreGridIds.size(); i++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      throttle.updatePercent(" - Identifying clusters", i, coreGridIds.size());

      std::vector<usize> neighborGrids = NeighborGridQuery(coreGridIds[i], hyperGridBitMap);

      std::vector<usize> cluster = {};
      cluster.push_back(coreGridIds[i]);
      for(const usize gridId : neighborGrids)
      {
        // If true they are in the same cluster
        if(clusterForest.infer(coreGridIds[i], gridId))
        {
          continue;
        }

        // Check if a point in neighbor grid is density reachable
        if(canMerge(coreGridIds[i], gridId))
        {
          // Check if it's a border grid and check if its unvisited
          if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && clusterForest.clusterForestNodes[gridId].parent == gridId)
          {
            // Border grids can not be their own cluster, which means this
            // is unvisited currently so merge it into the current cluster
            clusterForest.clusterForestNodes[gridId].parent = coreGridIds[i];
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

    // Now determine if non-core grids are close enough to a cluster to be border else noise
    m_MessageHandler.sendInfoMessage("Expanding and Merging Applicable Clusters:");
    usize loop = 1;
    usize operations = 0;
    do
    {
      m_MessageHandler.sendInfoMessage(fmt::format(" - Beginning cluster expansion pass: {}...", loop++));
      operations = 0;
      for(usize i = 0; i < hyperGridBitMap.gridVoxels.size(); i++)
      {
        throttle.updatePercent(" - Expanding clusters", i, hyperGridBitMap.gridVoxels.size());
        if(m_ShouldCancel)
        {
          return {};
        }

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
                  if(clusterForest.clusterForestNodes[activeParent].clusterId < clusterForest.clusterForestNodes[neighborGridParent].clusterId)
                  {
                    clusterForest.clusterForestNodes[neighborGridParent].parent = activeParent;
                  }
                  else
                  {
                    // Infer returning false means that they can't have the same cluster id so must be greater than
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

    m_MessageHandler.sendInfoMessage(" - Cleaning up cluster identifiers...");
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

    return {};
  }

  bool forestBuilt() const
  {
    return !clusterForest.clusterForestNodes.empty();
  }

  Result<> label(AbstractDataStore<int32>& fIdsDataStore)
  {
    if(clusterForest.clusterForestNodes.empty())
    {
      return MakeWarningVoidResult(-85640, "No clusters detected - If a mask is applied, verify that some points are unmasked. Otherwise, consider reducing the number of required points (`Minimum "
                                           "Points`) or increasing the acceptable distance (`Epsilon`).");
    }

    ThrottledMessageHandler throttle(m_MessageHandler);
    // label
    fIdsDataStore.fill(0);
    for(usize gridIdx = 0; gridIdx < hyperGridBitMap.gridVoxels.size(); gridIdx++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      throttle.updatePercent(" - Labeling", gridIdx, hyperGridBitMap.gridVoxels.size());

      int32 featureId = clusterForest.clusterForestNodes[clusterForest.findClusterRoot(gridIdx)].clusterId;
      for(usize pointIdx : hyperGridBitMap.gridVoxels[gridIdx])
      {
        fIdsDataStore.setValue(pointIdx, featureId);
      }
    }

    return {};
  }

private:
  HGBPT hyperGridBitMap;

  ClusterForest clusterForest = {};

  float32 m_Epsilon;
  const AbstractDataStore<T>& m_InputDataStore;
  ClusterUtilities::DistanceMetric m_DistMetric;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // First-element pivot quicksort partition (two-pointer, Hoare-style).
  // Worst case O(n^2) if occupancy values are already sorted ascending — unlikely on real spatial data.
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
    // The two partitions are disjoint, so the order they are processed in does not change the
    // final ordering. Recursing into the smaller partition and looping on the larger one caps
    // stack depth at O(log n) instead of the O(n) a plain double recursion reaches when the
    // occupancy values are already sorted ascending.
    while(begin < end)
    {
      usize next = ProcessSection(sorted, begin, end);

      if((next - begin) < (end - (next + 1)))
      {
        QuickSortGrids(sorted, begin, next);
        begin = next + 1;
      }
      else
      {
        QuickSortGrids(sorted, next + 1, end);
        end = next;
      }
    }
  }

  bool canMerge(usize pGridId, usize qGridId) const
  {
    for(usize pPointId : hyperGridBitMap.gridVoxels[pGridId])
    {
      if(m_ShouldCancel)
      {
        return false;
      }
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

template <class AlgorithmT, typename T>
Result<> RunAlgorithm(const DBSCANInputValues* inputValues, const AbstractDataStore<T>& inputArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, Int32Array& featureIds,
                      const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  messageHandler.sendInfoMessage("Partitioning Input Data:");
  AlgorithmT algorithm = AlgorithmT(shouldCancel, messageHandler, inputArray, inputValues->Epsilon, mask, inputValues->DistanceMetric);

  if(shouldCancel)
  {
    return {};
  }

  messageHandler.sendInfoMessage("Clustering:");
  Result<> result = algorithm.cluster(inputValues->MinPoints, static_cast<DBSCAN::ParseOrder>(inputValues->ParseOrder), inputValues->Seed);
  if(result.invalid())
  {
    return result;
  }
  if(!algorithm.forestBuilt())
  {
    return result;
  }

  if(shouldCancel)
  {
    return {};
  }

  messageHandler.sendInfoMessage("Labeling:");
  return algorithm.label(featureIds.getDataStoreRef());
}

struct DBSCANFunctor
{
  template <typename T>
  Result<> operator()(const DBSCANInputValues* inputValues, const IDataArray& clusterArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, Int32Array& featureIds,
                      const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    const auto& inputArray = dynamic_cast<const DataArray<T>&>(clusterArray).getDataStoreRef();
    if(inputArray.getNumberOfComponents() == 2)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap2D, T>, T>(inputValues, inputArray, mask, featureIds, messageHandler, shouldCancel);
    }
    if(inputArray.getNumberOfComponents() == 3)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap3D, T>, T>(inputValues, inputArray, mask, featureIds, messageHandler, shouldCancel);
    }

    return MakeErrorResult(-54060, fmt::format("Input array has {} components but only 2 or 3 are accepted.", inputArray.getNumberOfComponents()));
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
Result<> DBSCAN::operator()()
{

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

  Result<> result = ExecuteDataFunction(DBSCANFunctor{}, clusteringArray.getDataType(), m_InputValues, clusteringArray, maskCompare, featureIds, m_MessageHandler, m_ShouldCancel);
  if(result.invalid())
  {
    return result;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  m_MessageHandler.sendInfoMessage("Resizing clustering Attribute Matrix:");
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  const int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAM).resizeTuples(ShapeType{static_cast<usize>(maxCluster + 1)});

  return result;
}
