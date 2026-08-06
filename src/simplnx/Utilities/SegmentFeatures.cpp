#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief A candidate neighbor cell. `wrapped` is true when the neighbor was reached by wrapping
 * across a periodic boundary, so the driver can report non-contiguous features.
 */
struct NeighborPoint
{
  int64 index = 0;
  bool wrapped = false;
};

/**
 * @brief This will find the 6 face neighbor's indices. When isPeriodic is true, a boundary cell
 * additionally gets the wrap-around neighbor on the opposite side of the volume (axes of extent 1
 * never wrap — the cell would be its own neighbor).
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @param isPeriodic
 * @return Vector of neighbor points
 */
std::vector<NeighborPoint> getFaceNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth, const bool isPeriodic)
{
  std::vector<NeighborPoint> neighbors;
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
    neighbors.push_back({currentPoint - 1, false});
  }
  else if(isPeriodic && width > 1)
  {
    neighbors.push_back({currentPoint + (width - 1), true});
  }
  if(col < width - 1)
  {
    neighbors.push_back({currentPoint + 1, false});
  }
  else if(isPeriodic && width > 1)
  {
    neighbors.push_back({currentPoint - (width - 1), true});
  }
  if(row > 0)
  {
    neighbors.push_back({currentPoint - width, false});
  }
  else if(isPeriodic && height > 1)
  {
    neighbors.push_back({currentPoint + width * (height - 1), true});
  }
  if(row < height - 1)
  {
    neighbors.push_back({currentPoint + width, false});
  }
  else if(isPeriodic && height > 1)
  {
    neighbors.push_back({currentPoint - width * (height - 1), true});
  }
  if(plane > 0)
  {
    neighbors.push_back({currentPoint - slice, false});
  }
  else if(isPeriodic && depth > 1)
  {
    neighbors.push_back({currentPoint + slice * (depth - 1), true});
  }
  if(plane < depth - 1)
  {
    neighbors.push_back({currentPoint + slice, false});
  }
  else if(isPeriodic && depth > 1)
  {
    neighbors.push_back({currentPoint - slice * (depth - 1), true});
  }

  return neighbors;
}

/**
 * @brief This will find all indices that are connected via the 26 face, edge or vertex neighbors.
 * When isPeriodic is true, each axis wraps independently across the volume boundary (axes of
 * extent 1 never wrap — the cell would be its own neighbor).
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @param isPeriodic
 * @return vector of neighbor points
 */
std::vector<NeighborPoint> getAllNeighbors(const int64 currentPoint, const int64 width, const int64 height, const int64 depth, const bool isPeriodic)
{
  std::vector<NeighborPoint> neighbors;
  neighbors.reserve(26);

  // decode currentPoint -> (col, row, plane)
  const int64 col = currentPoint % width;
  const int64 tmp = currentPoint / width;
  const int64 row = tmp % height;
  const int64 plane = tmp / height;

  // stride for one z-slice
  const int64 slice = width * height;

  for(int64 dz = -1; dz <= 1; ++dz)
  {
    int64 p = plane + dz;
    bool zWrapped = false;
    if(p < 0 || p >= depth)
    {
      if(!isPeriodic || depth <= 1)
      {
        continue;
      }
      p = (p + depth) % depth;
      zWrapped = true;
    }

    for(int64 dy = -1; dy <= 1; ++dy)
    {
      int64 r = row + dy;
      bool yWrapped = false;
      if(r < 0 || r >= height)
      {
        if(!isPeriodic || height <= 1)
        {
          continue;
        }
        r = (r + height) % height;
        yWrapped = true;
      }

      for(int64 dx = -1; dx <= 1; ++dx)
      {
        // skip the center voxel itself
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }
        int64 c = col + dx;
        bool xWrapped = false;
        if(c < 0 || c >= width)
        {
          if(!isPeriodic || width <= 1)
          {
            continue;
          }
          c = (c + width) % width;
          xWrapped = true;
        }
        neighbors.push_back({c + (r * width) + (p * slice), xWrapped || yWrapped || zWrapped});
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
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SegmentFeatures::~SegmentFeatures() = default;

// -----------------------------------------------------------------------------
Result<> SegmentFeatures::execute(IGridGeometry* gridGeom)
{
  ThrottledMessageHandler throttledMessenger(m_MessageHandler);

  SizeVec3 udims = gridGeom->getDimensions();

  usize totalVoxels = udims[0] * udims[1] * udims[2];

  int64 dims[3] = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  int32 gnum = 1;
  int64 nextSeed = 0;
  // A run that is already canceled on entry must not mutate anything; getSeed() stamps the seed
  // cell's FeatureId as a side effect.
  if(m_ShouldCancel)
  {
    return {};
  }
  // The first seed must be validated (and its cell stamped with gnum) by getSeed() exactly like
  // every later seed; bursting from a raw index 0 can grow a feature from a masked or phase-0
  // voxel and leaves an empty feature behind whenever index 0 cannot legitimately seed anything.
  int64 seed = getSeed(gnum, nextSeed);
  nextSeed = seed + 1;
  usize size = 0;
  bool hasNonContiguousFeature = false;

  // Initialize the burst worklist. Only indices below `size` are ever read, so the contents are
  // never pre-filled; the list keeps its high-water size across features.
  constexpr usize initialVoxelsListSize = 100000;
  std::vector<int64> voxelsList(initialVoxelsListSize);

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
      std::vector<NeighborPoint> neighPoints;
      switch(m_NeighborScheme)
      {
      case NeighborScheme::Face:
        neighPoints = getFaceNeighbors(currentPoint, dims[0], dims[1], dims[2], m_IsPeriodic);
        break;
      case NeighborScheme::FaceEdgeVertex:
        neighPoints = getAllNeighbors(currentPoint, dims[0], dims[1], dims[2], m_IsPeriodic);
        break;
      }

      for(const auto& neighborPoint : neighPoints)
      {
        if(determineGrouping(currentPoint, neighborPoint.index, gnum))
        {
          if(neighborPoint.wrapped)
          {
            hasNonContiguousFeature = true;
          }
          voxelsList[size] = neighborPoint.index;
          size++;
          if(neighborPoint.index == nextSeed)
          {
            nextSeed = neighborPoint.index + 1;
          }
          if(size >= voxelsList.size())
          {
            size = voxelsList.size();
            voxelsList.resize(size + initialVoxelsListSize);
          }
          totalVoxelsSegmented++;
        }
      }
    }

    // Send a progress message
    float percentComplete = static_cast<float>(totalVoxelsSegmented) / static_cast<float>(totalVoxels) * 100.0f;
    throttledMessenger.queueMessage("{:.2f}% - Current Feature Count: {}", percentComplete, gnum);
    // Increment or set values for the next iteration
    gnum++;
    // Get the next seed value
    seed = getSeed(gnum, nextSeed); // If seed ends up being -1, then we will exit the loop.
    nextSeed = seed + 1;
  }

  m_FoundFeatures = gnum - 1; // Decrement the gnum because it will end up 1 larger than it should have been.
  if(hasNonContiguousFeature)
  {
    m_MessageHandler.sendInfoMessage("Non-contiguous Features were found: at least one Feature wraps across a periodic boundary.");
  }
  m_MessageHandler.sendInfoMessage(fmt::format("Total Features Found: {}", m_FoundFeatures));
  return {};
}

int64 SegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  return -1;
}

bool SegmentFeatures::determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const
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
  m_MessageHandler.sendInfoMessage("Randomizing Feature Ids");
  ClusterUtilities::RandomizeFeatureIds(featureIds->getDataStoreRef(), totalFeatures);
}
