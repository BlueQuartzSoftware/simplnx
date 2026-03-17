#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/UnionFind.hpp"

#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Returns the 6 face neighbor indices. When isPeriodic is true,
 * boundary voxels wrap to the opposite face instead of being skipped.
 * @param currentPoint Linear voxel index
 * @param width X dimension
 * @param height Y dimension
 * @param depth Z dimension
 * @param isPeriodic Whether to apply periodic boundary wrapping
 * @return Vector of neighbor indices
 */
std::vector<int64> getFaceNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth, const bool isPeriodic)
{
  std::vector<int64> neighbors;
  neighbors.reserve(6);

  const int64 col = currentPoint % width;
  const int64 tmp = currentPoint / width;
  const int64 row = tmp % height;
  const int64 plane = tmp / height;

  const int64 slice = width * height;

  // -X
  if(col > 0)
  {
    neighbors.push_back(currentPoint - 1);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint + width - 1);
  }

  // +X
  if(col < width - 1)
  {
    neighbors.push_back(currentPoint + 1);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint - width + 1);
  }

  // -Y
  if(row > 0)
  {
    neighbors.push_back(currentPoint - width);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint + (height - 1) * width);
  }

  // +Y
  if(row < height - 1)
  {
    neighbors.push_back(currentPoint + width);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint - (height - 1) * width);
  }

  // -Z
  if(plane > 0)
  {
    neighbors.push_back(currentPoint - slice);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint + (depth - 1) * slice);
  }

  // +Z
  if(plane < depth - 1)
  {
    neighbors.push_back(currentPoint + slice);
  }
  else if(isPeriodic)
  {
    neighbors.push_back(currentPoint - (depth - 1) * slice);
  }

  return neighbors;
}

/**
 * @brief Returns up to 26 face/edge/vertex neighbor indices. When isPeriodic
 * is true, boundary voxels wrap to the opposite face instead of being skipped.
 * @param currentPoint Linear voxel index
 * @param width X dimension
 * @param height Y dimension
 * @param depth Z dimension
 * @param isPeriodic Whether to apply periodic boundary wrapping
 * @return Vector of neighbor indices
 */
std::vector<int64> getAllNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth, const bool isPeriodic)
{
  std::vector<int64> neighbors;
  neighbors.reserve(26);

  const int64 col = currentPoint % width;
  const int64 tmp = currentPoint / width;
  const int64 row = tmp % height;
  const int64 plane = tmp / height;

  const int64 slice = width * height;

  for(int64 dz = -1; dz <= 1; ++dz)
  {
    int64 nz = plane + dz;
    if(nz < 0 || nz >= depth)
    {
      if(!isPeriodic)
      {
        continue;
      }
      nz = (nz + depth) % depth;
    }

    for(int64 dy = -1; dy <= 1; ++dy)
    {
      int64 ny = row + dy;
      if(ny < 0 || ny >= height)
      {
        if(!isPeriodic)
        {
          continue;
        }
        ny = (ny + height) % height;
      }

      for(int64 dx = -1; dx <= 1; ++dx)
      {
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }

        int64 nx = col + dx;
        if(nx < 0 || nx >= width)
        {
          if(!isPeriodic)
          {
            continue;
          }
          nx = (nx + width) % width;
        }

        neighbors.push_back(nz * slice + ny * width + nx);
      }
    }
  }

  return neighbors;
}

} // namespace

