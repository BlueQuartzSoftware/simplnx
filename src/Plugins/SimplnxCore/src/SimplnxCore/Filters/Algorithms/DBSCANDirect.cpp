#include "DBSCANDirect.hpp"

#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

using namespace nx::core;

/**
 * @file DBSCANDirect.cpp
 * @brief Implements the resident GDCF DBSCAN path.
 *
 * The implementation follows the grid-based DBSCAN method from Boonchoo et al.
 * (2019). Grid construction makes resident passes over coordinates. Cluster
 * merging makes random tuple reads. These accesses are efficient in memory but
 * can load and evict disk-backed chunks repeatedly. DBSCANScanline handles that
 * storage pattern with bounded sequential I/O.
 * @see https://yliu.site/pub/GDCF_PR2019.pdf
 */

namespace
{
/**
 * @class AllTrueMaskCompare
 * @brief Supplies an accepted mask when masking is disabled.
 *
 * The adapter avoids allocating one mask value for each coordinate tuple.
 */
class AllTrueMaskCompare final : public MaskCompareUtilities::MaskCompare
{
public:
  /**
   * @brief Accepts both tuples.
   * @param firstTuple Ignored first tuple index.
   * @param secondTuple Ignored second tuple index.
   * @return True.
   */
  bool bothTrue(usize firstTuple, usize secondTuple) const override
  {
    return true;
  }

  /**
   * @brief Rejects no tuple pair.
   * @param firstTuple Ignored first tuple index.
   * @param secondTuple Ignored second tuple index.
   * @return False.
   */
  bool bothFalse(usize firstTuple, usize secondTuple) const override
  {
    return false;
  }

  /**
   * @brief Accepts a tuple.
   * @param tupleIndex Ignored tuple index.
   * @return True.
   */
  bool isTrue(usize tupleIndex) const override
  {
    return true;
  }

  /**
   * @brief Ignores a mask update.
   * @param tupleIndex Ignored tuple index.
   * @param value Ignored mask value.
   *
   * The synthetic mask has no writable storage.
   */
  void setValue(usize tupleIndex, bool value) override
  {
  }

  /**
   * @brief Returns no stored tuples.
   * @return Zero.
   *
   * DBSCAN gets tuple bounds from the coordinate array.
   */
  usize getNumberOfTuples() const override
  {
    return 0;
  }

  /**
   * @brief Returns the logical mask component count.
   * @return One.
   */
  usize getNumberOfComponents() const override
  {
    return 1;
  }

  /**
   * @brief Returns no stored true values.
   * @return Zero.
   *
   * DBSCAN does not query a count for the synthetic unbounded mask.
   */
  usize countTrueValues() const override
  {
    return 0;
  }
};

/**
 * @struct GridBitMap
 * @brief Stores occupied-grid incidence for one coordinate axis.
 */
struct GridBitMap
{
  std::vector<uint8> gridTable = {}; // Bit-packed grid incidence rows.
  usize numPositions = 0;            // Number of positions on the indexed axis.
  usize rowLength = 0;               // Bytes in one axis-position row.
};

/**
 * @struct GridBitMapFactory
 * @brief Creates zeroed axis-incidence tables.
 */
struct GridBitMapFactory
{
  /**
   * @brief Creates one packed axis-incidence table.
   * @param numGrids Number of occupied grids represented by table bits.
   * @param numPositons Number of positions on the indexed axis.
   * @return Zeroed table with one bit for each grid and position pair.
   */
  static GridBitMap createGridBitMap(usize numGrids, usize numPositons)
  {
    GridBitMap gridBitMap = {};

    usize bitPackSize = numGrids / 8;
    bitPackSize += static_cast<usize>((numGrids % 8 > 0));

    gridBitMap.numPositions = numPositons;
    gridBitMap.rowLength = bitPackSize;
    gridBitMap.gridTable.resize(bitPackSize * numPositons);

    return gridBitMap;
  }
};

/**
 * @class HyperGridBitMap
 * @brief Stores resident point membership for occupied grids.
 */
class HyperGridBitMap
{
public:
  std::vector<std::vector<usize>> gridVoxels = {}; // Maps each occupied grid to its source tuple indices.

protected:
  /**
   * @brief Creates empty resident grid membership.
   */
  HyperGridBitMap() = default;
};

/**
 * @class HyperGridBitMap3D
 * @brief Builds the resident three-dimensional DBSCAN grid index.
 *
 * Axis incidence tables find neighboring occupied grids without scanning the
 * complete regular-grid bounding box.
 */
class HyperGridBitMap3D : public HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 3; // Number of coordinate components.

