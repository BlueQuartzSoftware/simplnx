#include "DBSCANScanline.hpp"

#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <fmt/format.h>

#include <nonstd/span.hpp>

using namespace nx::core;

// =============================================================================
// DBSCANScanline — Out-of-Core (OOC) Algorithm
//
// This file implements the out-of-core (Scanline) variant of DBSCAN.
// It is selected by DispatchAlgorithm when any input array uses chunked on-disk
// storage (e.g., ZarrStore / HDF5 chunked store).
//
// PROBLEM:
//   The DBSCAN algorithm requires multiple passes over the input array during
//   grid construction (bounds, binning, filling) and random access to arbitrary
//   tuple indices during canMerge distance checks. When data is stored out-of-core,
//   each operator[] call may trigger a chunk load/decompress/evict cycle. The grid
//   construction passes iterate over all N tuples 2-3 times, and canMerge checks
//   access random positions in the array, making both phases vulnerable to chunk
//   thrashing.
//
// SOLUTION:
//   1. Grid construction uses 64K-tuple chunk reads via copyIntoBuffer(). Each
//      of the 2-3 passes reads the input array sequentially in chunks, processing
//      all tuples in each chunk before moving to the next. This converts N per-
//      element random accesses into N/65536 sequential bulk reads.
//
//   2. canMerge uses readGridCellCoords() to bulk-read all coordinate data for
//      each grid cell into a local float32 buffer. The pairwise distance check
//      then operates entirely on in-memory data. Memory cost per canMerge call
//      is O(gridCellSize * dims), which is typically small (grid cells contain
//      just a handful of points in practice).
//
//   3. The clustering and labeling phases operate on the in-memory grid index
//      (gridVoxels, clusterForest), which is identical to the Direct variant.
//      Only grid construction and canMerge are modified for OOC.
//
// NOTE: The HyperGridBitMap constructors in this file do NOT accept a
//   MessageHelper parameter (unlike the Direct variant) because they were
//   written to minimize the parameter surface for the OOC path.
// =============================================================================

namespace
{
/**
 * Implementation derived from: https://yliu.site/pub/GDCF_PR2019.pdf
 *
 * OOC variant: uses chunked copyIntoBuffer bulk I/O for grid construction
 * and on-demand per-grid-cell reads for canMerge distance computation.
 */

struct GridBitMap
{
  std::vector<uint8> gridTable = {};
  usize numPositions = 0;
  usize rowLength = 0;
};

struct GridBitMapFactory
{
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

class HyperGridBitMap
{
public:
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
  HyperGridBitMap3D(const std::atomic_bool& shouldCancel, const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    const usize numTuples = inputArray.getNumberOfTuples();
    const usize numComps = inputArray.getNumberOfComponents();
    constexpr usize k_ChunkTuples = 65536;
    auto chunkBuf = std::make_unique<T[]>(k_ChunkTuples * numComps);

    // Load array bounds using chunked bulk I/O
    std::array<float32, 6> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN()};
    for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
      const usize count = endTup - startTup;
      inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

      for(usize local = 0; local < count; local++)
      {
        if(!mask->isTrue(startTup + local))
        {
          continue;
        }

        auto xVal = static_cast<float32>(chunkBuf[local * numComps + 0]);
        auto yVal = static_cast<float32>(chunkBuf[local * numComps + 1]);
        auto zVal = static_cast<float32>(chunkBuf[local * numComps + 2]);

        bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
        bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);
        bounds[2] = std::isnan(bounds[2]) ? zVal : std::min(bounds[2], zVal);

        bounds[3] = std::isnan(bounds[3]) ? xVal : std::max(bounds[3], xVal);
        bounds[4] = std::isnan(bounds[4]) ? yVal : std::max(bounds[4], yVal);
        bounds[5] = std::isnan(bounds[5]) ? zVal : std::max(bounds[5], zVal);
      }
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