// -----------------------------------------------------------------------------
SegmentFeatures::SegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHelper(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SegmentFeatures::~SegmentFeatures() = default;

// =============================================================================
// DFS Flood-Fill Segmentation (In-Core Path)
// =============================================================================
//
// This method implements a depth-first search (DFS) flood-fill algorithm for
// segmenting voxels into features when data resides entirely in memory.
//
// Algorithm overview:
//   1. Iterate through voxels to find "seed" voxels — unassigned, valid voxels
//      that start a new feature.
//   2. For each seed, assign a new feature ID (gnum) and push the seed onto a
//      stack (voxelsList).
//   3. Pop voxels from the stack, examine their neighbors via the configured
//      neighbor scheme (Face or FaceEdgeVertex), and call the subclass's
//      determineGrouping() to decide whether a neighbor belongs to the same
//      feature. If so, the neighbor is assigned the feature ID and pushed
//      onto the stack for further expansion.
//   4. When the stack empties, the current feature is complete. Find the next
//      seed and repeat until no seeds remain.
//
// Features are numbered in seed-discovery order (the first unassigned voxel
// encountered becomes feature 1, the next becomes feature 2, etc.).
//
// Performance note:
//   This algorithm uses random-access memory patterns — the stack can pop to
//   any voxel in the volume, causing non-sequential reads. This is efficient
//   for in-core DataStore (O(1) random access) but extremely slow for OOC
//   ZarrStore, where random access triggers chunk loads/evictions ("chunk
//   thrashing"). Use executeCCL() for out-of-core datasets.
// =============================================================================
Result<> SegmentFeatures::execute(IGridGeometry* gridGeom)
{
  ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();

  SizeVec3 udims = gridGeom->getDimensions();

  usize totalVoxels = udims[0] * udims[1] * udims[2];

  int64 dims[3] = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};

  // gnum tracks the current feature ID being assigned, starting at 1.
  // nextSeed is an optimization: it tracks the lowest voxel index that might
  // still be unassigned, so getSeed() can skip over already-segmented voxels
  // instead of rescanning from index 0 every time.
  int32 gnum = 1;
  int64 nextSeed = 0;
  int64 seed = getSeed(gnum, nextSeed);
  nextSeed = seed + 1;
  usize size = 0;

  // voxelsList serves as the DFS stack (LIFO). It is pre-allocated to avoid
  // frequent reallocations. 'size' is the logical stack pointer — elements
  // are pushed by writing to voxelsList[size] and incrementing, and popped
  // by decrementing size and reading voxelsList[size].
  constexpr usize initialVoxelsListSize = 100000;
  std::vector<int64> voxelsList(initialVoxelsListSize, -1);

  usize totalVoxelsSegmented = 0;
  while(seed >= 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    // Start a new feature: push the seed onto the stack
    size = 0;
    voxelsList[size] = seed;
    size++;
    // DFS expansion loop: pop a voxel, check its neighbors, push matches
    while(size > 0)
    {
      // Pop the top of the stack (LIFO order)
      const int64 currentPoint = voxelsList[size - 1];
      size -= 1;
      std::vector<int64> neighPoints;
      switch(m_NeighborScheme)
      {
      case NeighborScheme::Face:
        neighPoints = getFaceNeighbors(currentPoint, dims[0], dims[1], dims[2], m_IsPeriodic);
        break;
      case NeighborScheme::FaceEdgeVertex:
        neighPoints = getAllNeighbors(currentPoint, dims[0], dims[1], dims[2], m_IsPeriodic);
        break;
      }

      for(const auto& neighbor : neighPoints)
      {
        // determineGrouping() is implemented by the subclass. It checks whether
        // the neighbor is unassigned & similar to the reference voxel, and if
        // so, assigns it the current feature ID (gnum) and returns true.
        if(determineGrouping(currentPoint, neighbor, gnum))
        {
          // Push the newly-claimed neighbor onto the stack for further expansion
          voxelsList[size] = neighbor;
          size++;
          // nextSeed optimization: if this neighbor was the next candidate seed,
          // advance nextSeed so getSeed() won't return an already-assigned voxel.
          if(neighbor == nextSeed)
          {
            nextSeed = neighbor + 1;
          }
          // If the stack has grown beyond the allocated capacity, extend it.
          // The stack is stored in a flat vector, so we grow by a fixed block
          // and initialize the new entries to -1.
          if(size >= voxelsList.size())
          {
            size = voxelsList.size();
            voxelsList.resize(size + initialVoxelsListSize);
            for(std::vector<int64>::size_type j = size; j < voxelsList.size(); ++j)
            {
              voxelsList[j] = -1;
            }
          }
          totalVoxelsSegmented++;
        }
      }
    }

    // Send a progress message
    float percentComplete = static_cast<float>(totalVoxelsSegmented) / static_cast<float>(totalVoxels) * 100.0f;
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("{:.2f}% - Current Feature Count: {}", percentComplete, gnum); });
    // Reset the stack for the next feature. assign() shrinks/grows the vector
    // back to the finished feature size + 1 and fills with -1.
    voxelsList.assign(size + 1, -1);
    gnum++;
    // Get the next seed value
    seed = getSeed(gnum, nextSeed); // If seed ends up being -1, then we will exit the loop.
    nextSeed = seed + 1;
  }

  m_FoundFeatures = gnum - 1; // Decrement because gnum ends up 1 larger than the last assigned feature.
  m_MessageHelper.sendMessage(fmt::format("Total Features Found: {}", m_FoundFeatures));
  return {};
}

