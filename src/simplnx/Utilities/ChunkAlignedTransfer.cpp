#include "ChunkAlignedTransfer.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <stdexcept>

namespace nx::core
{
namespace
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
std::atomic<usize> g_AlignedCopyTargetBytesOverride{0}; // Test piece byte target; 0 selects k_AlignedCopyTargetBytes.
#endif

/**
 * @brief Tells if a chunk grid can classify ranges of a tuple shape.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension.
 * @return True if both ranks are equal and nonzero and no chunk extent is 0.
 */
bool IsValidGrid(const ShapeType& tupleShape, const ShapeType& chunkShape)
{
  return !tupleShape.empty() && chunkShape.size() == tupleShape.size() && std::find(chunkShape.begin(), chunkShape.end(), usize{0}) == chunkShape.end();
}

/**
 * @brief Computes the tuple count of a shape.
 * @param tupleShape Row-major tuple shape.
 * @return Product of the extents, or 0 if an extent is 0.
 * @throws std::overflow_error If the product overflows usize.
 */
usize CheckedTupleCount(const ShapeType& tupleShape)
{
  if(std::find(tupleShape.begin(), tupleShape.end(), usize{0}) != tupleShape.end())
  {
    return 0;
  }
  usize count = 1;
  for(const usize extent : tupleShape)
  {
    if(extent > std::numeric_limits<usize>::max() / count)
    {
      throw std::overflow_error("Chunk-aligned transfer: the tuple count of the tuple shape overflows usize.");
    }
    count *= extent;
  }
  return count;
}

/**
 * @brief Rejects a tuple range that extends past the last tuple of a shape.
 * @param tupleShape Row-major tuple shape.
 * @param startTuple First flat tuple of the range.
 * @param tupleCount Number of tuples in the range.
 * @throws std::out_of_range If the range extends past the last tuple.
 */
void ValidateTupleRange(const ShapeType& tupleShape, usize startTuple, usize tupleCount)
{
  const usize numTuples = CheckedTupleCount(tupleShape);
  if(startTuple > numTuples || tupleCount > numTuples - startTuple)
  {
    throw std::out_of_range("Chunk-aligned transfer: the tuple range [" + std::to_string(startTuple) + ", +" + std::to_string(tupleCount) + ") extends past the tuple count (" +
                            std::to_string(numTuples) + ").");
  }
}

/**
 * @brief Converts a flat tuple index to a row-major position.
 * @param flatTuple Flat tuple index. It must be less than the tuple count of the shape.
 * @param tupleShape Row-major tuple shape with no zero extent.
 * @return Index in each dimension.
 */
ShapeType FlatToPosition(usize flatTuple, const ShapeType& tupleShape)
{
  ShapeType position(tupleShape.size());
  for(usize dimension = tupleShape.size(); dimension-- > 0;)
  {
    position[dimension] = flatTuple % tupleShape[dimension];
    flatTuple /= tupleShape[dimension];
  }
  return position;
}

/**
 * @brief Returns the extent of the chunk that holds one index, after the chunk is clamped to the dataset.
 * @param index Index in one dimension.
 * @param chunkExtent Chunk extent in that dimension. It must not be 0.
 * @param datasetExtent Tuple extent in that dimension.
 * @return Clamped chunk extent.
 */
usize ClampedChunkExtent(usize index, usize chunkExtent, usize datasetExtent)
{
  const usize chunkStart = (index / chunkExtent) * chunkExtent;
  return std::min(chunkExtent, datasetExtent - chunkStart);
}

/**
 * @struct GridGeometry
 * @brief Holds the precomputed values that the planner uses for one valid grid.
 */
struct GridGeometry
{
  /**
   * @brief Chunk extents clamped to the tuple extents.
   *
   * The clamp does not change alignment results. It keeps each product of a chunk extent and a stride at or below
   * the tuple count.
   */
  ShapeType chunkExtents;
  /**
   * @brief Tuples in one index of each dimension, that is, the product of all faster extents.
   */
  ShapeType strides;
};

/**
 * @brief Computes the clamped chunk extents and dimension strides of a valid grid.
 * @param tupleShape Row-major tuple shape with no zero extent.
 * @param chunkShape Valid chunk shape.
 * @return Grid geometry.
 */
GridGeometry MakeGridGeometry(const ShapeType& tupleShape, const ShapeType& chunkShape)
{
  const usize rank = tupleShape.size();
  GridGeometry geometry{ShapeType(rank), ShapeType(rank)};
  usize stride = 1;
  for(usize dimension = rank; dimension-- > 0;)
  {
    geometry.chunkExtents[dimension] = std::min(chunkShape[dimension], tupleShape[dimension]);
    geometry.strides[dimension] = stride;
    stride *= tupleShape[dimension];
  }
  return geometry;
}

/**
 * @brief Tells if an aligned range with changing dimension c can start at a position.
 * @param position Row-major start position.
 * @param tupleShape Row-major tuple shape.
 * @param geometry Geometry of the grid.
 * @param changingDimension Dimension c.
 * @return True if every faster index is 0 and the index in c is on a chunk boundary. Every slower dimension must
 * also have a clamped chunk extent of 1 at the position.
 */
bool IsAlignedStart(const ShapeType& position, const ShapeType& tupleShape, const GridGeometry& geometry, usize changingDimension)
{
  for(usize dimension = changingDimension + 1; dimension < tupleShape.size(); ++dimension)
  {
    if(position[dimension] != 0)
    {
      return false;
    }
  }
  if(position[changingDimension] % geometry.chunkExtents[changingDimension] != 0)
  {
    return false;
  }
  for(usize dimension = 0; dimension < changingDimension; ++dimension)
  {
    if(ClampedChunkExtent(position[dimension], geometry.chunkExtents[dimension], tupleShape[dimension]) != 1)
    {
      return false;
    }
  }
  return true;
}

/**
 * @brief Returns the next flat tuple after a start at which the index in c is on a chunk boundary and every faster
 * index is 0.
 * @param startTuple Flat tuple to search after.
 * @param tupleShape Row-major tuple shape.
 * @param geometry Geometry of the grid.
 * @param changingDimension Dimension c.
 * @return Candidate flat tuple. It can be equal to or larger than the tuple count.
 *
 * The candidate does not check the slower dimensions. The caller checks it with IsAlignedStart().
 */
usize NextAlignedStartCandidate(usize startTuple, const ShapeType& tupleShape, const GridGeometry& geometry, usize changingDimension)
{
  const usize blockTuples = tupleShape[changingDimension] * geometry.strides[changingDimension];
  const usize stepTuples = geometry.chunkExtents[changingDimension] * geometry.strides[changingDimension];
  const usize blockStart = startTuple - (startTuple % blockTuples);
  const usize offset = startTuple - blockStart;
  const usize nextOffset = offset - (offset % stepTuples) + stepTuples;
  return blockStart + std::min(nextOffset, blockTuples);
}
} // namespace

