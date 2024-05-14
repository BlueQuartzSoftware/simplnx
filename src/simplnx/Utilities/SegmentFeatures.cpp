#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"

using namespace nx::core;

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

  // Initialize calculation modifiers
  int64 neighbor = 0;
  bool good = false;
  int64 col = 0, row = 0, plane = 0;

  // Initialize containers
  usize initialVoxelsListSize = 100000;
  std::vector<int64_t> voxelsList(initialVoxelsListSize, -1);

  int64 neighPoints[6] = {0, 0, 0, 0, 0, 0};
  { // Initialize neighPoints in a readable fashion
    neighPoints[0] = -(dims[0] * dims[1]);
    neighPoints[1] = -dims[0];
    neighPoints[2] = -1;
    neighPoints[3] = 1;
    neighPoints[4] = dims[0];
    neighPoints[5] = (dims[0] * dims[1]);
  }

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
      col = currentPoint % dims[0];
      row = (currentPoint / dims[0]) % dims[1];
      plane = currentPoint / (dims[0] * dims[1]);
      for(int32 i = 0; i < 6; i++)
      {
        good = true;
        neighbor = currentPoint + neighPoints[i];
        if(i == 0 && plane == 0)
        {
          good = false;
        }
        if(i == 5 && plane == (dims[2] - 1))
        {
          good = false;
        }
        if(i == 1 && row == 0)
        {
          good = false;
        }
        if(i == 4 && row == (dims[1] - 1))
        {
          good = false;
        }
        if(i == 2 && col == 0)
        {
          good = false;
        }
        if(i == 3 && col == (dims[0] - 1))
        {
          good = false;
        }
        if(good)
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