// =============================================================================
// Chunk-Sequential Connected Component Labeling (CCL) Algorithm
// =============================================================================
//
// This method replaces the DFS flood-fill (execute()) with a scanline-based
// connected-component labeling algorithm optimized for out-of-core (OOC)
// data stores (e.g. ZarrStore). Unlike DFS, which accesses voxels in
// unpredictable stack-driven order, CCL processes voxels in strict Z-Y-X
// scanline order, resulting in sequential chunk access patterns that avoid
// chunk thrashing.
//
// The algorithm has three phases:
//
// Phase 1 (Forward CCL):
//   Scan voxels in Z-Y-X order. For each valid voxel, examine only its
//   "backward" neighbors — those already visited earlier in scanline order.
//   If a backward neighbor has a label and is similar (per areNeighborsSimilar),
//   adopt that label. If multiple distinct labels are found among backward
//   neighbors, unite them in a Union-Find structure. If no backward neighbor
//   matches, assign a fresh provisional label. Labels are written to both an
//   in-memory rolling buffer (for fast neighbor lookups) and to the OOC
//   featureIds store (for persistence).
//
// Phase 1b (Periodic boundary merge):
//   If periodic boundaries are enabled, Phase 1 cannot detect connections
//   that wrap around the volume (the wrapped neighbor has a higher linear
//   index and hasn't been visited yet). This phase reads back provisional
//   labels and unites similar voxels on opposite boundary faces.
//
// Phase 2 (Resolution + Relabeling):
//   Flatten the Union-Find tree, then scan the featureIds store chunk by
//   chunk. For each provisional label, look up its Union-Find root and
//   map it to a contiguous final feature ID. Write the final ID back in
//   the same pass. This combined discover-and-write approach halves the
//   number of OOC accesses compared to separate resolution and write
//   passes, and chunk-sequential iteration ensures optimal I/O.
// =============================================================================
Result<> SegmentFeatures::executeCCL(IGridGeometry* gridGeom, AbstractDataStore<int32>& featureIdsStore)
{
  ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();

  const SizeVec3 udims = gridGeom->getDimensions();
  // getDimensions() returns [X, Y, Z]
  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);
  const usize totalVoxels = static_cast<usize>(dimX) * static_cast<usize>(dimY) * static_cast<usize>(dimZ);

  const int64 sliceStride = dimX * dimY;

  const bool useFaceOnly = (m_NeighborScheme == NeighborScheme::Face);

  UnionFind unionFind;
  int32 nextLabel = 1; // Provisional labels start at 1

  // Rolling 2-slice buffer for backward neighbor label lookups.
  //
  // Why 2 slices is sufficient:
  //   In Z-Y-X scanline order, a voxel at (ix, iy, iz) has backward neighbors
  //   only in the current Z-slice (iz) or the immediately previous Z-slice
  //   (iz-1). No backward neighbor can ever be in Z-slice (iz-2) or earlier,
  //   because all 13 backward neighbor offsets have dz in {-1, 0}. Therefore,
  //   keeping just 2 slices in memory — the current and the previous — is
  //   enough for all backward neighbor label reads.
  //
  // This design uses O(dimX * dimY) memory instead of O(dimX * dimY * dimZ),
  // enabling processing of datasets much larger than available RAM.
  //
  // Buffer layout: Z-slice (iz % 2) occupies indices
  //   [sliceOffset .. sliceOffset + sliceStride), where
  //   sliceOffset = (iz % 2) * sliceSize.
  const usize sliceSize = static_cast<usize>(sliceStride);
  std::vector<int32> labelBuffer(2 * sliceSize, 0);

  // =========================================================================
  // Phase 1: Forward CCL - assign provisional labels using backward neighbors
  // =========================================================================
  m_MessageHelper.sendMessage("Forward CCL pass...");

  for(int64 iz = 0; iz < dimZ; iz++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    // Let the subclass pre-load input arrays (e.g. GoodVoxels, CellPhases,
    // Quats) for this Z-slice into local std::vector buffers. This eliminates
    // per-element OOC overhead during areNeighborsSimilar() calls — instead
    // of each comparison triggering a chunk load from ZarrStore, the subclass
    // reads from fast contiguous vectors that were bulk-loaded once per slice.
    prepareForSlice(iz, dimX, dimY, dimZ);

    // Clear the current slice's portion of the rolling buffer
    const usize currentSliceOffset = static_cast<usize>(iz % 2) * sliceSize;
    std::fill(labelBuffer.begin() + currentSliceOffset, labelBuffer.begin() + currentSliceOffset + sliceSize, 0);

    for(int64 iy = 0; iy < dimY; iy++)
    {
      for(int64 ix = 0; ix < dimX; ix++)
      {
        const int64 index = iz * sliceStride + iy * dimX + ix;
        const usize bufIdx = currentSliceOffset + static_cast<usize>(iy * dimX + ix);

        // Skip voxels that are not valid
        if(!isValidVoxel(index))
        {
          continue;
        }

        // Check backward neighbors for existing labels.
        // "Backward" neighbors are those with a smaller linear index — i.e.,
        // already processed earlier in Z-Y-X scanline order. In 3D, these are
        // neighbors with dz < 0, or dz == 0 && dy < 0, or dz == 0 && dy == 0
        // && dx < 0. Forward neighbors (higher linear index) are not yet
        // labeled and cannot be consulted.
        //
        // Neighbor labels are read from the rolling buffer (direct memory
        // access, O(1)) rather than from the OOC featureIds store, avoiding
        // chunk loads for every neighbor lookup.
        int32 assignedLabel = 0;
        const usize prevSliceOffset = static_cast<usize>((iz + 1) % 2) * sliceSize;

        if(useFaceOnly)
        {
          // Face connectivity: exactly 3 backward neighbors exist:
          //   -X (dx=-1): one column to the left in the same row/slice
          //   -Y (dy=-1): one row earlier in the same slice
          //   -Z (dz=-1): same (x,y) position in the previous slice
          // The 3 forward neighbors (+X, +Y, +Z) have not been labeled yet
          // and are skipped.

          // Check -X neighbor (same Z-slice, same buffer region)
          if(ix > 0)
          {
            const int64 neighIdx = index - 1;
            int32 neighLabel = labelBuffer[bufIdx - 1];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }
          // Check -Y neighbor (same Z-slice, same buffer region)
          if(iy > 0)
          {
            const int64 neighIdx = index - dimX;
            int32 neighLabel = labelBuffer[currentSliceOffset + static_cast<usize>((iy - 1) * dimX + ix)];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }
          // Check -Z neighbor (previous Z-slice, other buffer region)
          if(iz > 0)
          {
            const int64 neighIdx = index - sliceStride;
            int32 neighLabel = labelBuffer[prevSliceOffset + static_cast<usize>(iy * dimX + ix)];
            if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }
        }
        else
        {
          // FaceEdgeVertex connectivity: 13 backward neighbors out of 26 total.
          //
          // A 3x3x3 neighborhood has 26 neighbors (excluding self). Exactly
          // half (13) have a smaller linear index in Z-Y-X order and are thus
          // "backward." These are enumerated by iterating:
          //   dz in {-1, 0}:
          //     dz=-1: all 9 neighbors in the previous Z-slice (any dx, dy)
          //     dz= 0: only neighbors with dy < 0 (3 neighbors), or
          //            dy == 0 && dx == -1 (1 neighbor) => 4 total
          //   Total: 9 + 4 = 13 backward neighbors
          //
          // The loop bounds below encode this enumeration efficiently:
          //   - dz ranges [-1, 0]
          //   - dy ranges [-1, +1] when dz<0, or [-1, 0] when dz==0
          //   - dx ranges [-1, +1] when dz<0 or dy<0, or [-1, -1] when dz==0 && dy==0
          for(int64 dz = -1; dz <= 0; ++dz)
          {
            const int64 nz = iz + dz;
            if(nz < 0 || nz >= dimZ)
            {
              continue;
            }

            const usize neighSliceOffset = (dz < 0) ? prevSliceOffset : currentSliceOffset;

            const int64 dyStart = -1;
            const int64 dyEnd = (dz < 0) ? 1 : 0;

            for(int64 dy = dyStart; dy <= dyEnd; ++dy)
            {
              const int64 ny = iy + dy;
              if(ny < 0 || ny >= dimY)
              {
                continue;
              }

              int64 dxStart;
              int64 dxEnd;
              if(dz < 0)
              {
                dxStart = -1;
                dxEnd = 1;
              }
              else if(dy < 0)
              {
                dxStart = -1;
                dxEnd = 1;
              }
              else
              {
                dxStart = -1;
                dxEnd = -1;
              }

              for(int64 dx = dxStart; dx <= dxEnd; ++dx)
              {
                const int64 nx = ix + dx;
                if(nx < 0 || nx >= dimX)
                {
                  continue;
                }
                if(dx == 0 && dy == 0 && dz == 0)
                {
                  continue;
                }

                const int64 neighIdx = nz * sliceStride + ny * dimX + nx;
                int32 neighLabel = labelBuffer[neighSliceOffset + static_cast<usize>(ny * dimX + nx)];
                if(neighLabel > 0 && areNeighborsSimilar(index, neighIdx))
                {
                  if(assignedLabel == 0)
                  {
                    assignedLabel = neighLabel;
                  }
                  else if(assignedLabel != neighLabel)
                  {
                    unionFind.unite(assignedLabel, neighLabel);
                  }
                }
              }
            }
          }
        }

        // If no matching backward neighbor, assign new provisional label
        if(assignedLabel == 0)
        {
          assignedLabel = nextLabel++;
          unionFind.find(assignedLabel); // Initialize in union-find
        }

        // Write label to both rolling buffer (for neighbor reads) and featureIds store
        labelBuffer[bufIdx] = assignedLabel;
        featureIdsStore[index] = assignedLabel;
      }
    }

    // Send progress per Z-slice
    float percentComplete = static_cast<float>(iz + 1) / static_cast<float>(dimZ) * 100.0f;
    throttledMessenger.sendThrottledMessage([percentComplete]() { return fmt::format("Forward CCL: {:.1f}% complete", percentComplete); });
  }

  featureIdsStore.flush();

  if(m_ShouldCancel)
  {
    return {};
  }

  // Disable subclass input buffering by passing iz=-1 as a sentinel value.
  // Phase 1b (periodic boundary merge) compares voxels on opposite faces of
  // the volume, which may be in any Z-slice (e.g. iz=0 vs iz=dimZ-1). The
  // subclass's 1-or-2-slice buffering strategy from Phase 1 cannot handle
  // arbitrary cross-volume access, so we signal it to fall back to direct
  // (unbuffered) reads from the underlying data store.
  prepareForSlice(-1, dimX, dimY, dimZ);

  // =========================================================================
  // Phase 1b: Periodic boundary merge
  // =========================================================================
  // The forward CCL pass cannot detect connections that wrap around periodic
  // boundaries because the wrapped neighbor has a higher linear index and
  // has not been processed yet when the boundary voxel is visited. This
  // phase reads back provisional labels from featureIdsStore and unites
  // labels of similar voxels on opposite boundary faces.
  if(m_IsPeriodic)
  {
    m_MessageHelper.sendMessage("Merging periodic boundaries...");

    if(useFaceOnly)
    {
      // Face connectivity: each axis is handled independently because face
      // neighbors only connect along a single axis. For each axis, we
      // iterate over the 2D face and compare each voxel at the low boundary
      // (e.g. ix=0) with its counterpart at the high boundary (e.g.
      // ix=dimX-1). These are the same voxel pairs that getFaceNeighbors()
      // would return with isPeriodic=true, but which Phase 1 could not
      // process because the wrapped neighbor had not yet been labeled.

      // X-axis: unite voxels at ix=0 with ix=dimX-1
      if(dimX > 1)
      {
        for(int64 iz = 0; iz < dimZ; iz++)
        {
          for(int64 iy = 0; iy < dimY; iy++)
          {
            const int64 idxA = iz * sliceStride + iy * dimX;
            const int64 idxB = iz * sliceStride + iy * dimX + (dimX - 1);
            const int32 labelA = featureIdsStore[idxA];
            const int32 labelB = featureIdsStore[idxB];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              unionFind.unite(labelA, labelB);
            }
          }
        }
      }

      // Y-axis: unite voxels at iy=0 with iy=dimY-1
      if(dimY > 1)
      {
        for(int64 iz = 0; iz < dimZ; iz++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
            const int64 idxA = iz * sliceStride + ix;
            const int64 idxB = iz * sliceStride + (dimY - 1) * dimX + ix;
            const int32 labelA = featureIdsStore[idxA];
            const int32 labelB = featureIdsStore[idxB];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              unionFind.unite(labelA, labelB);
            }
          }
        }
      }

      // Z-axis: unite voxels at iz=0 with iz=dimZ-1
      if(dimZ > 1)
      {
        for(int64 iy = 0; iy < dimY; iy++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
            const int64 idxA = iy * dimX + ix;
            const int64 idxB = (dimZ - 1) * sliceStride + iy * dimX + ix;
            const int32 labelA = featureIdsStore[idxA];
            const int32 labelB = featureIdsStore[idxB];
            if(labelA > 0 && labelB > 0 && areNeighborsSimilar(idxA, idxB))
            {
              unionFind.unite(labelA, labelB);
            }
          }
        }
      }
    }
    else
    {
      // FaceEdgeVertex connectivity: check all 26-neighbor pairs that wrap
      // across periodic boundaries. Unlike face-only mode, edge and vertex
      // neighbors can wrap across two or even three axes simultaneously
      // (e.g. a corner voxel's diagonal neighbor wraps in X, Y, and Z).
      // This requires checking all 26 neighbor offsets for every boundary
      // voxel, filtering to only those that actually wrap.
      //
      // The onBoundary check skips interior voxels (whose 26 neighbors are
      // all within bounds and were already handled by Phase 1).
      //
      // The neighIdx > index deduplication ensures each pair of periodic
      // neighbors is united exactly once. Since union-find is symmetric
      // (unite(A,B) == unite(B,A)), processing only the pair where the
      // neighbor has the larger linear index avoids redundant work.
      for(int64 iz = 0; iz < dimZ; iz++)
      {
        for(int64 iy = 0; iy < dimY; iy++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
            // Only boundary voxels can have neighbors that wrap around
            const bool onBoundary = (ix == 0 || ix == dimX - 1 || iy == 0 || iy == dimY - 1 || iz == 0 || iz == dimZ - 1);
            if(!onBoundary)
            {
              continue;
            }

            const int64 index = iz * sliceStride + iy * dimX + ix;
            const int32 labelCurrent = featureIdsStore[index];
            if(labelCurrent <= 0)
            {
              continue;
            }

            for(int64 dz = -1; dz <= 1; ++dz)
            {
              int64 nz = iz + dz;
              bool wrappedZ = false;
              if(nz < 0)
              {
                nz += dimZ;
                wrappedZ = true;
              }
              else if(nz >= dimZ)
              {
                nz -= dimZ;
                wrappedZ = true;
              }

              for(int64 dy = -1; dy <= 1; ++dy)
              {
                int64 ny = iy + dy;
                bool wrappedY = false;
                if(ny < 0)
                {
                  ny += dimY;
                  wrappedY = true;
                }
                else if(ny >= dimY)
                {
                  ny -= dimY;
                  wrappedY = true;
                }

                for(int64 dx = -1; dx <= 1; ++dx)
                {
                  if(dx == 0 && dy == 0 && dz == 0)
                  {
                    continue;
                  }

                  int64 nx = ix + dx;
                  bool wrappedX = false;
                  if(nx < 0)
                  {
                    nx += dimX;
                    wrappedX = true;
                  }
                  else if(nx >= dimX)
                  {
                    nx -= dimX;
                    wrappedX = true;
                  }

                  // Only process pairs that actually wrap around at least one
                  // axis. Non-wrapped pairs were already handled in Phase 1.
                  if(!wrappedX && !wrappedY && !wrappedZ)
                  {
                    continue;
                  }

                  const int64 neighIdx = nz * sliceStride + ny * dimX + nx;
                  // Deduplication: only process the pair where neighIdx > index.
                  // This ensures each (voxelA, voxelB) pair is united exactly
                  // once, since unite() is symmetric.
                  if(neighIdx <= index)
                  {
                    continue;
                  }

                  const int32 labelNeigh = featureIdsStore[neighIdx];
                  if(labelNeigh > 0 && areNeighborsSimilar(index, neighIdx))
                  {
                    unionFind.unite(labelCurrent, labelNeigh);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // =========================================================================
  // Phase 2: Resolution + Relabeling (combined single pass)
  // =========================================================================
  //
  // After Phase 1/1b, every valid voxel has a provisional label and the
  // Union-Find knows which provisional labels belong to the same connected
  // component. This phase:
  //   1. Flattens the Union-Find so every label points directly to its root
  //      (path compression eliminates intermediate nodes).
  //   2. Scans voxels chunk-by-chunk in deterministic order. For each
  //      provisional label, performs a two-level lookup:
  //        a) label -> root: via unionFind.find(label) (O(1) after flatten)
  //        b) root -> finalId: via the labelToFinal[] map
  //      If the root has not yet been assigned a final ID, allocate the next
  //      sequential ID (finalFeatureCount++). Then cache the mapping for the
  //      original label as well (labelToFinal[label] = finalId) so subsequent
  //      voxels with the same provisional label skip the union-find lookup.
  //   3. Writes the final ID back to featureIdsStore[index] in the same pass.
  //
  // Combining discovery and relabeling into a single pass halves the number
  // of OOC chunk loads compared to doing them separately. The chunk-sequential
  // iteration order ensures each chunk is loaded exactly once.
  //
  // Because the scan is in linear (Z-Y-X) order, final feature IDs are
  // assigned in the order their first voxel appears in the volume, matching
  // the seed-discovery order of the DFS algorithm.
  // =========================================================================
  m_MessageHelper.sendMessage("Resolving labels and writing final feature IDs...");

  unionFind.flatten();

  // labelToFinal maps provisional label -> final contiguous feature ID.
  // Indexed by provisional label (0..nextLabel-1). A value of 0 means
  // "not yet assigned." This avoids a hash map and gives O(1) lookups.
  std::vector<int32> labelToFinal(static_cast<usize>(nextLabel), 0);
  int32 finalFeatureCount = 0;

  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    featureIdsStore.loadChunk(chunkIdx);
    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * static_cast<usize>(sliceStride) + y * static_cast<usize>(dimX) + x;
          int32 label = featureIdsStore[index];
          if(label > 0)
          {
            // Two-level lookup: provisional label -> union-find root -> final ID
            if(labelToFinal[label] == 0)
            {
              // Level 1: find this label's root in the (flattened) union-find
              int32 root = static_cast<int32>(unionFind.find(label));
              // Level 2: if the root hasn't been assigned a final ID yet,
              // allocate the next sequential feature ID
              if(labelToFinal[root] == 0)
              {
                finalFeatureCount++;
                labelToFinal[root] = finalFeatureCount;
              }
              // Cache the mapping for this provisional label so future voxels
              // with the same label skip the union-find lookup entirely
              labelToFinal[label] = labelToFinal[root];
            }
            // Write the final contiguous feature ID back to the data store
            featureIdsStore[index] = labelToFinal[label];
          }
        }
      }
    }

    // Send progress
    float percentComplete = static_cast<float>(chunkIdx + 1) / static_cast<float>(numChunks) * 100.0f;
    throttledMessenger.sendThrottledMessage([percentComplete]() { return fmt::format("Relabeling: {:.1f}% chunks complete", percentComplete); });
  }

  featureIdsStore.flush();

  m_FoundFeatures = finalFeatureCount;
  m_MessageHelper.sendMessage(fmt::format("Total Features Found: {}", m_FoundFeatures));
  return {};
}

// -----------------------------------------------------------------------------
int64 SegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  return -1;
}

// -----------------------------------------------------------------------------
bool SegmentFeatures::determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const
{
  return false;
}

// -----------------------------------------------------------------------------
void SegmentFeatures::prepareForSlice(int64 /*iz*/, int64 /*dimX*/, int64 /*dimY*/, int64 /*dimZ*/)
{
}

// -----------------------------------------------------------------------------
bool SegmentFeatures::isValidVoxel(int64 point) const
{
  return true;
}

// -----------------------------------------------------------------------------
bool SegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  return false;
}

// -----------------------------------------------------------------------------
SegmentFeatures::SeedGenerator SegmentFeatures::initializeStaticVoxelSeedGenerator() const
{
  return SeedGenerator(SeedGenerator::default_seed);
}

// -----------------------------------------------------------------------------
void SegmentFeatures::randomizeFeatureIds(nx::core::Int32Array* featureIds, uint64 totalFeatures)
{
  m_MessageHelper.sendMessage("Randomizing Feature Ids");
  ClusterUtilities::RandomizeFeatureIds(featureIds->getDataStoreRef(), totalFeatures);
}