    // Fill the BitMap
    {
      std::vector<std::array<usize, 3>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // Find num grid cells - chunked bulk I/O pass
        for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
          const usize count = endTup - startTup;
          inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

          for(usize local = 0; local < count; local++)
          {
            const usize tup = startTup + local;
            if(!mask->isTrue(tup))
            {
              continue;
            }

            usize xPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 0]) - origin[0]) / spacing[0]);
            usize yPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 1]) - origin[1]) / spacing[1]);
            usize zPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 2]) - origin[2]) / spacing[2]);

            usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;
            grids[bin] = true;
          }
        }
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
        // Fill grid cells - chunked bulk I/O pass
        for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
          const usize count = endTup - startTup;
          inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

          for(usize local = 0; local < count; local++)
          {
            const usize tup = startTup + local;
            if(!mask->isTrue(tup))
            {
              continue;
            }
            usize xPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 0]) - origin[0]) / spacing[0]);
            usize yPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 1]) - origin[1]) / spacing[1]);
            usize zPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 2]) - origin[2]) / spacing[2]);

            usize bin = (zPos * dims[1] * dims[0]) + (yPos * dims[0]) + xPos;
            gridVoxels[gridMap[bin]].push_back(tup);
          }
        }
      } // End of filling non-empty grids and positions vector

      // Pack down memory further
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

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

class HyperGridBitMap2D : public HyperGridBitMap
{
public:
  static constexpr float32 Dimensions = 2;

  GridBitMap xTable;
  GridBitMap yTable;

  HyperGridBitMap2D() = delete;

  template <typename T>
  HyperGridBitMap2D(const std::atomic_bool& shouldCancel, const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : HyperGridBitMap()
  {
    const usize numTuples = inputArray.getNumberOfTuples();
    const usize numComps = inputArray.getNumberOfComponents();
    constexpr usize k_ChunkTuples = 65536;
    auto chunkBuf = std::make_unique<T[]>(k_ChunkTuples * numComps);

    // Load array bounds using chunked bulk I/O
    std::array<float32, 4> bounds = {std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::quiet_NaN(),
                                     std::numeric_limits<float32>::quiet_NaN()};
    for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
      const usize count = endTup - startTup;
      inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

      for(usize local = 0; local < count; local++)
      {
        if(!mask->isTrue(startTup + local))
        {
          continue;
        }

        auto xVal = static_cast<float32>(chunkBuf[local * numComps + 0]);
        auto yVal = static_cast<float32>(chunkBuf[local * numComps + 1]);

        bounds[0] = std::isnan(bounds[0]) ? xVal : std::min(bounds[0], xVal);
        bounds[1] = std::isnan(bounds[1]) ? yVal : std::min(bounds[1], yVal);

        bounds[2] = std::isnan(bounds[2]) ? xVal : std::max(bounds[2], xVal);
        bounds[3] = std::isnan(bounds[3]) ? yVal : std::max(bounds[3], yVal);
      }
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

    // Fill the BitMap
    {
      std::vector<std::array<usize, 2>> positions = {};
      // Build a set of non-empty grids and temporarily store their positions
      {
        std::vector<bool> grids(std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>()), false);
        // Find num grid cells - chunked bulk I/O pass
        for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
          const usize count = endTup - startTup;
          inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

          for(usize local = 0; local < count; local++)
          {
            const usize tup = startTup + local;
            if(!mask->isTrue(tup))
            {
              continue;
            }

            usize xPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 0]) - origin[0]) / spacing[0]);
            usize yPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 1]) - origin[1]) / spacing[1]);

            usize bin = (yPos * dims[0]) + xPos;
            grids[bin] = true;
          }
        }

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
        // Fill grid cells - chunked bulk I/O pass
        for(usize startTup = 0; startTup < numTuples; startTup += k_ChunkTuples)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize endTup = std::min(startTup + k_ChunkTuples, numTuples);
          const usize count = endTup - startTup;
          inputArray.copyIntoBuffer(startTup * numComps, nonstd::span<T>(chunkBuf.get(), count * numComps));

          for(usize local = 0; local < count; local++)
          {
            const usize tup = startTup + local;
            if(!mask->isTrue(tup))
            {
              continue;
            }
            usize xPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 0]) - origin[0]) / spacing[0]);
            usize yPos = std::floor((static_cast<float32>(chunkBuf[local * numComps + 1]) - origin[1]) / spacing[1]);

            usize bin = (yPos * dims[0]) + xPos;
            gridVoxels[gridMap[bin]].push_back(tup);
          }
        }
      } // End of filling non-empty grids and positions vector

      // Pack down memory further
      for(auto& grid : gridVoxels)
      {
        grid.shrink_to_fit();
      }

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