bool IsChunkAlignedTupleRange(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount)
{
  ValidateTupleRange(tupleShape, startTuple, tupleCount);
  if(tupleCount == 0 || !IsValidGrid(tupleShape, chunkShape))
  {
    return false;
  }

  const usize tupleDims = tupleShape.size();
  const ShapeType startNd = FlatToPosition(startTuple, tupleShape);
  const ShapeType endNd = FlatToPosition(startTuple + tupleCount - 1, tupleShape);
  usize changingDimension = tupleDims;
  for(usize dimension = 0; dimension < tupleDims; ++dimension)
  {
    if(startNd[dimension] != endNd[dimension])
    {
      changingDimension = dimension;
      break;
    }
  }

  // Every dimension that does not change must hold a one-index clamped chunk. For a single tuple that is all of them.
  for(usize dimension = 0; dimension < changingDimension; ++dimension)
  {
    if(ClampedChunkExtent(startNd[dimension], chunkShape[dimension], tupleShape[dimension]) != 1)
    {
      return false;
    }
  }
  if(changingDimension == tupleDims)
  {
    return true;
  }

  const usize changingChunk = chunkShape[changingDimension];
  const bool startsOnBoundary = startNd[changingDimension] % changingChunk == 0;
  const bool endsOnBoundary = (endNd[changingDimension] + 1) % changingChunk == 0 || endNd[changingDimension] == tupleShape[changingDimension] - 1;
  if(!startsOnBoundary || !endsOnBoundary)
  {
    return false;
  }
  for(usize dimension = changingDimension + 1; dimension < tupleDims; ++dimension)
  {
    if(startNd[dimension] != 0 || endNd[dimension] != tupleShape[dimension] - 1)
    {
      return false;
    }
  }
  return true;
}

