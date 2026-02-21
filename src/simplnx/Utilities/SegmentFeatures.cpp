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
 * @brief This will find the 6 face neighbor's indices.
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @return Vector of indices
 */
std::vector<int64> getFaceNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth)
{
  std::vector<int64> neighbors;
  neighbors.reserve(6);

  // decode currentPoint -> (col, row, plane)
  const int64 col = currentPoint % width;
  const int64 tmp = currentPoint / width;
  const int64 row = tmp % height;
  const int64 plane = tmp / height;

  // stride for one z-slice
  const int64 slice = width * height;

  if(col > 0)
  {
    neighbors.push_back(currentPoint - 1);
  }
  if(col < width - 1)
  {
    neighbors.push_back(currentPoint + 1);
  }
  if(row > 0)
  {
    neighbors.push_back(currentPoint - width);
  }
  if(row < height - 1)
  {
    neighbors.push_back(currentPoint + width);
  }
  if(plane > 0)
  {
    neighbors.push_back(currentPoint - slice);
  }
  if(plane < depth - 1)
  {
    neighbors.push_back(currentPoint + slice);
  }

  return neighbors;
}

/**
 * @brief This will find all indices that are connected via the 26 face, edge or vertex neighbors
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @return vector of indices
 */
std::vector<int64> getAllNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth)
{
  std::vector<int64> neighbors;
  neighbors.reserve(26);

  // decode currentPoint -> (col, row, plane)
  const int64 col = currentPoint % width;
  const int64 tmp = currentPoint / width;
  const int64 row = tmp % height;
  const int64 plane = tmp / height;

  // stride for one z-slice
  const int64 slice = width * height;

  // baseOffset == currentPoint
  const int64 baseOffset = currentPoint;

  for(int64 dz = -1; dz <= 1; ++dz)
  {
    if(const int64 p = plane + dz; p < 0 || p >= depth)
    {
      continue;
    }
    const int64 dzOff = dz * slice;

    for(int64 dy = -1; dy <= 1; ++dy)
    {
      if(const int64 r = row + dy; r < 0 || r >= height)
      {
        continue;
      }
      const int64 dyOff = dy * width;

      for(int64 dx = -1; dx <= 1; ++dx)
      {
        // skip the center voxel itself
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }
        if(int64 c = col + dx; c < 0 || c >= width)
        {
          continue;
        }
        int64 neighbor = baseOffset + dzOff + dyOff + dx;
        neighbors.push_back(neighbor);
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
  int64 seed = 0; // Always use the very first value of the array that we are using to segment
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
        neighPoints = getFaceNeighbors(currentPoint, dims[0], dims[1], dims[2]);
        break;
      case NeighborScheme::FaceEdgeVertex:
        neighPoints = getAllNeighbors(currentPoint, dims[0], dims[1], dims[2]);
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

  m_FoundFeatures = gnum - 1; // Decrement the gnum because it will end up 1 larger than it should have been.
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

  // In-memory provisional labels buffer. This avoids reading backward neighbor
  // labels from the OOC featureIdsStore, where evicted chunks would cause
  // out-of-bounds memory access or require expensive chunk reloads.
  std::vector<int32> provisionalLabels(totalVoxels, 0);

  // =========================================================================
  // Phase 1: Forward CCL - assign provisional labels using backward neighbors
  // =========================================================================
  m_MessageHelper.sendMessage("Phase 1/2: Forward CCL pass...");

  for(int64 iz = 0; iz < dimZ; iz++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    for(int64 iy = 0; iy < dimY; iy++)
    {
      for(int64 ix = 0; ix < dimX; ix++)
      {
        const int64 index = iz * sliceStride + iy * dimX + ix;

        // Skip voxels that already have a label or are not valid
        if(provisionalLabels[index] != 0 || !isValidVoxel(index))
        {
          continue;
        }

        // Check backward neighbors for existing labels
        // "Backward" means already processed in Z-Y-X scanline order
        int32 assignedLabel = 0;

        if(useFaceOnly)
        {
          // Face connectivity: 3 backward neighbors (-X, -Y, -Z)
          // Check -X neighbor
          if(ix > 0)
          {
            const int64 neighIdx = index - 1;
            int32 neighLabel = provisionalLabels[neighIdx];
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
          // Check -Y neighbor
          if(iy > 0)
          {
            const int64 neighIdx = index - dimX;
            int32 neighLabel = provisionalLabels[neighIdx];
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
          // Check -Z neighbor
          if(iz > 0)
          {
            const int64 neighIdx = index - sliceStride;
            int32 neighLabel = provisionalLabels[neighIdx];
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
          // All (dz, dy, dx) where the neighbor comes before the current voxel
          // in Z-Y-X scanline order
          for(int64 dz = -1; dz <= 0; ++dz)
          {
            const int64 nz = iz + dz;
            if(nz < 0 || nz >= dimZ)
            {
              continue;
            }

            const int64 dyStart = (dz < 0) ? -1 : -1;
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
                // Previous Z-slice: all dx values
                dxStart = -1;
                dxEnd = 1;
              }
              else if(dy < 0)
              {
                // Same Z-slice, previous Y-row: all dx values
                dxStart = -1;
                dxEnd = 1;
              }
              else
              {
                // Same Z-slice, same Y-row: only -X
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
                int32 neighLabel = provisionalLabels[neighIdx];
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

        provisionalLabels[index] = assignedLabel;
      }
    }

    // Send progress per Z-slice
    float percentComplete = static_cast<float>(iz + 1) / static_cast<float>(dimZ) * 100.0f;
    throttledMessenger.sendThrottledMessage([percentComplete]() { return fmt::format("Phase 1/2: {:.1f}% complete", percentComplete); });
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // =========================================================================
  // Phase 2: Resolution - build direct provisional-label-to-final-ID lookup
  // =========================================================================
  m_MessageHelper.sendMessage("Phase 2/2: Resolving labels and writing final feature IDs...");

  unionFind.flatten();

  // Build a direct lookup table: provisionalLabel -> finalFeatureId
  // Linear scan ensures feature IDs are assigned in the order that seeds
  // are first encountered (matching DFS seed-discovery order).
  // The lookup table eliminates per-voxel find() calls during relabeling.
  std::vector<int32> labelToFinal(static_cast<usize>(nextLabel), 0);
  int32 finalFeatureCount = 0;

  for(usize index = 0; index < totalVoxels; index++)
  {
    int32 label = provisionalLabels[index];
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

  if(m_ShouldCancel)
  {
    return {};
  }

  // Write final feature IDs to the data store in chunk-sequential order
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    featureIdsStore.loadChunk(chunkIdx);

    // Chunk bounds are in tuple shape order [Z, Y, X] (inclusive)
    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * static_cast<usize>(sliceStride) + y * static_cast<usize>(dimX) + x;
          int32 provLabel = provisionalLabels[index];
          if(provLabel > 0)
          {
            featureIdsStore[index] = labelToFinal[provLabel];
          }
        }
      }
    }

    // Send progress
    float percentComplete = static_cast<float>(chunkIdx + 1) / static_cast<float>(numChunks) * 100.0f;
    throttledMessenger.sendThrottledMessage([percentComplete]() { return fmt::format("Phase 2/2: {:.1f}% chunks relabeled", percentComplete); });
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
