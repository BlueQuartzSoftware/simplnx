#include "simplnx/Utilities/Parsing/HDF5/ChunkShapePolicy.hpp"

#include <algorithm>

namespace
{
/**
 * @brief Calculates the number of complete rows that fit a byte target.
 * @param dims Dataset dimensions, slowest-varying first.
 * @param firstRowDim Identifies the first full-extent row dimension.
 * @param unitBytes Specifies bytes in one tuple, including all components.
 * @param targetBytes Specifies the physical chunk target.
 * @param maxRows Specifies the subdivided dimension extent.
 * @return Row count in the inclusive range [1, maxRows].
 * @pre firstRowDim is not greater than dims.size(). unitBytes, targetBytes, and maxRows are nonzero.
 * @pre All row-byte products fit usize.
 *
 * A row includes the full extent of each dimension from firstRowDim onward.
 * The function does not split a row across chunks.
 */
nx::core::usize computeRowsForByteTarget(const nx::core::ShapeType& dims, nx::core::usize firstRowDim, nx::core::usize unitBytes, nx::core::usize targetBytes, nx::core::usize maxRows)
{
  nx::core::usize rowBytes = unitBytes;
  for(nx::core::usize i = firstRowDim; i < dims.size(); ++i)
  {
    rowBytes *= dims[i];
  }
  nx::core::usize rows = (rowBytes == 0) ? 1 : targetBytes / rowBytes;
  rows = std::max<nx::core::usize>(rows, 1);
  return std::min(rows, maxRows);
}
} // namespace

namespace nx::core::HDF5
{

ShapeType computeChunkShape(const ShapeType& dims, usize numComponents, usize elementByteSize, const ChunkShapeOptions& opts)
{
  if(dims.empty())
  {
    return {};
  }

  // Fold the full component extent into one tuple so tuple-only dimensions still
  // produce a physical-byte target.
  const usize unitBytes = elementByteSize * numComponents;

  if(opts.regime == ChunkShapeRegime::PinSlowestDim)
  {
    // Rank three or greater pins the slowest dimension to one and bands the next
    // dimension. Lower ranks band the slowest dimension. Inner dimensions stay full.
    ShapeType chunk(dims);
    if(dims.size() >= 3)
    {
      chunk[0] = 1;
      chunk[1] = computeRowsForByteTarget(dims, /*firstRowDim=*/2, unitBytes, opts.targetBytes, dims[1]);
    }
    else
    {
      chunk[0] = computeRowsForByteTarget(dims, /*firstRowDim=*/1, unitBytes, opts.targetBytes, dims[0]);
    }
    return chunk;
  }

  // suffixBytes[i] is the byte cost of one index step in dimension i.
  // Calculate it from the innermost dimension to support the outermost-first walk.
  ShapeType chunk(dims);
  ShapeType suffixBytes(dims.size());
  usize inner = unitBytes;
  for(usize i = dims.size(); i-- > 0;)
  {
    suffixBytes[i] = inner;
    inner *= dims[i];
  }
  // Use extent one while an inner slab meets the target. The first smaller slab
  // bundles as many complete rows as possible. All inner dimensions stay full.
  for(usize i = 0; i < dims.size(); ++i)
  {
    if(suffixBytes[i] >= opts.targetBytes)
    {
      chunk[i] = 1;
      continue;
    }
    usize rows = (suffixBytes[i] == 0) ? 1 : opts.targetBytes / suffixBytes[i];
    rows = std::min<usize>(std::max<usize>(rows, 1), dims[i]);
    chunk[i] = rows;
    break;
  }
  return chunk;
}

} // namespace nx::core::HDF5