  GridBitMap xTable; // Occupied-grid incidence by X position.
  GridBitMap yTable; // Occupied-grid incidence by Y position.
  GridBitMap zTable; // Occupied-grid incidence by Z position.

  HyperGridBitMap3D() = delete;

  /**
   * @brief Builds grid membership and axis incidence tables from accepted 3D points.
   * @tparam T Coordinate value type.
   * @param shouldCancel Cancellation flag.
   * @param messageHelper Sends progress messages.
   * @param inputArray Resident coordinate store.
   * @param epsilon DBSCAN neighborhood radius.
   * @param mask Selects coordinate tuples.
   *
   * The direct path makes per-tuple reads because its caller selects resident
   * storage. It stops construction when shouldCancel is set.
   */
  template <typename T>
  HyperGridBitMap3D(const std::atomic_bool& shouldCancel, MessageHelper& messageHelper, const AbstractDataStore<T>& inputArray, float32 epsilon,
                    const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    const usize numTuples = inputArray.getNumberOfTuples();
    const usize numComps = inputArray.getNumberOfComponents();

    messageHelper.sendMessage(" - Determining bounds...");
    // Resident tuple reads find the coordinate bounds without a bulk buffer.
    std::array<float32, 6> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN()};
    for(usize tup = 0; tup < numTuples; tup++)
    {
      if(shouldCancel)
      {
        return;
      }

      if(!mask->isTrue(tup))
      {
        continue;
      }

      auto xVal = static_cast<float32>(inputArray[tup * numComps + 0]);
      auto yVal = static_cast<float32>(inputArray[tup * numComps + 1]);
      auto zVal = static_cast<float32>(inputArray[tup * numComps + 2]);

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);
      bounds[2] = std::isnan(bounds[2]) ? zVal : std::min(bounds[2], zVal);

      bounds[3] = std::isnan(bounds[3]) ? xVal : std::max(bounds[3], xVal);
      bounds[4] = std::isnan(bounds[4]) ? yVal : std::max(bounds[4], yVal);
      bounds[5] = std::isnan(bounds[5]) ? zVal : std::max(bounds[5], zVal);
    }

    if(std::isnan(bounds[0]))
    {
      messageHelper.sendMessage(" - No active (unmasked) points were found in the input array; there is nothing to cluster.");
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

    messageHelper.sendMessage(" - Binning values into a regular grid...");
    {
      std::vector<std::array<usize, 3>> positions = {};
      {
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // The temporary regular-grid map identifies occupied cells for compression.
        for(usize tup = 0; tup < numTuples; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          if(!mask->isTrue(tup))
          {
            continue;
          }

          usize xPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 0]) - origin[0]) / spacing[0]);
          usize yPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 1]) - origin[1]) / spacing[1]);
          usize zPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 2]) - origin[2]) / spacing[2]);

          usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;
          grids[bin] = true;
        }

        messageHelper.sendMessage(" - Compressing regular grid...");
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

            std::array<usize, 3> position = {};
            position[2] = i / zSize;
            usize zRemdr = i % zSize;
            position[1] = zRemdr / ySize;
            position[0] = zRemdr % ySize;
            positions.push_back(position);
          }
        }

        gridVoxels = std::vector<std::vector<usize>>(activeGridCount, std::vector<usize>(0));
        for(usize tup = 0; tup < numTuples; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          if(!mask->isTrue(tup))
          {
            continue;
          }

          usize xPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 0]) - origin[0]) / spacing[0]);
          usize yPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 1]) - origin[1]) / spacing[1]);
          usize zPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 2]) - origin[2]) / spacing[2]);

          usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;
          gridVoxels[gridMap[bin]].push_back(tup);
        }
      }

      // Release construction capacity before the clustering phase retains this index.
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

      messageHelper.sendMessage(" - Generating adjacency matrix for search...");
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

      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());
      zTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), zSet.size());

      if(shouldCancel)
      {
        return;
      }

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

/**
 * @class HyperGridBitMap2D
 * @brief Builds the resident two-dimensional DBSCAN grid index.
 */
