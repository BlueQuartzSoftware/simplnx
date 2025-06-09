#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"

#include <cstdint>
#include <vector>

using namespace nx::core;

namespace
{

// dims: width=dims[0], height=dims[1], depth=dims[2]

/**
 * @brief This will find the 6 face neighbor's indices.
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @return
 */
std::vector<int64_t> getFaceNeighbors(int64_t currentPoint, int64_t width, int64_t height, int64_t depth)
{
  std::vector<int64_t> neighbors;
  neighbors.reserve(6);

  // decode currentPoint -> (col, row, plane)
  const int64_t col = currentPoint % width;
  const int64_t tmp = currentPoint / width;
  const int64_t row = tmp % height;
  const int64_t plane = tmp / height;

  // stride for one z-slice
  const int64_t slice = width * height;

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
 * @brief This will find all indices that are connected via the 26 face, edge or vertex
 * @param currentPoint
 * @param width
 * @param height
 * @param depth
 * @return
 */
std::vector<int64_t> getAllNeighbors(int64_t currentPoint, int64_t width, int64_t height, int64_t depth)
{
  std::vector<int64_t> neighbors;
  neighbors.reserve(26);

  // decode currentPoint -> (col, row, plane)
  int64_t col = currentPoint % width;
  int64_t tmp = currentPoint / width;
  int64_t row = tmp % height;
  int64_t plane = tmp / height;

  // stride for one z-slice
  int64_t slice = width * height;

  // baseOffset == currentPoint
  int64_t baseOffset = currentPoint;

  for(int64_t dz = -1; dz <= 1; ++dz)
  {
    int64_t p = plane + dz;
    if(p < 0 || p >= depth)
    {
      continue;
    }
    int64_t dzOff = dz * slice;

    for(int64_t dy = -1; dy <= 1; ++dy)
    {
      int64_t r = row + dy;
      if(r < 0 || r >= height)
      {
        continue;
      }
      int64_t dyOff = dy * width;

      for(int64_t dx = -1; dx <= 1; ++dx)
      {
        // skip the center voxel itself
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue;
        }
        int64_t c = col + dx;
        if(c < 0 || c >= width)
        {
          continue;
        }
        int64_t neighbor = baseOffset + dzOff + dyOff + dx;
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
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SegmentFeatures::~SegmentFeatures() = default;

// -----------------------------------------------------------------------------
Result<> SegmentFeatures::execute(IGridGeometry* gridGeom)
{
  SizeVec3 udims = gridGeom->getDimensions();

  int64 dims[3] = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};

  // Initialize sequence of execution modifiers
  int32 gnum = 1;
  int64 nextSeed = 0;
  int64 seed = getSeed(gnum, nextSeed);
  usize size = 0;

  // Initialize containers
  usize initialVoxelsListSize = 100000;
  std::vector<int64_t> voxelsList(initialVoxelsListSize, -1);

  auto start = std::chrono::steady_clock::now();

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
      int64 currentPoint = voxelsList[size - 1];
      size -= 1;
      std::vector<int64_t> neighPoints;
      if(m_UseFaceNeighbors)
      {
        neighPoints = getFaceNeighbors(currentPoint, dims[0], dims[1], dims[2]);
      }
      else
      {
        neighPoints = getAllNeighbors(currentPoint, dims[0], dims[1], dims[2]);
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
            for(std::vector<int64_t>::size_type j = size; j < voxelsList.size(); ++j)
            {
              voxelsList[j] = -1;
            }
          }
        }
      }
    }

    voxelsList.assign(initialVoxelsListSize, -1);
    gnum++;

    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      std::string message = fmt::format("Features Found: {}", gnum);
      m_MessageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, 0});
      start = std::chrono::steady_clock::now();
    }

    nextSeed = seed + 1;
    seed = getSeed(gnum, nextSeed);
  }

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Total Features Found: {}", gnum)});
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
void SegmentFeatures::randomizeFeatureIds(nx::core::Int32Array* featureIds, uint64 totalFeatures) const
{
  m_MessageHandler(IFilter::Message::Type::Info, "Randomizing Feature Ids");
  // Generate an even distribution of numbers between the min and max range
  const usize rangeMin = 1;
  const usize rangeMax = totalFeatures - 1;
  auto gen = initializeStaticVoxelSeedGenerator();
  std::uniform_real_distribution<float64> dist(0, 1);

  std::vector<int32> randomIds(totalFeatures);
  std::iota(randomIds.begin(), randomIds.end(), 0);

  //--- Shuffle elements by randomly exchanging each with one other.
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto r = static_cast<usize>(std::floor(dist(gen) * static_cast<float64>(rangeMax))); // Random remaining position.
    if(r < rangeMin)
    {
      continue;
    }

    int32 randId_i = randomIds[i];
    randomIds[i] = randomIds[r];
    randomIds[r] = randId_i;
  }

  // Now adjust all the Grain ID values for each Voxel
  auto& featureIdsStore = featureIds->getDataStoreRef();

  // instead of taking total points as an input just extract the size, so we don't walk of
  usize totalPoints = featureIdsStore.getSize();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore[i] = randomIds[featureIdsStore[i]];
  }
}
