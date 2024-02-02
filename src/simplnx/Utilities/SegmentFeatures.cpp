#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
SegmentFeatures::SegmentFeatures(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(data)
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

  int32 gnum = 1;
  int64 seed = 0;
  int64 neighbor = 0;
  bool good = false;
  int64 col = 0, row = 0, plane = 0;
  usize size = 0;
  usize initialVoxelsListSize = 100000;
  std::vector<int64_t> voxelslist(initialVoxelsListSize, -1);
  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -(dims[0] * dims[1]);
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = (dims[0] * dims[1]);
  int64 nextSeed = 0;

  auto start = std::chrono::steady_clock::now();

  while(seed >= 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    seed = getSeed(gnum, nextSeed);
    nextSeed = seed + 1;
    if(seed >= 0)
    {
      size = 0;
      voxelslist[size] = seed;
      size++;
      while(size > 0)
      {
        int64 currentpoint = voxelslist[size - 1];
        size -= 1;
        col = currentpoint % dims[0];
        row = (currentpoint / dims[0]) % dims[1];
        plane = currentpoint / (dims[0] * dims[1]);
        for(int32 i = 0; i < 6; i++)
        {
          good = true;
          neighbor = currentpoint + neighpoints[i];
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
            if(determineGrouping(currentpoint, neighbor, gnum))
            {
              voxelslist[size] = neighbor;
              size++;
              if(neighbor == nextSeed)
              {
                nextSeed = neighbor + 1;
              }
              if(size >= voxelslist.size())
              {
                size = voxelslist.size();
                voxelslist.resize(size + initialVoxelsListSize);
                for(std::vector<int64_t>::size_type j = size; j < voxelslist.size(); ++j)
                {
                  voxelslist[j] = -1;
                }
              }
            }
          }
        }
      }

      voxelslist.assign(initialVoxelsListSize, -1);
      gnum++;

      auto now = std::chrono::steady_clock::now();
      // Only send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        std::string message = fmt::format("Features Found: {}", gnum);
        m_MessageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, 0});
        start = std::chrono::steady_clock::now();
      }
    }
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
SegmentFeatures::SeedGenerator SegmentFeatures::initializeStaticVoxelSeedGenerator(Int64Distribution& distribution, const int64 rangeMin, const int64 rangeMax) const
{
  SeedGenerator generator;
  generator.seed(SeedGenerator::default_seed);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);

  return generator;
}

// -----------------------------------------------------------------------------
void SegmentFeatures::randomizeFeatureIds(nx::core::Int32Array* featureIds, uint64 totalPoints, uint64 totalFeatures, Int64Distribution& distribution) const
{
  m_MessageHandler(IFilter::Message::Type::Info, "Randomizing Feature Ids");
  // Generate an even distribution of numbers between the min and max range
  const int64 rangeMin = 1;
  const int64 rangeMax = totalFeatures - 1;
  auto generator = initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  DataStructure tmpStructure;
  auto* rndNumbers = Int64Array::CreateWithStore<DataStore<int64>>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), std::vector<usize>{totalFeatures}, std::vector<usize>{1});
  auto& rndStore = rndNumbers->getDataStoreRef();

  for(int64 i = 0; i < totalFeatures; ++i)
  {
    rndStore.setValue(i, i);
  }

  int64 r = 0;
  int64 temp = 0;

  //--- Shuffle elements by randomly exchanging each with one other.
  for(int64 i = 1; i < totalFeatures; i++)
  {
    r = distribution(generator); // Random remaining position.
    if(r >= totalFeatures)
    {
      continue;
    }
    temp = rndStore.getValue(i);
    rndStore.setValue(i, rndStore.getValue(r));
    rndStore.setValue(r, temp);
  }

  // Now adjust all the Grain Id values for each Voxel
  auto& featureIdsStore = featureIds->getDataStoreRef();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore[i] = rndStore[featureIdsStore[i]];
  }
}