template <class HGBMT>
concept IsHGBP = std::is_base_of_v<HyperGridBitMap, HGBMT>;

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

struct ClusterNode
{
  int32 clusterId = 0;
  usize parent = 0;
};

struct ClusterForest
{
  std::vector<ClusterNode> clusterForestNodes = {};

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

/**
 * @brief OOC GDCF: uses on-demand per-grid-cell reads for canMerge.
 */
template <IsHGBP HGBPT, typename T>
class GDCF
{
public:
  GDCF() = delete;
  GDCF(const std::atomic_bool& shouldCancel, const AbstractDataStore<T>& inputArray, float32 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
       ClusterUtilities::DistanceMetric distMetric)
  : hyperGridBitMap(HGBPT(shouldCancel, inputArray, epsilon, mask))
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputArray)
  , m_DistMetric(distMetric)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> cluster(usize minPoints, DBSCAN::ParseOrder parseOrder, std::mt19937_64::result_type seed = std::mt19937_64::default_seed)
  {
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
      return MakeWarningVoidResult(-85640, "No clusters detected - Consider reducing number of required points (`Minimum Points`) or increasing acceptable distance (`Epsilon`).");
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    switch(parseOrder)
    {
    case DBSCAN::ParseOrder::LowDensityFirst: {
      QuickSortGrids(coreGridIds, 0, coreGridIds.size() - 1);
      break;
    }
    case DBSCAN::ParseOrder::Random: {
      std::mt19937_64 gen(seed);
      std::uniform_real_distribution<float64> dist(0, 1);

      auto maxIdx = static_cast<float64>(coreGridIds.size() - 1);

      for(usize i = 1; i < coreGridIds.size(); i++)
      {
        auto r = static_cast<usize>(std::floor(dist(gen) * maxIdx));
        std::swap(coreGridIds[i], coreGridIds[r]);
      }

      break;
    }
    case DBSCAN::SeededRandom: {
      std::mt19937_64 gen(seed);
      std::uniform_real_distribution<float64> dist(0, 1);

      auto maxIdx = static_cast<float64>(coreGridIds.size() - 1);

      for(usize i = 1; i < coreGridIds.size(); i++)
      {
        auto r = static_cast<usize>(std::floor(dist(gen) * maxIdx));
        std::swap(coreGridIds[i], coreGridIds[r]);
      }

      break;
    }
    }

    if(m_ShouldCancel)
    {
      return {};
    }

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

    // Now determine if non-core grids are close enough to a cluster to be border else noise
    usize operations = 0;
    do
    {
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

  Result<> label(AbstractDataStore<int32>& fIdsDataStore)
  {
    if(clusterForest.clusterForestNodes.empty())
    {
      return MakeWarningVoidResult(-85640, "No clusters detected - Consider reducing number of required points (`Minimum Points`) or increasing acceptable distance (`Epsilon`).");
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

  /**
   * @brief Reads coordinate data for all points in a grid cell from the OOC store.
   *
   * Instead of random operator[] access to arbitrary tuple indices scattered across
   * the full input array (which would cause chunk thrashing), this method reads each
   * grid cell member's coordinates via single-tuple copyIntoBuffer() calls and
   * assembles them into a contiguous local buffer.
   *
   * Memory cost is O(gridCellSize * dims) per call, which is typically very small
   * (grid cells contain a handful of points in practice). The returned buffer is
   * used for all pairwise distance computations in canMerge, so the data is read
   * once and reused for every comparison.
   *
   * @param gridId Index of the grid cell whose member coordinates to read
   * @return Contiguous float32 buffer with coordinates for all grid cell members
   */
  std::vector<float32> readGridCellCoords(usize gridId) const
  {
    const auto& indices = hyperGridBitMap.gridVoxels[gridId];
    const usize dims = static_cast<usize>(HGBPT::Dimensions);
    std::vector<float32> coords(indices.size() * dims);
    auto tupleBuf = std::make_unique<T[]>(dims);
    for(usize i = 0; i < indices.size(); i++)
    {
      m_InputDataStore.copyIntoBuffer(indices[i] * dims, nonstd::span<T>(tupleBuf.get(), dims));
      for(usize d = 0; d < dims; d++)
      {
        coords[i * dims + d] = static_cast<float32>(tupleBuf[d]);
      }
    }
    return coords;
  }

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

    QuickSortGrids(sorted, begin, next);
    QuickSortGrids(sorted, next + 1, end);
  }

  // OOC path: read grid cell coords on-demand into O(gridCellSize) local buffers.
  // Both grid cells' coordinate data is read in full via readGridCellCoords(),
  // then all pairwise distances are computed entirely in memory. This avoids
  // random operator[] access to the full OOC input array, which would trigger
  // chunk load/evict cycles for every single distance computation.
  bool canMerge(usize pGridId, usize qGridId)
  {
    const usize dims = static_cast<usize>(HGBPT::Dimensions);
    auto pCoords = readGridCellCoords(pGridId);
    auto qCoords = readGridCellCoords(qGridId);

    for(usize p = 0; p < hyperGridBitMap.gridVoxels[pGridId].size(); p++)
    {
      for(usize q = 0; q < hyperGridBitMap.gridVoxels[qGridId].size(); q++)
      {
        float64 dist = ClusterUtilities::GetDistance(pCoords, dims * p, qCoords, dims * q, dims, m_DistMetric);
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
                      const std::atomic_bool& shouldCancel)
{
  AlgorithmT algorithm = AlgorithmT(shouldCancel, inputArray, inputValues->Epsilon, mask, inputValues->DistanceMetric);

  if(shouldCancel)
  {
    return {};
  }

  Result<> result = algorithm.cluster(inputValues->MinPoints, static_cast<DBSCAN::ParseOrder>(inputValues->ParseOrder), inputValues->Seed);
  if(result.invalid() || !result.warnings().empty())
  {
    return result;
  }

  if(shouldCancel)
  {
    return {};
  }

  return algorithm.label(featureIds.getDataStoreRef());
}

struct DBSCANScanlineFunctor
{
  template <typename T>
  Result<> operator()(const DBSCANInputValues* inputValues, const IDataArray& clusterArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, Int32Array& featureIds,
                      const std::atomic_bool& shouldCancel)
  {
    const auto& inputArray = dynamic_cast<const DataArray<T>&>(clusterArray).getDataStoreRef();
    if(inputArray.getNumberOfComponents() == 2)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap2D, T>, T>(inputValues, inputArray, mask, featureIds, shouldCancel);
    }
    else if(inputArray.getNumberOfComponents() == 3)
    {
      return RunAlgorithm<GDCF<HyperGridBitMap3D, T>, T>(inputValues, inputArray, mask, featureIds, shouldCancel);
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
DBSCANScanline::DBSCANScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCANScanline::~DBSCANScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief OOC DBSCAN execution using chunked copyIntoBuffer I/O.
 */
Result<> DBSCANScanline::operator()()
{
  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54060, message);
  }

  Result<> result = ExecuteDataFunction(DBSCANScanlineFunctor{}, clusteringArray.getDataType(), m_InputValues, clusteringArray, maskCompare, featureIds, m_ShouldCancel);
  if(result.invalid())
  {
    return result;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // OOC: find max cluster ID using chunked bulk I/O instead of std::max_element
  // on the OOC store (which would use per-element iterator access). Read featureIds
  // in 1M-element chunks and track the maximum across all chunks.
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = 0;
  {
    const usize totalSize = featureIdsDataStore.getSize();
    constexpr usize k_ChunkSize = 1000000;
    std::vector<int32> maxBuf(std::min(totalSize, k_ChunkSize));
    for(usize start = 0; start < totalSize; start += k_ChunkSize)
    {
      usize count = std::min(k_ChunkSize, totalSize - start);
      featureIdsDataStore.copyIntoBuffer(start, nonstd::span<int32>(maxBuf.data(), count));
      for(usize i = 0; i < count; i++)
      {
        maxCluster = std::max(maxCluster, maxBuf[i]);
      }
    }
  }
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(ShapeType{static_cast<usize>(maxCluster + 1)});

  return result;
}