class HyperGridBitMap2D : public HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 2; // Number of coordinate components.

  GridBitMap xTable; // Occupied-grid incidence by X position.
  GridBitMap yTable; // Occupied-grid incidence by Y position.

  HyperGridBitMap2D() = delete;

  /**
   * @brief Builds grid membership and axis incidence tables from accepted 2D points.
   * @tparam T Coordinate value type.
   * @param shouldCancel Cancellation flag.
   * @param messageHelper Sends progress messages.
   * @param inputArray Resident coordinate store.
   * @param epsilon DBSCAN neighborhood radius.
   * @param mask Selects coordinate tuples.
   *
   * The direct path makes per-tuple reads because its caller selects resident
   * storage. It stops construction when shouldCancel is set.
   */
  template <typename T>
  HyperGridBitMap2D(const std::atomic_bool& shouldCancel, MessageHelper& messageHelper, const AbstractDataStore<T>& inputArray, float32 epsilon,
                    const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    const usize numTuples = inputArray.getNumberOfTuples();
    const usize numComps = inputArray.getNumberOfComponents();

    messageHelper.sendMessage(" - Determining bounds...");
    // Load array bounds using direct per-element access
    std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN()};
    for(usize tup = 0; tup < numTuples; tup++)
    {
      if(shouldCancel)
      {
        return;
      }

      if(!mask->isTrue(tup))
      {
        continue;
      }

      auto xVal = static_cast<float32>(inputArray[tup * numComps + 0]);
      auto yVal = static_cast<float32>(inputArray[tup * numComps + 1]);

      bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
      bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

      bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
      bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
    }

    if(std::isnan(bounds[0]))
    {
      messageHelper.sendMessage(" - No active (unmasked) points were found in the input array; there is nothing to cluster.");
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

    messageHelper.sendMessage(" - Binning values into a regular grid...");
    {
      std::vector<std::array<usize, 2>> positions = {};
      {
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // The temporary regular-grid map identifies occupied cells for compression.
        for(usize tup = 0; tup < numTuples; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          if(!mask->isTrue(tup))
          {
            continue;
          }

          usize xPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 0]) - origin[0]) / spacing[0]);
          usize yPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 1]) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;
          grids[bin] = true;
        }

        messageHelper.sendMessage(" - Compressing regular grid...");

        usize ySize = dims[0];
        usize activeGridCount = 0;
        std::vector<usize> gridMap(grids.size());
        for(usize i = 0; i < grids.size(); i++)
        {
          if(grids[i])
          {
            gridMap[i] = activeGridCount;
            activeGridCount++;

            std::array<usize, 2> position = {};
            position[1] = i / ySize;
            position[0] = i % ySize;
            positions.push_back(position);
          }
        }

        gridVoxels = std::vector<std::vector<usize>>(activeGridCount, std::vector<usize>(0));
        for(usize tup = 0; tup < numTuples; tup++)
        {
          if(shouldCancel)
          {
            return;
          }

          if(!mask->isTrue(tup))
          {
            continue;
          }

          usize xPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 0]) - origin[0]) / spacing[0]);
          usize yPos = std::floor((static_cast<float32>(inputArray[tup * numComps + 1]) - origin[1]) / spacing[1]);

          usize bin = (yPos * dims[0]) + xPos;
          gridVoxels[gridMap[bin]].push_back(tup);
        }
      }

      // Release construction capacity before the clustering phase retains this index.
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

      messageHelper.sendMessage(" - Generating adjacency matrix for search...");
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

      xTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), xSet.size());
      yTable = GridBitMapFactory::createGridBitMap(gridVoxels.size(), ySet.size());

      if(shouldCancel)
      {
        return;
      }

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

/**
 * @brief Intersects a candidate mask with occupied grids near one axis position.
 * @param outputGridMask Candidate grid bits updated in place.
 * @param searchSpace Axis positions to inspect on each side.
 * @param targetPosition Center axis position.
 * @param selectedTable Axis-incidence table.
 */
