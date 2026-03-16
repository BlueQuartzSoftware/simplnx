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

// -----------------------------------------------------------------------------
Result<> SegmentFeatures::execute(IGridGeometry* gridGeom)
{
  ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();

  SizeVec3 udims = gridGeom->getDimensions();

  usize totalVoxels = udims[0] * udims[1] * udims[2];

  int64 dims[3] = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};

  // Initialize a sequence of execution modifiers
  int32 gnum = 1;
  int64 nextSeed = 0;
  int64 seed = getSeed(gnum, nextSeed);
  nextSeed = seed + 1;
  usize size = 0;

  // Initialize containers
  constexpr usize initialVoxelsListSize = 100000;
  std::vector<int64> voxelsList(initialVoxelsListSize, -1);

  usize totalVoxelsSegmented = 0;
  while(seed >= 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    size = 0;
    voxelsList[size] = seed;
    size++;
    while(size > 0)
    {
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
        if(determineGrouping(currentPoint, neighbor, gnum))
        {
          voxelsList[size] = neighbor;
          size++;
          if(neighbor == nextSeed)
          {
            nextSeed = neighbor + 1;
          }
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
    // Increment or set values for the next iteration
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
// Replaces the DFS flood-fill with a three-phase scanline algorithm optimized
// for out-of-core performance.
//
// Phase 1: Forward CCL pass - assign provisional labels using backward neighbors.
//          Uses an in-memory buffer for labels to avoid cross-chunk reads from
//          OOC storage (backward neighbors may be in evicted chunks).
// Phase 2: Resolution - flatten Union-Find and build contiguous renumbering.
//          Operates entirely in-memory on the provisional labels buffer.
// Phase 3: Relabeling - write final contiguous feature IDs to the data store
//          in chunk-sequential order for optimal OOC write performance.
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
  // Backward neighbors in CCL are always in the current Z-slice or the
  // previous Z-slice, so 2 slices is sufficient. This uses O(slice) memory
  // instead of O(volume), enabling processing of datasets larger than RAM.
  // Buffer layout: slice (iz % 2) occupies [sliceOffset .. sliceOffset + sliceStride)
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

        // Check backward neighbors for existing labels
        // "Backward" means already processed in Z-Y-X scanline order
        // Read neighbor labels from the rolling buffer (direct memory access)
        int32 assignedLabel = 0;
        const usize prevSliceOffset = static_cast<usize>((iz + 1) % 2) * sliceSize;

        if(useFaceOnly)
        {
          // Face connectivity: 3 backward neighbors (-X, -Y, -Z)
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
          // FaceEdgeVertex connectivity: 13 backward neighbors
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
      // across periodic boundaries. Only boundary voxels can have wrapped
      // neighbors, so skip interior voxels. Each pair is processed once
      // (neighIdx > index) since union-find is symmetric.
      for(int64 iz = 0; iz < dimZ; iz++)
      {
        for(int64 iy = 0; iy < dimY; iy++)
        {
          for(int64 ix = 0; ix < dimX; ix++)
          {
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

                  // Only process pairs that wrap in at least one axis
                  if(!wrappedX && !wrappedY && !wrappedZ)
                  {
                    continue;
                  }

                  const int64 neighIdx = nz * sliceStride + ny * dimX + nx;
                  // Process each pair once to avoid redundant work
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
  // Phase 2: Resolution - build direct provisional-label-to-final-ID lookup
  // =========================================================================
  m_MessageHelper.sendMessage("Resolving labels and writing final feature IDs...");

  unionFind.flatten();

  // Build a direct lookup table: provisionalLabel -> finalFeatureId
  // Read provisional labels from the featureIds store (written during Phase 1).
  // Linear scan ensures feature IDs are assigned in the order that seeds
  // are first encountered (matching DFS seed-discovery order).
  std::vector<int32> labelToFinal(static_cast<usize>(nextLabel), 0);
  int32 finalFeatureCount = 0;

  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // First pass: discover label-to-final mapping by reading provisional labels
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
          if(label > 0 && labelToFinal[label] == 0)
          {
            int32 root = static_cast<int32>(unionFind.find(label));
            if(labelToFinal[root] == 0)
            {
              finalFeatureCount++;
              labelToFinal[root] = finalFeatureCount;
            }
            labelToFinal[label] = labelToFinal[root];
          }
        }
      }
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Second pass: write final feature IDs to the data store in chunk-sequential order
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
          int32 provLabel = featureIdsStore[index];
          if(provLabel > 0)
          {
            featureIdsStore[index] = labelToFinal[provLabel];
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
