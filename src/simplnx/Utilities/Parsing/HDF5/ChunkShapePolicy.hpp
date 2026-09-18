#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core::HDF5
{

/**
 * @brief Specifies the shared one-MiB physical chunk target.
 *
 * This size fits in the configured HDF5 chunk cache and forms one codec work unit.
 * It also keeps chunk-index overhead small relative to the payload.
 */
inline constexpr uint64 k_TargetChunkBytes = 1ull << 20; // 1 MiB

/**
 * @brief Specifies the shared contiguous-storage threshold.
 *
 * Below 16 KiB, chunk-index overhead can exceed compression and streaming benefits.
 * Callers apply this threshold. computeChunkShape always returns a shape.
 */
inline constexpr uint64 k_SmallArrayThresholdBytes = 16ull * 1024ull; // 16 KiB

/**
 * @enum ChunkShapeRegime
 * @brief Selects how the slowest-varying dimensions form chunks.
 */
enum class ChunkShapeRegime
{
  /**
   * @brief Uses an outermost-first shape for write-once output.
   *
   * Inner dimensions keep full extent. The first dimension whose inner slab fits
   * the target includes as many complete slabs as possible. Larger outer dimensions
   * use extent one. This produces fewer, larger chunks for compression.
   */
  BundleOuterSlabs,

  /**
   * @brief Keeps session read-modify-write access aligned with outer slices.
   *
   * For rank three or greater, the slowest dimension has extent one. The next
   * dimension forms row bands. For rank one or two, the slowest dimension forms
   * the row bands. A slice read does not include an adjacent outer slice.
   */
  PinSlowestDim
};

/**
 * @struct ChunkShapeOptions
 * @brief Supplies the byte target and outer-dimension policy.
 *
 * Defaults select the shared one-MiB target and write-once policy. Session stores
 * select PinSlowestDim.
 */
struct ChunkShapeOptions
{
  uint64 targetBytes = k_TargetChunkBytes;
  ChunkShapeRegime regime = ChunkShapeRegime::BundleOuterSlabs;
};

/**
 * @brief Computes a byte-targeted row-band chunk shape.
 * @param dims Dataset dimensions, slowest-varying first.
 * @param numComponents Full trailing component count. Use one when dims includes component dimensions.
 * @param elementByteSize Bytes in one dataset element.
 * @param opts Byte target and outer-dimension regime.
 * @return Chunk shape with the same rank as dims. Empty dims produces an empty shape.
 * @pre If dims is not empty, all dimensions are nonzero.
 * @pre numComponents, elementByteSize, and opts.targetBytes are nonzero.
 * @pre All intermediate byte products fit usize.
 *
 * When dims contains only tuple dimensions, numComponents includes the complete
 * component-shape product. Thus, component bytes count toward the target. The
 * caller makes the separate contiguous-versus-chunked decision.
 */
SIMPLNX_EXPORT nx::core::ShapeType computeChunkShape(const nx::core::ShapeType& dims, usize numComponents, usize elementByteSize, const ChunkShapeOptions& opts);

} // namespace nx::core::HDF5