void SearchTablePositions(std::vector<uint8>& outputGridMask, usize searchSpace, usize targetPosition, const GridBitMap& selectedTable)
{
  std::vector<uint8> tempGridMask(selectedTable.rowLength, 0);

  usize xStart = (targetPosition < searchSpace) ? 0 : targetPosition - searchSpace;
  usize xEnd = (targetPosition + searchSpace < selectedTable.numPositions) ? targetPosition + searchSpace + 1 : selectedTable.numPositions;

  for(usize pos = xStart; pos < xEnd; pos++)
  {
    for(usize i = 0; i < selectedTable.rowLength; i++)
    {
      tempGridMask[i] = tempGridMask[i] | selectedTable.gridTable[(pos * selectedTable.rowLength) + i];
    }
  }

  for(usize i = 0; i < selectedTable.rowLength; i++)
  {
    outputGridMask[i] = tempGridMask[i] & outputGridMask[i];
  }
}

/**
 * @concept IsHGBP
 * @brief Constrains a resident two- or three-dimensional grid index.
 * @tparam HGBMT Candidate grid-index type.
 */
template <class HGBMT>
concept IsHGBP = std::is_base_of_v<HyperGridBitMap, HGBMT>;

/**
 * @brief Returns occupied grids within one DBSCAN search neighborhood.
 * @tparam HGBPT Resident grid-index type.
 * @param targetGridId Grid that defines the neighborhood.
 * @param hyperGridBitMap Resident axis-incidence tables.
 * @return Occupied neighboring grid identifiers.
 *
 * The bitwise intersection removes empty regular-grid positions before pairwise
 * distance checks.
 */