usize ComputeNominalChunkAlignedUnitTuples(const ShapeType& tupleShape, const ShapeType& chunkShape)
{
  if(!IsValidGrid(tupleShape, chunkShape) || CheckedTupleCount(tupleShape) == 0)
  {
    return 0;
  }
  const GridGeometry geometry = MakeGridGeometry(tupleShape, chunkShape);
  usize changingDimension = 0;
  while(changingDimension + 1 < tupleShape.size() && geometry.chunkExtents[changingDimension] == 1)
  {
    ++changingDimension;
  }
  return geometry.chunkExtents[changingDimension] * geometry.strides[changingDimension];
}

ChunkAlignedPiece ComputeNextChunkAlignedPiece(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuplesPerPiece)
{
  ValidateTupleRange(tupleShape, startTuple, tupleCount);
  const usize capTuples = std::min(tupleCount, std::max<usize>(1, maxTuplesPerPiece));
  if(tupleCount == 0)
  {
    return {startTuple, 0, false};
  }
  if(!IsValidGrid(tupleShape, chunkShape))
  {
    return {startTuple, capTuples, false};
  }

  const GridGeometry geometry = MakeGridGeometry(tupleShape, chunkShape);
  const ShapeType position = FlatToPosition(startTuple, tupleShape);
  const usize rank = tupleShape.size();

  // A slower changing dimension gives a longer aligned range, so the first one that fits gives the longest piece.
  for(usize changingDimension = 0; changingDimension < rank; ++changingDimension)
  {
    if(!IsAlignedStart(position, tupleShape, geometry, changingDimension))
    {
      continue;
    }
    const usize stride = geometry.strides[changingDimension];
    const usize indicesToEnd = tupleShape[changingDimension] - position[changingDimension];
    const usize indicesInCap = capTuples / stride;
    const usize chunkExtent = geometry.chunkExtents[changingDimension];
    const usize indices = indicesInCap >= indicesToEnd ? indicesToEnd : (indicesInCap / chunkExtent) * chunkExtent;
    if(indices != 0)
    {
      const usize pieceTuples = indices * stride;
      return {startTuple, pieceTuples, IsChunkAlignedTupleRange(tupleShape, chunkShape, startTuple, pieceTuples)};
    }
  }

  // No aligned range fits here. End the unaligned piece where the next aligned range can start, so later pieces align.
  usize endTuple = startTuple + capTuples;
  for(usize changingDimension = 0; changingDimension < rank; ++changingDimension)
  {
    const usize candidate = NextAlignedStartCandidate(startTuple, tupleShape, geometry, changingDimension);
    if(candidate < endTuple && IsAlignedStart(FlatToPosition(candidate, tupleShape), tupleShape, geometry, changingDimension))
    {
      endTuple = candidate;
    }
  }
  const usize pieceTuples = endTuple - startTuple;
  return {startTuple, pieceTuples, IsChunkAlignedTupleRange(tupleShape, chunkShape, startTuple, pieceTuples)};
}

std::vector<ChunkAlignedPiece> PlanChunkAlignedPieces(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuplesPerPiece)
{
  ValidateTupleRange(tupleShape, startTuple, tupleCount);
  std::vector<ChunkAlignedPiece> pieces;
  for(usize planned = 0; planned < tupleCount;)
  {
    const ChunkAlignedPiece piece = ComputeNextChunkAlignedPiece(tupleShape, chunkShape, startTuple + planned, tupleCount - planned, maxTuplesPerPiece);
    pieces.push_back(piece);
    planned += piece.tupleCount;
  }
  return pieces;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
void SetAlignedCopyTargetBytesForTesting(usize targetBytes)
{
  g_AlignedCopyTargetBytesOverride.store(targetBytes, std::memory_order_relaxed);
}

usize GetAlignedCopyTargetBytesForTesting()
{
  const usize targetBytes = g_AlignedCopyTargetBytesOverride.load(std::memory_order_relaxed);
  return targetBytes != 0 ? targetBytes : k_AlignedCopyTargetBytes;
}
#endif
} // namespace nx::core
