#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <vector>

namespace nx::core
{
/**
 * @struct ChunkAlignedPiece
 * @brief Describes one consecutive flat tuple range of a transfer plan.
 */
struct ChunkAlignedPiece
{
  /// First flat tuple of the range.
  usize startTuple = 0;
  /// Number of tuples in the range. The count is 0 only for a piece of an empty range.
  usize tupleCount = 0;
  /**
   * @brief True if the range covers only whole chunks of the grid.
   *
   * A chunk-granular writer can then replace each chunk that the range touches without a read-modify-write.
   * IsChunkAlignedTupleRange() defines the rule.
   */
  bool isChunkAligned = false;

  bool operator==(const ChunkAlignedPiece&) const = default;
};

/**
 * @brief Tells if a flat tuple range covers only whole chunks of a tuple-rank chunk grid.
 * @param tupleShape Row-major tuple shape. Dimension 0 changes slowest.
 * @param chunkShape Chunk extent for each tuple dimension. An extent can be larger than its tuple dimension.
 * @param startTuple First flat tuple of the range.
 * @param tupleCount Number of tuples in the range.
 * @return True if the range is chunk aligned. False for an empty range or an invalid grid. A grid is invalid if its
 * rank is zero or differs from the tuple rank, or if a chunk extent is 0.
 * @throws std::out_of_range If the range extends past the last tuple.
 * @throws std::overflow_error If the tuple count of the shape overflows usize.
 *
 * A chunk extent is clamped to the dataset: the last chunk of a dimension can be shorter than the chunk shape.
 * - A range of one tuple is aligned only if every clamped chunk extent at that tuple is 1.
 * - Otherwise let c be the slowest dimension in which the first and last tuple differ. Every dimension slower than
 *   c has a clamped chunk extent of 1 at the range position. In dimension c the range starts on a chunk boundary
 *   and ends on a chunk boundary or at the dataset end. Every dimension faster than c covers its full extent.
 *
 * This is the rule that the out-of-core parallel chunk writer uses to accept a tuple write. Keep both rules
 * identical, because a transfer planner uses this function to predict which writes take the parallel path.
 */
SIMPLNX_EXPORT bool IsChunkAlignedTupleRange(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount);

/**
 * @brief Returns the tuple count of the smallest chunk-aligned range in the interior of a chunk grid.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension.
 * @return Tuples in one nominal aligned unit, or 0 if the grid is invalid or the shape holds no tuples.
 *
 * Let c be the first dimension whose clamped chunk extent is not 1. If there is no such dimension, c is the fastest
 * dimension. The unit is the clamped chunk extent of c, multiplied by the tuples in one index of c. Ranges at the
 * dataset edge can be aligned with fewer tuples. A caller uses this value to skip transfer planning for a range that
 * is too small to contain an aligned piece.
 */
SIMPLNX_EXPORT usize ComputeNominalChunkAlignedUnitTuples(const ShapeType& tupleShape, const ShapeType& chunkShape);

/**
 * @brief Returns the first piece of a chunk-aligned split of a flat tuple range.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension. An invalid grid gives unaligned pieces only.
 * @param startTuple First flat tuple of the range.
 * @param tupleCount Number of tuples in the range.
 * @param maxTuplesPerPiece Largest permitted piece. A value of 0 is used as 1.
 * @return Piece that starts at startTuple. Its tupleCount is 0 only if tupleCount is 0.
 * @throws std::out_of_range If the range extends past the last tuple.
 * @throws std::overflow_error If the tuple count of the shape overflows usize.
 *
 * The piece is the longest aligned range that starts at startTuple and fits in the cap. If no aligned range fits,
 * the piece is unaligned. It ends at the next position where an aligned range can start, or at the cap. An
 * aligned unit that is larger than the cap is therefore split into unaligned pieces. Thus memory stays bounded for
 * a grid with very large chunks. The isChunkAligned flag always equals IsChunkAlignedTupleRange() for the piece.
 *
 * A caller whose chunk grid can change during a transfer calls this function again for each piece.
 */
SIMPLNX_EXPORT ChunkAlignedPiece ComputeNextChunkAlignedPiece(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuplesPerPiece);

/**
 * @brief Splits a flat tuple range into consecutive pieces that are marked aligned or unaligned against a chunk grid.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension. An invalid grid gives unaligned pieces only.
 * @param startTuple First flat tuple of the range.
 * @param tupleCount Number of tuples in the range.
 * @param maxTuplesPerPiece Largest permitted piece. A value of 0 is used as 1.
 * @return Pieces in ascending order that cover the range with no gap or overlap. Empty if tupleCount is 0.
 * @throws std::out_of_range If the range extends past the last tuple.
 * @throws std::overflow_error If the tuple count of the shape overflows usize.
 *
 * Each piece is the result of ComputeNextChunkAlignedPiece() at the end of the previous piece.
 */
SIMPLNX_EXPORT std::vector<ChunkAlignedPiece> PlanChunkAlignedPieces(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuplesPerPiece);
} // namespace nx::core
