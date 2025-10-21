#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

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
  SizeVec3 udims = gridGeom->getDimensions();

  const int64 dims[3] = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  // Initialize a sequence of execution modifiers
  int32 gnum = 1;
  int64 nextSeed = 0;
  int64 seed = getSeed(gnum, nextSeed);
  usize size = 0;

  // Initialize containers
  constexpr usize initialVoxelsListSize = 100000;
  std::vector<int64> voxelsList(initialVoxelsListSize, -1);

  ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
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
        }
      }
    }

    voxelsList.assign(initialVoxelsListSize, -1);
    gnum++;

    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Features Found: {}", gnum); });

    nextSeed = seed + 1;
    seed = getSeed(gnum, nextSeed);
  }

  m_MessageHelper.sendMessage(fmt::format("Total Features Found: {}", gnum));
  m_FoundFeatures = gnum;
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
  m_MessageHelper.sendMessage("Randomizing Feature Ids");
  ClusterUtilities::RandomizeFeatureIds(featureIds->getDataStoreRef(), totalFeatures);
}