template <IsHGBP HGBPT>
std::vector<usize> NeighborGridQuery(usize targetGridId, const HGBPT& hyperGridBitMap)
{
  usize searchSpace = std::ceil(std::sqrt(HGBPT::Dimensions));

  std::vector<usize> neighborGridIds = {};

  std::vector<uint8> finalGridMask(hyperGridBitMap.xTable.rowLength, std::numeric_limits<uint8>::max());

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

/**
 * @struct ClusterNode
 * @brief Stores one grid's union parent and feature identifier.
 */
struct ClusterNode
{
  int32 clusterId = 0; // Feature identifier for a root grid.
  usize parent = 0;    // Parent grid identifier in the union forest.
};

/**
 * @struct ClusterForest
 * @brief Merges density-connected resident grids.
 */
struct ClusterForest
{
  std::vector<ClusterNode> clusterForestNodes = {}; // State for every occupied grid.

  /**
   * @brief Creates one singleton cluster node for each occupied grid.
   * @param numGrids Number of occupied grids.
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

  /** @brief Finds the canonical cluster root without recursive stack growth. */
  usize findClusterRoot(usize gridId) const
  {
    while(clusterForestNodes[gridId].parent != gridId)
    {
      gridId = clusterForestNodes[gridId].parent;
    }
    return gridId;
  }

  /** @brief Returns whether two grids already resolve to the same cluster. */
  bool infer(usize pGridId, usize qGridId) const
  {
    return findClusterRoot(pGridId) == findClusterRoot(qGridId);
  }

  /**
   * @brief Merges grids under the root with the lowest feature identifier.
   * @param gridIds Grids to merge.
   *
   * The selected root keeps feature numbering deterministic across merge order.
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

/**
 * @class GDCF
 * @brief Performs resident grid-based DBSCAN clustering.
 * @tparam HGBPT Resident grid-index type.
 * @tparam T Coordinate value type.
 *
 * Random tuple comparisons are efficient in resident storage. DBSCANScanline
 * replaces these reads with bounded tiles for disk-backed stores.
 */
template <IsHGBP HGBPT, typename T>
class GDCF
{
public:
  GDCF() = delete;

  /**
   * @brief Creates a resident GDCF clustering operation.
   * @param shouldCancel Cancellation flag.
   * @param messageHelper Sends progress messages.
   * @param inputArray Resident coordinate store.
   * @param epsilon DBSCAN neighborhood radius.
   * @param mask Selects coordinate tuples.
   * @param distMetric Distance metric for pairwise tests.
   */
  GDCF(const std::atomic_bool& shouldCancel, MessageHelper& messageHelper, const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
       ClusterUtilities::DistanceMetric distMetric)
  : hyperGridBitMap(HGBPT(shouldCancel, messageHelper, inputArray, epsilon, mask))
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputArray)
  , m_DistMetric(distMetric)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHelper(messageHelper)
  {
  }

  /**
   * @brief Forms core clusters and assigns connected border grids.
   * @param minPoints Minimum points that make a grid core.
   * @param parseOrder Order for core-grid processing.
   * @param seed Random generator seed for random orders.
   * @return Warning when no grid is core, or success otherwise.
   *
   * The method checks cancellation between grid passes.
   * Lowest-root union makes core-grid merges independent of merge order. Parse order can still control border-grid attachment.
   */
  Result<> cluster(usize minPoints, DBSCAN::ParseOrder parseOrder, std::mt19937_64::result_type seed = std::mt19937_64::default_seed)
  {
    m_MessageHelper.sendMessage(" - Identifying core grids...");
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

    m_MessageHelper.sendMessage(" - Sorting grids according to supplied parse order...");
    switch(parseOrder)
    {
    case DBSCAN::ParseOrder::LowDensityFirst: {
      QuickSortGrids(coreGridIds, 0, coreGridIds.size() - 1);
      break;
    }
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

    m_MessageHelper.sendMessage("Identifying Qualifying Independent Clusters:");
    clusterForest.initialize(hyperGridBitMap.gridVoxels.size());
    for(usize i = 0; i < coreGridIds.size(); i++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      std::vector<usize> neighborGrids = NeighborGridQuery(coreGridIds[i], hyperGridBitMap);

      std::vector<usize> cluster = {};
      cluster.push_back(coreGridIds[i]);
      for(const usize gridId : neighborGrids)
      {
        if(clusterForest.infer(coreGridIds[i], gridId))
        {
          continue;
        }

        if(canMerge(coreGridIds[i], gridId))
        {
          if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && clusterForest.clusterForestNodes[gridId].parent == gridId)
          {
            clusterForest.clusterForestNodes[gridId].parent = coreGridIds[i];
          }
          else
          {
            cluster.push_back(gridId);
          }
        }
      }

      clusterForest.mergeLRC(cluster);
    }

    // Non-core grids attach to connected clusters or remain labeled as noise.
    m_MessageHelper.sendMessage("Expanding and Merging Applicable Clusters:");
    usize loop = 1;
    usize operations = 0;
    do
    {
      m_MessageHelper.sendMessage(fmt::format(" - Beginning cluster expansion pass: {}...", loop++));
      operations = 0;
      for(usize i = 0; i < hyperGridBitMap.gridVoxels.size(); i++)
      {
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
              if(activeParent == i)
              {
                if(hyperGridBitMap.gridVoxels[gridId].size() < minPoints && neighborGridParent == gridId)
                {
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

    m_MessageHelper.sendMessage(" - Cleaning up cluster identifiers...");
    std::vector<usize> clusters = {};
    for(usize i = 0; i < clusterForest.clusterForestNodes.size(); i++)
    {
      if(clusterForest.clusterForestNodes[i].parent == i)
      {
        if(hyperGridBitMap.gridVoxels[i].size() >= minPoints)
        {
          clusters.push_back(i);
        }
        else
        {
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

  /** @brief Returns true when clustering created resident forest state. */
  bool forestBuilt() const
  {
    return !clusterForest.clusterForestNodes.empty();
  }

  /** @brief Writes each point's final cluster ID, leaving masked/outlier points at zero. */
  Result<> label(AbstractDataStore<int32>& fIdsDataStore)
  {
    if(clusterForest.clusterForestNodes.empty())
    {
      return MakeWarningVoidResult(-85640, "No clusters detected - If a mask is applied, verify that some points are unmasked. Otherwise, consider reducing the number of required points (`Minimum "
                                           "Points`) or increasing the acceptable distance (`Epsilon`).");
    }

    fIdsDataStore.fill(0);
    for(usize gridIdx = 0; gridIdx < hyperGridBitMap.gridVoxels.size(); gridIdx++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

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

  float32 m_Epsilon = 0.0f;
  const AbstractDataStore<T>& m_InputDataStore;
  ClusterUtilities::DistanceMetric m_DistMetric;
  const std::atomic_bool& m_ShouldCancel;
  MessageHelper& m_MessageHelper;

  /**
   * @brief Partitions one density-sort range with Hoare's method.
   * @param sorted Grid identifiers ordered in place.
   * @param begin First index in the partition.
   * @param end Last index in the partition.
   * @return Last index in the lower partition.
   *
   * Hoare partitioning sorts in place and avoids another core-grid buffer.
   */
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

  /**
   * @brief Sorts grid identifiers by population for LowDensityFirst traversal.
   * @param sorted Grid identifiers ordered in place.
   * @param begin First index in the sort range.
   * @param end Last index in the sort range.
   */
  void QuickSortGrids(std::vector<usize>& sorted, usize begin, usize end) const
  {
    while(begin < end)
    {
      const usize next = ProcessSection(sorted, begin, end);
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

  // In-core path: direct random access via operator[] is fast.
  // For each pair of points (one from each grid cell), compute the distance
  // and return true as soon as any pair is within epsilon. With in-memory data,
  // operator[] is a pointer dereference, so random access to arbitrary tuple
  // indices is efficient. For OOC data, see DBSCANScanline's readGridCellCoords().
  /** @brief Tests whether any point pair across two candidate grids lies within epsilon. */
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

/**
 * @brief Runs one resident GDCF specialization through clustering and labeling.
 * @tparam AlgorithmT GDCF specialization type.
 * @tparam T Coordinate value type.
 * @param inputValues DBSCAN parameters.
 * @param inputArray Resident coordinate store.
 * @param mask Selects coordinate tuples.
 * @param featureIds Receives final feature identifiers.
 * @param messageHelper Sends progress messages.
 * @param shouldCancel Cancellation flag.
 * @return Warning, error, or success from clustering and labeling.
 */
template <class AlgorithmT, typename T>
Result<> RunAlgorithm(const DBSCANInputValues* inputValues, const AbstractDataStore<T>& inputArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, Int32Array& featureIds,
                      MessageHelper& messageHelper, const std::atomic_bool& shouldCancel)
{
  messageHelper.sendMessage("Partitioning Input Data:");
  AlgorithmT algorithm = AlgorithmT(shouldCancel, messageHelper, inputArray, inputValues->Epsilon, mask, inputValues->DistanceMetric);

  if(shouldCancel)
  {
    return {};
  }

  messageHelper.sendMessage("Clustering:");
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

  messageHelper.sendMessage("Labeling:");
  return algorithm.label(featureIds.getDataStoreRef());
}

/**
 * @struct DBSCANDirectFunctor
 * @brief Dispatches resident DBSCAN by coordinate type and dimension.
 */
struct DBSCANDirectFunctor
{
  /**
   * @brief Selects a two- or three-dimensional GDCF specialization.
   * @tparam T Coordinate value type.
   * @param inputValues DBSCAN parameters.
   * @param clusterArray Coordinate array.
   * @param mask Selects coordinate tuples.
   * @param featureIds Receives final feature identifiers.
   * @param messageHelper Sends progress messages.
   * @param shouldCancel Cancellation flag.
   * @return Result from the selected specialization, or an error for another component count.
   */
  template <typename T>
  Result<> operator()(const DBSCANInputValues* inputValues, const IDataArray& clusterArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, Int32Array& featureIds,
                      MessageHelper& messageHelper, const std::atomic_bool& shouldCancel)
  {
    const auto& inputArray = dynamic_cast<const DataArray<T>&>(clusterArray).getDataStoreRef();
    if(inputArray.getNumberOfComponents() == 2)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap2D, T>, T>(inputValues, inputArray, mask, featureIds, messageHelper, shouldCancel);
    }
    else if(inputArray.getNumberOfComponents() == 3)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap3D, T>, T>(inputValues, inputArray, mask, featureIds, messageHelper, shouldCancel);
    }
    else
    {
      return MakeErrorResult(-54060, fmt::format("Input array has {} components but only 2 or 3 are accepted.", inputArray.getNumberOfComponents()));
    }
  }
};
} // namespace

DBSCANDirect::DBSCANDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

DBSCANDirect::~DBSCANDirect() noexcept = default;

Result<> DBSCANDirect::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);

  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-54060, message);
    }
  }
  else
  {
    maskCompare = std::make_unique<AllTrueMaskCompare>();
  }

  Result<> result = ExecuteDataFunction(DBSCANDirectFunctor{}, clusteringArray.getDataType(), m_InputValues, clusteringArray, maskCompare, featureIds, messageHelper, m_ShouldCancel);
  if(result.invalid())
  {
    return result;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  messageHelper.sendMessage("Resizing clustering Attribute Matrix:");
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(ShapeType{static_cast<usize>(maxCluster + 1)});

  return result;
}
