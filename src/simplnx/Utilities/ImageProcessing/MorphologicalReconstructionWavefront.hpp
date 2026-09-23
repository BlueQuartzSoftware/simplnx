#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <vector>

/**
 * @file MorphologicalReconstructionWavefront.hpp
 * @brief Block-wavefront tiling machinery for the in-plane parallel morphological reconstruction sweeps: splits a
 *        dimX x dimY plane into square blocks, groups the blocks into dependency-respecting wavefronts, and runs a
 *        directional sweep over those wavefronts with independent blocks executed concurrently. Only the streamed
 *        (out-of-core) sweep translation unit includes this header, so its parallel scheduling logic is generated
 *        in one place, isolated from the in-core hybrid engine and the serial streamed sweep bodies that
 *        never call it.
 */

namespace nx::core::ImageProcessing::detail
{
// Block-wavefront tiling constants for the in-plane parallel sweeps (see BuildReconstructionPlaneWavefrontSchedule
// and RunPlaneWavefront below). The edge is clamped to this range so a plane never fragments into so many tiny
// blocks that per-wavefront synchronization dominates, nor stays so large that there is nothing to parallelize.
inline constexpr usize k_ReconstructionMinBlockEdge = 64;
inline constexpr usize k_ReconstructionMaxBlockEdge = 256;
// Heuristic target: aim for roughly this many blocks along the plane's SHORTER edge. A wavefront's block count is
// bounded by the shorter edge's block count (a wave can have at most min(blockRows, blockCols) blocks, since each
// block in a wave needs a distinct blockRow and a distinct blockCol -- see RunPlaneWavefront's proof), so this is
// what actually caps how many blocks can run concurrently; 8 comfortably covers common core counts without
// over-subdividing planes that do not need it.
inline constexpr usize k_ReconstructionTargetBlocksPerShortEdge = 8;

/**
 * @brief Chooses the square block edge used to tile one dimX x dimY plane for the wavefront-parallel sweeps below.
 *        Targets k_ReconstructionTargetBlocksPerShortEdge blocks along the shorter plane dimension, clamped to
 *        [k_ReconstructionMinBlockEdge, k_ReconstructionMaxBlockEdge]. A plane no larger than the minimum edge in
 *        both dimensions collapses to a single block covering the whole plane, which reproduces today's fully
 *        serial per-plane sweep exactly (the required no-parallelism floor) without any dedicated small-plane path.
 */
inline usize ComputeReconstructionBlockEdge(usize dimX, usize dimY)
{
  const usize shortEdge = std::min(dimX, dimY);
  if(shortEdge == 0)
  {
    return 1;
  }
  const usize targetEdge = shortEdge / k_ReconstructionTargetBlocksPerShortEdge;
  return std::clamp(targetEdge, k_ReconstructionMinBlockEdge, k_ReconstructionMaxBlockEdge);
}

/**
 * @brief One tile of an in-plane wavefront sweep, in real (direction-independent) coordinates: covers rows
 *        [rowBegin, rowEnd) and columns [colBegin, colEnd).
 */
struct ReconstructionPlaneBlock
{
  usize rowBegin;
  usize rowEnd;
  usize colBegin;
  usize colEnd;
};

/**
 * @brief A dimX x dimY plane tiled into blockEdge x blockEdge blocks (partial blocks along the far edges) and
 *        grouped into wavefronts: waves[w] holds every block whose index (2*blockRow + blockCol) equals w.
 *        RunPlaneWavefront's documentation proves why 2*blockRow + blockCol is the correct schedule for both
 *        connectivities and both sweep directions. The tiling depends only on the plane's fixed dimensions, so
 *        this is built once per algorithm run and reused for every plane, direction, and sweep pair.
 */
struct ReconstructionPlaneWavefrontSchedule
{
  usize blockEdge = 1;
  usize blockRows = 1;
  usize blockCols = 1;
  std::vector<std::vector<ReconstructionPlaneBlock>> waves;
};

inline ReconstructionPlaneWavefrontSchedule BuildReconstructionPlaneWavefrontSchedule(usize dimX, usize dimY)
{
  ReconstructionPlaneWavefrontSchedule schedule;
  if(dimX == 0 || dimY == 0)
  {
    return schedule;
  }
  schedule.blockEdge = ComputeReconstructionBlockEdge(dimX, dimY);
  schedule.blockRows = (dimY + schedule.blockEdge - 1) / schedule.blockEdge;
  schedule.blockCols = (dimX + schedule.blockEdge - 1) / schedule.blockEdge;
  const usize waveCount = 2 * (schedule.blockRows - 1) + (schedule.blockCols - 1) + 1;
  schedule.waves.assign(waveCount, {});
  for(usize blockRow = 0; blockRow < schedule.blockRows; ++blockRow)
  {
    const usize rowBegin = blockRow * schedule.blockEdge;
    const usize rowEnd = std::min(dimY, rowBegin + schedule.blockEdge);
    for(usize blockCol = 0; blockCol < schedule.blockCols; ++blockCol)
    {
      const usize colBegin = blockCol * schedule.blockEdge;
      const usize colEnd = std::min(dimX, colBegin + schedule.blockEdge);
      schedule.waves[2 * blockRow + blockCol].push_back(ReconstructionPlaneBlock{rowBegin, rowEnd, colBegin, colEnd});
    }
  }
  return schedule;
}

/**
 * @brief Runs one in-plane directional sweep over @p schedule's blocks, letting every block within a wavefront run
 *        concurrently while preserving, within a block, the un-blocked serial sweep's exact voxel update order.
 *
 * TWO usage modes are safe, with different guarantees; see the connectivity-specific reasoning below.
 *
 * (a) FACE-connectivity one-pass sweeps -- bit-exact. MakeReconstructionOffsets partitions all\{center} into
 * "previous" (forward's in-plane predecessors, already-updated when forward reaches a voxel) and "later"
 * (reverse's). For face connectivity, previous's in-plane (dz==0) members are just {N, W}; later mirrors this to
 * {S, E}. Tiling the plane into blockEdge x blockEdge blocks and indexing block (blockRow, blockCol) by
 * w = 2*blockRow + blockCol places N's block at w-2 and W's block at w-1 -- both strictly smaller than the current
 * block's w -- so visiting waves in ASCENDING order never starts a block before every block it can read from has
 * finished. S (w+2) and E (w+1) mirror this for DESCENDING order. Two blocks sharing one wave always have distinct
 * blockRow AND distinct blockCol (w = 2*blockRow + blockCol is strictly monotonic in blockRow for a fixed w, so
 * equal w with different blockRow forces different blockCol), so they cover disjoint rectangles of the plane and
 * never race on any voxel. Inside a block, voxels are still visited in the same ascending/descending row-major
 * order the un-blocked sweep used, so any predecessor that lies inside the same block was already updated earlier
 * in that identical order. Every voxel therefore reads exactly the value the fully serial sweep would have
 * produced at that point, so the result is bit-identical and the pass/convergence count is unchanged.
 *
 * (b) FULL connectivity, run as a forward/reverse pair iterated to convergence -- fixpoint-exact, NOT bit-exact
 * per pass. Full connectivity's in-plane predecessor set adds the diagonals (forward gains NW, NE; reverse gains
 * SE, SW), and a diagonal offset does not always cross the same block boundary its per-voxel row/column step
 * implies: forward's NE steps one row up and one column right, but at a voxel that is NOT on the block's top row
 * edge, the row step stays inside the current blockRow, so NE's neighbor sits in block (blockRow, blockCol+1) =
 * w+1 -- STRICTLY LARGER than the current block's w. Ascending order has therefore not necessarily finished that
 * block yet: a genuine horizontal block-dependency cycle no rectangular row-major schedule can order. Reverse's SW
 * has the mirror failure -- at a voxel not on the block's bottom row edge it lands in (blockRow, blockCol-1) = w-1,
 * which descending order has not yet visited. A single full-connectivity forward or reverse pass is therefore NOT
 * guaranteed bit-exact against the un-blocked serial sweep. It remains safe when the caller loops forward/reverse
 * pairs to a fixed point: every update folds the voxel's neighbors with an associative, commutative,
 * mask-clamped max-or-min, so a stale in-plane read can only delay propagation within a pass -- it can never move
 * the volume away from the unique fixed point that same forward/reverse loop would reach serially -- and a pass
 * that reports no change is itself proof nothing was still stale (a stale read that still mattered would still be
 * producing a change). Callers relying on mode (b) MUST loop sweep pairs until a pass makes no change; they must
 * not treat a single pass's result as final.
 *
 * A wavefront holding only one block runs @p blockBody directly, skipping the parallel dispatch -- this is also
 * what happens for every wave whenever the plane is too small to tile (the schedule collapses to a single block in
 * a single wave), so small planes reproduce today's fully serial behavior with no separate code path.
 *
 * @param forward true = ascending wavefront order (raster sweep); false = descending (anti-raster sweep).
 * @param blockBody bool(const ReconstructionPlaneBlock&): must process every voxel in the block in the same
 *        forward/reverse row-major order the un-blocked sweep used, and return true iff it modified the block.
 * @return true iff any block reported a modification.
 */
template <class BlockBody>
bool RunPlaneWavefront(const ReconstructionPlaneWavefrontSchedule& schedule, bool forward, const BlockBody& blockBody)
{
  bool changed = false;
  const usize waveCount = schedule.waves.size();
  for(usize waveStep = 0; waveStep < waveCount; ++waveStep)
  {
    const usize wave = forward ? waveStep : (waveCount - 1 - waveStep);
    const std::vector<ReconstructionPlaneBlock>& blocks = schedule.waves[wave];
    const usize blockCount = blocks.size();
    if(blockCount == 0)
    {
      continue;
    }
    if(blockCount == 1)
    {
      if(blockBody(blocks.front()))
      {
        changed = true;
      }
      continue;
    }
    // One byte per block in this wavefront: each parallel task reports its own block's result into a disjoint
    // slot, so the OR-reduction just below (which runs only after ParallelDataAlgorithm::execute has fully
    // synchronized every task in the wave) needs no atomics or locking.
    std::vector<uint8> blockChanged(blockCount, uint8{0});
    ParallelDataAlgorithm algorithm;
    algorithm.setRange(0, blockCount);
    algorithm.execute([&blocks, &blockChanged, &blockBody](const Range& range) {
      for(usize index = range.min(); index < range.max(); ++index)
      {
        blockChanged[index] = blockBody(blocks[index]) ? uint8{1} : uint8{0};
      }
    });
    for(uint8 flag : blockChanged)
    {
      if(flag != 0)
      {
        changed = true;
        break;
      }
    }
  }
  return changed;
}

/**
 * @brief Runs one in-plane REVERSE (descending wavefront) directional sweep over @p schedule's blocks for a
 *        fold-and-seed pass, then drains the seeds a block collects through @p seedConsumer in block-schedule
 *        order.
 *
 * MUST only be used to drive a FACE-connectivity seeding pass. A seeding pass is inherently a single one-pass
 * fold -- it is never itself iterated to convergence -- so it can only rely on RunPlaneWavefront's mode (a)
 * (face-connectivity, bit-exact per pass), never mode (b) (full connectivity, safe only when forward/reverse
 * passes are looped to a fixed point; see RunPlaneWavefront's documentation for both). Driving this helper with
 * full connectivity would let a same-row-band diagonal read reach into a block the descending schedule has not
 * yet processed, seeding from a value the un-blocked serial sweep would not have seen at that point.
 *
 * WHY this dedicated helper exists alongside RunPlaneWavefront: RunPlaneWavefront's blocks run concurrently and
 * must never touch shared mutable state, but a reverse anti-raster pass that also seeds a flood-fill FIFO needs
 * exactly that -- a side effect unsafe to perform from inside a concurrently-executing block. This helper keeps
 * RunPlaneWavefront's wave loop and concurrency guarantee (descending wave order only, since seeding only ever
 * happens on the reverse anti-raster pass), but has each block append its own candidate seed positions to a
 * private std::vector<uint64> slot instead of a shared queue, and only invokes the caller-supplied serial
 * @p seedConsumer -- once per wave, strictly after that wave's parallel dispatch has fully synchronized -- to drain
 * those per-block vectors in block-schedule order.
 *
 * WHY block-order concatenation is a safe substitute for voxel-exact serial push order: a seed candidate's
 * condition reads only values already finalized earlier in the same reverse traversal (this plane's LATER in-plane
 * predecessors, or an already fully-processed adjacent plane) -- bit-identical to the serial sweep regardless of
 * intra-plane block execution order, by the same guarantee RunPlaneWavefront's docstring proves -- so the exact
 * same SET of positions is queued, independent of order. The flood-fill this seeds folds with a pure, commutative,
 * associative selection (max/min), so its unique fixed point does not depend on dequeue order, only on the seed set
 * and the (already bit-identical) shared state. And if a bounded consumer's capacity is exhausted at a different
 * point because seeds arrive in a different order, every caller's fallback is a full fold-to-convergence resident
 * sweep that reconverges to the same fixed point regardless of how far the interrupted seeding pass got. So
 * concatenating per-block seed vectors in block-schedule order after each wave -- deterministic and race-free -- is
 * safe without reproducing the serial push order voxel-for-voxel.
 *
 * @param blockBody void(const ReconstructionPlaneBlock&, std::vector<uint64>&): must process every voxel in the
 *        block in the same reverse row-major order the un-blocked sweep used, updating shared per-voxel state
 *        in-place, and append the flat index of any voxel that should seed the flood to the given per-block vector.
 * @param seedConsumer bool(uint64): called once per queued seed, in block-schedule order, only after its wave has
 *        fully synchronized; return false to stop draining early (e.g. a bounded FIFO is full), which also stops
 *        this function from visiting any further wave.
 * @return true iff every collected seed was accepted by @p seedConsumer; false iff @p seedConsumer rejected one.
 */
template <class BlockBody, class SeedConsumer>
bool RunReversePlaneWavefrontSeeding(const ReconstructionPlaneWavefrontSchedule& schedule, const BlockBody& blockBody, const SeedConsumer& seedConsumer)
{
  const usize waveCount = schedule.waves.size();
  for(usize waveStep = 0; waveStep < waveCount; ++waveStep)
  {
    const usize wave = waveCount - 1 - waveStep;
    const std::vector<ReconstructionPlaneBlock>& blocks = schedule.waves[wave];
    const usize blockCount = blocks.size();
    if(blockCount == 0)
    {
      continue;
    }
    if(blockCount == 1)
    {
      std::vector<uint64> seeds;
      blockBody(blocks.front(), seeds);
      for(uint64 seed : seeds)
      {
        if(!seedConsumer(seed))
        {
          return false;
        }
      }
      continue;
    }
    // One seed vector per block in this wavefront: each parallel task appends only to its own disjoint slot, so
    // draining them below (which runs only after ParallelDataAlgorithm::execute has fully synchronized every task
    // in the wave) needs no atomics or locking.
    std::vector<std::vector<uint64>> blockSeeds(blockCount);
    ParallelDataAlgorithm algorithm;
    algorithm.setRange(0, blockCount);
    algorithm.execute([&blocks, &blockSeeds, &blockBody](const Range& range) {
      for(usize index = range.min(); index < range.max(); ++index)
      {
        blockBody(blocks[index], blockSeeds[index]);
      }
    });
    for(const std::vector<uint64>& seeds : blockSeeds)
    {
      for(uint64 seed : seeds)
      {
        if(!seedConsumer(seed))
        {
          return false;
        }
      }
    }
  }
  return true;
}
} // namespace nx::core::ImageProcessing::detail
