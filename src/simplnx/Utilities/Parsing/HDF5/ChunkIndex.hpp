#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <vector>

/**
 * @namespace nx::core::HDF5
 * @brief Contains HDF5 parsing and I/O utilities.
 */
namespace nx::core::HDF5
{

/**
 * @brief Converts a flat index to a multidimensional position.
 * @param flatIndex Zero-based flat index.
 * @param shape Array dimensions.
 * @return Position in row-major order, with the last dimension fastest.
 * @pre All dimensions are nonzero. For a nonempty shape, flatIndex is in range.
 */
inline std::vector<uint64> flatToNd(uint64 flatIndex, const std::vector<uint64>& shape)
{
  std::vector<uint64> position(shape.size());
  for(uint64 d = shape.size(); d > 0; --d)
  {
    uint64 i = d - 1;
    position[i] = flatIndex % shape[i];
    flatIndex /= shape[i];
  }
  return position;
}

/**
 * @brief Converts a multidimensional position to a flat index.
 * @param position Zero-based coordinate in each dimension.
 * @param shape Array dimensions.
 * @return Flat index in row-major order, with the last dimension fastest.
 * @pre position and shape have equal size. Coordinates are in range and arithmetic fits uint64.
 */
inline uint64 ndToFlat(const std::vector<uint64>& position, const std::vector<uint64>& shape)
{
  uint64 flat = 0;
  uint64 stride = 1;
  for(uint64 d = shape.size(); d > 0; --d)
  {
    uint64 i = d - 1;
    flat += position[i] * stride;
    stride *= shape[i];
  }
  return flat;
}

/**
 * @brief Converts a tuple position to its multidimensional chunk index.
 * @param position Zero-based tuple coordinate.
 * @param chunkShape Chunk dimensions.
 * @return Zero-based chunk coordinate.
 * @pre position and chunkShape have equal size, and all chunk dimensions are nonzero.
 */
inline std::vector<uint64> positionToChunkNd(const std::vector<uint64>& position, const std::vector<uint64>& chunkShape)
{
  std::vector<uint64> chunkNd(position.size());
  for(uint64 d = 0; d < position.size(); ++d)
  {
    chunkNd[d] = position[d] / chunkShape[d];
  }
  return chunkNd;
}

/**
 * @brief Converts a multidimensional chunk index to a flat chunk index.
 * @param chunkNd Zero-based chunk coordinate.
 * @param chunksPerDim Chunk counts in each dimension.
 * @return Row-major flat chunk index.
 * @pre Inputs satisfy ndToFlat() preconditions.
 */
inline uint64 chunkNdToFlat(const std::vector<uint64>& chunkNd, const std::vector<uint64>& chunksPerDim)
{
  return ndToFlat(chunkNd, chunksPerDim);
}

/**
 * @brief Calculates the chunk count in each dimension.
 * @param tupleShape Array tuple dimensions.
 * @param chunkShape Chunk dimensions.
 * @return Ceiling of tupleShape divided by chunkShape in each dimension.
 * @pre Inputs have equal size, chunk dimensions are nonzero, and additions fit uint64.
 */
inline std::vector<uint64> getChunksPerDimension(const std::vector<uint64>& tupleShape, const std::vector<uint64>& chunkShape)
{
  std::vector<uint64> chunksPerDim(tupleShape.size());
  for(uint64 d = 0; d < tupleShape.size(); ++d)
  {
    chunksPerDim[d] = (tupleShape[d] + chunkShape[d] - 1) / chunkShape[d];
  }
  return chunksPerDim;
}

/**
 * @brief Calculates the total chunk count.
 * @param tupleShape Array tuple dimensions.
 * @param chunkShape Chunk dimensions.
 * @return Product of per-dimension chunk counts. Empty shapes return one.
 * @pre Inputs satisfy getChunksPerDimension() preconditions, and the product fits uint64.
 */
inline uint64 getNumberOfChunks(const std::vector<uint64>& tupleShape, const std::vector<uint64>& chunkShape)
{
  auto chunksPerDim = getChunksPerDimension(tupleShape, chunkShape);
  uint64 total = 1;
  for(uint64 d = 0; d < chunksPerDim.size(); ++d)
  {
    total *= chunksPerDim[d];
  }
  return total;
}

/**
 * @brief Calculates one chunk's inclusive tuple extent.
 * @param flatChunkIndex Zero-based flat chunk index.
 * @param tupleShape Nonzero array tuple dimensions.
 * @param chunkShape Nonzero chunk dimensions.
 * @return Inclusive extent clamped to array bounds for an edge chunk.
 * @pre Shapes have equal size and all nonempty dimensions are nonzero. The index is in range, and arithmetic fits uint64.
 */
inline Extent getChunkBounds(uint64 flatChunkIndex, const std::vector<uint64>& tupleShape, const std::vector<uint64>& chunkShape)
{
  auto chunksPerDim = getChunksPerDimension(tupleShape, chunkShape);
  auto chunkNd = flatToNd(flatChunkIndex, chunksPerDim);

  std::vector<uint64> minBounds(tupleShape.size());
  std::vector<uint64> maxBounds(tupleShape.size());
  for(uint64 d = 0; d < tupleShape.size(); ++d)
  {
    minBounds[d] = chunkNd[d] * chunkShape[d];
    maxBounds[d] = std::min(minBounds[d] + chunkShape[d] - 1, tupleShape[d] - 1);
  }
  return Extent{std::move(minBounds), std::move(maxBounds)};
}
} // namespace nx::core::HDF5
